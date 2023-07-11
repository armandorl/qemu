#ifndef QEMU_ATWILC1000_H
#define QEMU_ATWILC1000_H

#include "qom/object.h"
#include "hw/sysbus.h"
#include "hw/spi/spi.h"

// ATWILC1000 device definition
//
#define TYPE_ATWILC1000 "atwilc1000"

#define ATWILC1000_GET_CLASS(obj) \
        OBJECT_GET_CLASS(ATWILC1000Class, (obj), TYPE_ATWILC1000)
#define ATWILC1000_CLASS(klass) \
        OBJECT_CLASS_CHECK(ATWILC1000Class, (klass), TYPE_ATWILC1000)
#define ATWILC1000(obj) \
        OBJECT_CHECK(ATWILC1000State, (obj), TYPE_ATWILC1000)


void atwilc1000_instance_init(Object *obj);
void atwilc1000_class_init(ObjectClass *klass, void *data); 


#define INTERFACE_CONVENTIONAL_SPI_DEVICE "conventional-spi-device"

typedef struct {
    SPISlaveClass parent_class;
} ATWILC1000Class;

typedef struct {
    SPISlave parent;
    QEMUFile *irq_state;
    uint32_t cs_gpio;
    uint32_t irq_gpio;
    SPIBus *bus;
    uint32_t address;
    // Add any other required state variables here
} ATWILC1000State;

static InterfaceInfo interfaces[] = {
	{ INTERFACE_CONVENTIONAL_SPI_DEVICE },
	{ },
};

static const TypeInfo atwilc1000_info = {
    .name = TYPE_ATWILC1000,
    .parent = TYPE_SPI_SLAVE,
    .instance_size = sizeof(ATWILC1000State),
    .class_init    = atwilc1000_class_init,
    .instance_init = atwilc1000_instance_init,
    .interfaces = interfaces,
};

#endif /* QEMU_ATWILC1000_H */

