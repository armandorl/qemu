#ifndef QEMU_SPI_H
#define QEMU_SPI_H

#include "hw/qdev-core.h"
#include "qom/object.h"

enum spi_event {
    SPI_START_RECV,
    SPI_START_SEND,
    SPI_START_SEND_ASYNC,
    SPI_FINISH,
    SPI_NACK /* Masker NACKed a receive byte.  */
};

typedef struct SPINodeList SPINodeList;

#define TYPE_SPI_SLAVE "spi-slave"
OBJECT_DECLARE_TYPE(SPISlave, SPISlaveClass,
                    SPI_SLAVE)

struct SPISlaveClass {
    DeviceClass parent_class;

    /* Master to slave. Returns non-zero for a NAK, 0 for success. */
    int (*send)(SPISlave *s, uint8_t data);

    /* Master to slave (asynchronous). Receiving slave must call spi_ack(). */
    void (*send_async)(SPISlave *s, uint8_t data);

    /*
     * Slave to master.  This cannot fail, the device should always
     * return something here.
     */
    uint8_t (*recv)(SPISlave *s);

    /*
     * Notify the slave of a bus state change.  For start event,
     * returns non-zero to NAK an operation.  For other events the
     * return code is not used and should be zero.
     */
    int (*event)(SPISlave *s, enum spi_event event);

    /*
     * Check if this device matches the address provided.  Returns bool of
     * true if it matches (or broadcast), and updates the device list, false
     * otherwise.
     *
     * If broadcast is true, match should add the device and return true.
     */
    bool (*match_and_add)(SPISlave *candidate, uint8_t address, bool broadcast,
                          SPINodeList *current_devs);
};

struct SPISlave {
    DeviceState qdev;

    /* Remaining fields for internal use by the SPI code.  */
    uint8_t address;
};

#define TYPE_SPI_BUS "spi-bus"
OBJECT_DECLARE_SIMPLE_TYPE(SPIBus, SPI_BUS)

typedef struct SPINode SPINode;

struct SPINode {
    SPISlave *elt;
    QLIST_ENTRY(SPINode) next;
};

typedef struct SPIPendingMaster SPIPendingMaster;

struct SPIPendingMaster {
    QEMUBH *bh;
    QSIMPLEQ_ENTRY(SPIPendingMaster) entry;
};

typedef QLIST_HEAD(SPINodeList, SPINode) SPINodeList;
typedef QSIMPLEQ_HEAD(SPIPendingMasters, SPIPendingMaster) SPIPendingMasters;

struct SPIBus {
    BusState qbus;
    SPINodeList current_devs;
    SPIPendingMasters pending_masters;
    uint8_t saved_address;
    bool broadcast;

    /* Set from slave currently mastering the bus. */
    QEMUBH *bh;
};

SPIBus *spi_init_bus(DeviceState *parent, const char *name);
int spi_bus_busy(SPIBus *bus);

/**
 * spi_start_transfer: start a transfer on an SPI bus.
 *
 * @bus: #SPIBus to be used
 * @address: address of the slave
 * @is_recv: indicates the transfer direction
 *
 * When @is_recv is a known boolean constant, use the
 * spi_start_recv() or spi_start_send() helper instead.
 *
 * Returns: 0 on success, -1 on error
 */
int spi_start_transfer(SPIBus *bus, uint8_t address, bool is_recv);

/**
 * spi_start_recv: start a 'receive' transfer on an SPI bus.
 *
 * @bus: #SPIBus to be used
 * @address: address of the slave
 *
 * Returns: 0 on success, -1 on error
 */
int spi_start_recv(SPIBus *bus, uint8_t address);

/**
 * spi_start_send: start a 'send' transfer on an SPI bus.
 *
 * @bus: #SPIBus to be used
 * @address: address of the slave
 *
 * Returns: 0 on success, -1 on error
 */
int spi_start_send(SPIBus *bus, uint8_t address);

/**
 * spi_start_send_async: start an asynchronous 'send' transfer on an SPI bus.
 *
 * @bus: #SPIBus to be used
 * @address: address of the slave
 *
 * Return: 0 on success, -1 on error
 */
int spi_start_send_async(SPIBus *bus, uint8_t address);

void spi_schedule_pending_master(SPIBus *bus);

void spi_end_transfer(SPIBus *bus);
void spi_nack(SPIBus *bus);
void spi_ack(SPIBus *bus);
void spi_bus_master(SPIBus *bus, QEMUBH *bh);
void spi_bus_release(SPIBus *bus);
int spi_send(SPIBus *bus, uint8_t data);
int spi_send_async(SPIBus *bus, uint8_t data);
uint8_t spi_recv(SPIBus *bus);
bool spi_scan_bus(SPIBus *bus, uint8_t address, bool broadcast,
                  SPINodeList *current_devs);

/**
 * Create an SPI slave device on the heap.
 * @name: a device type name
 * @addr: SPI address of the slave when put on a bus
 *
 * This only initializes the device state structure and allows
 * properties to be set. Type @name must exist. The device still
 * needs to be realized. See qdev-core.h.
 */
SPISlave *spi_slave_new(const char *name, uint8_t addr);

/**
 * Create and realize an SPI slave device on the heap.
 * @bus: SPI bus to put it on
 * @name: SPI slave device type name
 * @addr: SPI address of the slave when put on a bus
 *
 * Create the device state structure, initialize it, put it on the
 * specified @bus, and drop the reference to it (the device is realized).
 */
SPISlave *spi_slave_create_simple(SPIBus *bus, const char *name, uint8_t addr);

/**
 * Realize and drop a reference an SPI slave device
 * @dev: SPI slave device to realize
 * @bus: SPI bus to put it on
 * @addr: SPI address of the slave on the bus
 * @errp: pointer to NULL initialized error object
 *
 * Returns: %true on success, %false on failure.
 *
 * Call 'realize' on @dev, put it on the specified @bus, and drop the
 * reference to it.
 *
 * This function is useful if you have created @dev via qdev_new(),
 * spi_slave_new() or spi_slave_try_new() (which take a reference to
 * the device it returns to you), so that you can set properties on it
 * before realizing it. If you don't need to set properties then
 * spi_slave_create_simple() is probably better (as it does the create,
 * init and realize in one step).
 *
 * If you are embedding the SPI slave into another QOM device and
 * initialized it via some variant on object_initialize_child() then
 * do not use this function, because that family of functions arrange
 * for the only reference to the child device to be held by the parent
 * via the child<> property, and so the reference-count-drop done here
 * would be incorrect.  (Instead you would want spi_slave_realize(),
 * which doesn't currently exist but would be trivial to create if we
 * had any code that wanted it.)
 */
bool spi_slave_realize_and_unref(SPISlave *dev, SPIBus *bus, Error **errp);

/**
 * Set the SPI bus address of a slave device
 * @dev: SPI slave device
 * @address: SPI address of the slave when put on a bus
 */
void spi_slave_set_address(SPISlave *dev, uint8_t address);

extern const VMStateDescription vmstate_spi_slave;

#define VMSTATE_SPI_SLAVE(_field, _state) {                          \
    .name       = (stringify(_field)),                               \
    .size       = sizeof(SPISlave),                                  \
    .vmsd       = &vmstate_spi_slave,                                \
    .flags      = VMS_STRUCT,                                        \
    .offset     = vmstate_offset_value(_state, _field, SPISlave),    \
}

#endif
