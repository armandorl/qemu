/*
 * QEMU SPI bus interface.
 *
 * Copyright (c) 2023 based on the I2C bus.
 * Written by Jose Armando Ruiz
 *
 * This code is licensed under the LGPL.
 */

#include "qemu/osdep.h"
#include "hw/spi/spi.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
#include "qemu/module.h"
#include "qemu/main-loop.h"
#include "trace.h"

#define SPI_BROADCAST 0x00

static Property spi_props[] = {
    DEFINE_PROP_UINT8("address", struct SPISlave, address, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static const TypeInfo conventional_spi_interface_info = {
    .name          = INTERFACE_CONVENTIONAL_SPI_DEVICE,
    .parent        = TYPE_INTERFACE,
};

static const TypeInfo spi_bus_info = {
    .name = TYPE_SPI_BUS,
    .parent = TYPE_BUS,
    .instance_size = sizeof(SPIBus),
};

static int spi_bus_pre_save(void *opaque)
{
    SPIBus *bus = opaque;

    bus->saved_address = -1;
    if (!QLIST_EMPTY(&bus->current_devs)) {
        if (!bus->broadcast) {
            bus->saved_address = QLIST_FIRST(&bus->current_devs)->elt->address;
        } else {
            bus->saved_address = SPI_BROADCAST;
        }
    }

    return 0;
}

static const VMStateDescription vmstate_spi_bus = {
    .name = "spi_bus",
    .version_id = 1,
    .minimum_version_id = 1,
    .pre_save = spi_bus_pre_save,
    .fields = (VMStateField[]) {
        VMSTATE_UINT8(saved_address, SPIBus),
        VMSTATE_END_OF_LIST()
    }
};

/* Create a new SPI bus.  */
SPIBus *spi_init_bus(DeviceState *parent, const char *name)
{
    SPIBus *bus;

    bus = SPI_BUS(qbus_new(TYPE_SPI_BUS, parent, name));
    QLIST_INIT(&bus->current_devs);
    QSIMPLEQ_INIT(&bus->pending_masters);
    vmstate_register(NULL, VMSTATE_INSTANCE_ID_ANY, &vmstate_spi_bus, bus);
    return bus;
}

void spi_slave_set_address(SPISlave *dev, uint8_t address)
{
    dev->address = address;
}

/* Return nonzero if bus is busy.  */
int spi_bus_busy(SPIBus *bus)
{
    return !QLIST_EMPTY(&bus->current_devs) || bus->bh;
}

bool spi_scan_bus(SPIBus *bus, uint8_t address, bool broadcast,
                  SPINodeList *current_devs)
{
    BusChild *kid;

    QTAILQ_FOREACH(kid, &bus->qbus.children, sibling) {
        DeviceState *qdev = kid->child;
        SPISlave *candidate = SPI_SLAVE(qdev);
        SPISlaveClass *sc = SPI_SLAVE_GET_CLASS(candidate);

        if (sc->match_and_add(candidate, address, broadcast, current_devs)) {
            if (!broadcast) {
                return true;
            }
        }
    }

    /*
     * If broadcast was true, and the list was full or empty, return true. If
     * broadcast was false, return false.
     */
    return broadcast;
}

/* TODO: Make this handle multiple masters.  */
/*
 * Start or continue an spi transaction.  When this is called for the
 * first time or after an spi_end_transfer(), if it returns an error
 * the bus transaction is terminated (or really never started).  If
 * this is called after another spi_start_transfer() without an
 * intervening spi_end_transfer(), and it returns an error, the
 * transaction will not be terminated.  The caller must do it.
 *
 * This corresponds with the way real hardware works.  The SMBus
 * protocol uses a start transfer to switch from write to read mode
 * without releasing the bus.  If that fails, the bus is still
 * in a transaction.
 *
 * @event must be SPI_START_RECV or SPI_START_SEND.
 */
static int spi_do_start_transfer(SPIBus *bus, uint8_t address,
                                 enum spi_event event)
{
    SPISlaveClass *sc;
    SPINode *node;
    bool bus_scanned = false;

    if (address == SPI_BROADCAST) {
        /*
         * This is a broadcast, the current_devs will be all the devices of the
         * bus.
         */
        bus->broadcast = true;
    }

    /*
     * If there are already devices in the list, that means we are in
     * the middle of a transaction and we shouldn't rescan the bus.
     *
     * This happens with any SMBus transaction, even on a pure SPI
     * device.  The interface does a transaction start without
     * terminating the previous transaction.
     */
    if (QLIST_EMPTY(&bus->current_devs)) {
        /* Disregard whether devices were found. */
        (void)spi_scan_bus(bus, address, bus->broadcast, &bus->current_devs);
        bus_scanned = true;
    }

    if (QLIST_EMPTY(&bus->current_devs)) {
        return 1;
    }

    QLIST_FOREACH(node, &bus->current_devs, next) {
        SPISlave *s = node->elt;
        int rv;

        sc = SPI_SLAVE_GET_CLASS(s);
        /* If the bus is already busy, assume this is a repeated
           start condition.  */

        if (sc->event) {
            trace_spi_event(event == SPI_START_SEND ? "start" : "start_async",
                            s->address);
            rv = sc->event(s, event);
            if (rv && !bus->broadcast) {
                if (bus_scanned) {
                    /* First call, terminate the transfer. */
                    spi_end_transfer(bus);
                }
                return rv;
            }
        }
    }
    return 0;
}

int spi_start_transfer(SPIBus *bus, uint8_t address, bool is_recv)
{
    return spi_do_start_transfer(bus, address, is_recv
                                               ? SPI_START_RECV
                                               : SPI_START_SEND);
}

void spi_bus_master(SPIBus *bus, QEMUBH *bh)
{
    SPIPendingMaster *node = g_new(struct SPIPendingMaster, 1);
    node->bh = bh;

    QSIMPLEQ_INSERT_TAIL(&bus->pending_masters, node, entry);
}

void spi_schedule_pending_master(SPIBus *bus)
{
    SPIPendingMaster *node;

    if (spi_bus_busy(bus)) {
        /* someone is already controlling the bus; wait for it to release it */
        return;
    }

    if (QSIMPLEQ_EMPTY(&bus->pending_masters)) {
        return;
    }

    node = QSIMPLEQ_FIRST(&bus->pending_masters);
    bus->bh = node->bh;

    QSIMPLEQ_REMOVE_HEAD(&bus->pending_masters, entry);
    g_free(node);

    qemu_bh_schedule(bus->bh);
}

void spi_bus_release(SPIBus *bus)
{
    bus->bh = NULL;

    spi_schedule_pending_master(bus);
}

int spi_start_recv(SPIBus *bus, uint8_t address)
{
    return spi_do_start_transfer(bus, address, SPI_START_RECV);
}

int spi_start_send(SPIBus *bus, uint8_t address)
{
    return spi_do_start_transfer(bus, address, SPI_START_SEND);
}

int spi_start_send_async(SPIBus *bus, uint8_t address)
{
    return spi_do_start_transfer(bus, address, SPI_START_SEND_ASYNC);
}

void spi_end_transfer(SPIBus *bus)
{
    SPISlaveClass *sc;
    SPINode *node, *next;

    QLIST_FOREACH_SAFE(node, &bus->current_devs, next, next) {
        SPISlave *s = node->elt;
        sc = SPI_SLAVE_GET_CLASS(s);
        if (sc->event) {
            trace_spi_event("finish", s->address);
            sc->event(s, SPI_FINISH);
        }
        QLIST_REMOVE(node, next);
        g_free(node);
    }
    bus->broadcast = false;
}

int spi_send(SPIBus *bus, uint8_t data)
{
    SPISlaveClass *sc;
    SPISlave *s;
    SPINode *node;
    int ret = 0;

    QLIST_FOREACH(node, &bus->current_devs, next) {
        s = node->elt;
        sc = SPI_SLAVE_GET_CLASS(s);
        if (sc->send) {
            trace_spi_send(s->address, data);
            ret = ret || sc->send(s, data);
        } else {
            ret = -1;
        }
    }

    return ret ? -1 : 0;
}

int spi_send_async(SPIBus *bus, uint8_t data)
{
    SPINode *node = QLIST_FIRST(&bus->current_devs);
    SPISlave *slave = node->elt;
    SPISlaveClass *sc = SPI_SLAVE_GET_CLASS(slave);

    if (!sc->send_async) {
        return -1;
    }

    trace_spi_send_async(slave->address, data);

    sc->send_async(slave, data);

    return 0;
}

uint8_t spi_recv(SPIBus *bus)
{
    uint8_t data = 0xff;
    SPISlaveClass *sc;
    SPISlave *s;

    if (!QLIST_EMPTY(&bus->current_devs) && !bus->broadcast) {
        sc = SPI_SLAVE_GET_CLASS(QLIST_FIRST(&bus->current_devs)->elt);
        if (sc->recv) {
            s = QLIST_FIRST(&bus->current_devs)->elt;
            data = sc->recv(s);
            trace_spi_recv(s->address, data);
        }
    }

    return data;
}

void spi_nack(SPIBus *bus)
{
    SPISlaveClass *sc;
    SPINode *node;

    if (QLIST_EMPTY(&bus->current_devs)) {
        return;
    }

    QLIST_FOREACH(node, &bus->current_devs, next) {
        sc = SPI_SLAVE_GET_CLASS(node->elt);
        if (sc->event) {
            trace_spi_event("nack", node->elt->address);
            sc->event(node->elt, SPI_NACK);
        }
    }
}

void spi_ack(SPIBus *bus)
{
    if (!bus->bh) {
        return;
    }

    trace_spi_ack();

    qemu_bh_schedule(bus->bh);
}

static int spi_slave_post_load(void *opaque, int version_id)
{
    SPISlave *dev = opaque;
    SPIBus *bus;
    SPINode *node;

    bus = SPI_BUS(qdev_get_parent_bus(DEVICE(dev)));
    if ((bus->saved_address == dev->address) ||
        (bus->saved_address == SPI_BROADCAST)) {
        node = g_new(struct SPINode, 1);
        node->elt = dev;
        QLIST_INSERT_HEAD(&bus->current_devs, node, next);
    }
    return 0;
}

const VMStateDescription vmstate_spi_slave = {
    .name = "SPISlave",
    .version_id = 1,
    .minimum_version_id = 1,
    .post_load = spi_slave_post_load,
    .fields = (VMStateField[]) {
        VMSTATE_UINT8(address, SPISlave),
        VMSTATE_END_OF_LIST()
    }
};

SPISlave *spi_slave_new(const char *name, uint8_t addr)
{
    DeviceState *dev;

    dev = qdev_new(name);
    qdev_prop_set_uint8(dev, "address", addr);
    return SPI_SLAVE(dev);
}

bool spi_slave_realize_and_unref(SPISlave *dev, SPIBus *bus, Error **errp)
{
    return qdev_realize_and_unref(&dev->qdev, &bus->qbus, errp);
}

SPISlave *spi_slave_create_simple(SPIBus *bus, const char *name, uint8_t addr)
{
    SPISlave *dev = spi_slave_new(name, addr);

    spi_slave_realize_and_unref(dev, bus, &error_abort);

    return dev;
}

static bool spi_slave_match(SPISlave *candidate, uint8_t address,
                            bool broadcast, SPINodeList *current_devs)
{
    if ((candidate->address == address) || (broadcast)) {
        SPINode *node = g_new(struct SPINode, 1);
        node->elt = candidate;
        QLIST_INSERT_HEAD(current_devs, node, next);
        return true;
    }

    /* Not found and not broadcast. */
    return false;
}

static void spi_slave_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *k = DEVICE_CLASS(klass);
    SPISlaveClass *sc = SPI_SLAVE_CLASS(klass);
    set_bit(DEVICE_CATEGORY_MISC, k->categories);
    k->bus_type = TYPE_SPI_BUS;
    device_class_set_props(k, spi_props);
    sc->match_and_add = spi_slave_match;
}

static const TypeInfo spi_slave_type_info = {
    .name = TYPE_SPI_SLAVE,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(SPISlave),
    .abstract = true,
    .class_size = sizeof(SPISlaveClass),
    .class_init = spi_slave_class_init,
};

static void spi_slave_register_types(void)
{
    type_register_static(&spi_bus_info);
    type_register_static(&spi_slave_type_info);
    type_register_static(&conventional_spi_interface_info);
}

type_init(spi_slave_register_types)
