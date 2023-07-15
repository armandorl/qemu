

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "hw/sysbus.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/timer.h"
#include "qemu/module.h"
#include "qemu/qemu-plugin.h"
#include "hw/qdev-core.h"
#include "hw/spi/spi.h"
#include "hw/net/atwilc.h"


static void atwilc1000_receive(void *opaque, const uint8_t *data, size_t size);
static void atwilc1000_transmit(void *opaque, const uint8_t *data, size_t size);

static int atwilc1000_send(SPISlave *s, uint8_t data)
{
    printf("Send spi data %x\n", data);
    atwilc1000_transmit(s, &data, 1);
    return 0;
}

static uint8_t atwilc1000_recv(SPISlave *s)
{
    printf("Rx spi data\n");
    uint8_t data = 0;
    atwilc1000_receive(s, &data, 1);
    return data;
}

static void atwilc1000_receive(void *opaque, const uint8_t *data, size_t size) {
    /*ATWILC1000State *s = ATWILC1000(opaque); */

    /* Process received data from ATWILC1000 */
    for (size_t i = 0; i < size; i++) {
        /* Read and process the received data byte by byte */
        uint8_t received_byte = data[i];

        /* Access ATWILC1000 registers based on the received data */
        switch (received_byte) {
            case 0x00:
                /* Register 0x00: Handle functionality */
                /* Handle the specific functionality */
                break;
            case 0x01:
                /* Register 0x01: Handle functionality */
                /* Handle the specific functionality */
                break;
            /* Add cases for other registers */
            default:
                /* Unknown register: Handle error condition */
                break;
        }
    }
}

static void atwilc1000_transmit(void *opaque, const uint8_t *data, size_t size) {
    /* ATWILC1000State *s = ATWILC1000(opaque); */

    /* Transmit data to ATWILC1000 */
    for (size_t i = 0; i < size; i++) {
        /* Access ATWILC1000 registers based on the transmitted data */
        uint8_t transmit_byte = data[i];

        switch (transmit_byte) {
            case 0x10:
                /* Register 0x10: Handle functionality */
                /* Handle the specific functionality */
                break;
            case 0x11:
                /* Register 0x11: Handle functionality */
                /* Handle the specific functionality */
                break;
            /* Add cases for other registers */
            default:
                /* Unknown register: Handle error condition */
                break;
        }
    }
}

static void atwilc1000_realize(DeviceState *dev, Error **errp) {
#if 0
    ATWILC1000State *s = ATWILC1000(dev);
    g_autofree char *name = g_strdup_printf(TYPE_ATWILC1000 ".%d", 0);

    /* Initialize the SPI interface */
/*    s->bus = spi_init_bus(dev, "spi0"); */
    uint32_t address = 1;
    SPISlave *spi = spi_slave_new(TYPE_ATWILC1000, address);
    SPISlaveClass *sc = SPI_SLAVE_CLASS(spi);
    
    sc->send = atwilc1000_send;
    sc->recv = atwilc1000_recv;

#endif
    /* Perform other initialization and setup here */
    /* ... */
}

static Property atwilc1000_properties[] = {
    /* Define your device properties here */
    DEFINE_PROP_END_OF_LIST(),
};

void atwilc1000_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);
    SPISlaveClass *atc = SPI_SLAVE_CLASS(klass);

    dc->realize = atwilc1000_realize;
    device_class_set_props(dc, atwilc1000_properties);

    atc->send = atwilc1000_send;
    atc->recv = atwilc1000_recv;
}

static void atwilc1000_register_types(void) {
    type_register_static(&atwilc1000_info);
}

void atwilc1000_instance_init(Object *obj)
{
    ATWILC1000State *atwilc = ATWILC1000(obj);

    object_property_add_uint32_ptr(obj, "cs_gpio",
                                   &atwilc->cs_gpio, OBJ_PROP_FLAG_READWRITE);
    object_property_add_uint32_ptr(obj, "irq_qpio_",
                                   &atwilc->irq_gpio, OBJ_PROP_FLAG_READWRITE);
}

type_init(atwilc1000_register_types)
