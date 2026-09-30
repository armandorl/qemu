/*
 * S32G2 QSPI simulated external NOR flash devices
 *
 * Copyright (C) 2023 Jose Armando Ruiz <armandorl@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qemu/module.h"
#include "qemu/log.h"
#include "qapi/error.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "sysemu/block-backend.h"
#include "hw/misc/s32g2/qspi_flash.h"

/*
 * JEDEC manufacturer ID for Macronix is 0xC2. The two remaining bytes
 * (memory type, capacity) are chip specific and come from the datasheet.
 * The value already used by the QSPI model before this device existed
 * (0xC2, type 0x81, capacity 0x3A) matches the MX25UW51245G octal part,
 * so it is kept as that chip's default to preserve existing behaviour.
 */
#define MX25U51245G_JEDEC_ID   0x003A25C2  /* C2 25 3A: SPI, 512Mb */
#define MX25UW51245G_JEDEC_ID  0x003A81C2  /* C2 81 3A: Octal, 512Mb */
#define QSPI_FLASH_SIZE        (64 * MiB)

static bool s32g2_qspi_flash_bus_check_address(BusState *bus, DeviceState *dev,
                                                Error **errp)
{
    if (!object_dynamic_cast(OBJECT(dev), TYPE_S32G2_QSPI_FLASH)) {
        error_setg(errp, "%s cannot be plugged into %s: not a %s device",
                   object_get_typename(OBJECT(dev)), bus->name,
                   TYPE_S32G2_QSPI_FLASH);
        return false;
    }
    return true;
}

static void s32g2_qspi_flash_bus_class_init(ObjectClass *klass, void *data)
{
    BusClass *k = BUS_CLASS(klass);

    k->check_address = s32g2_qspi_flash_bus_check_address;
    k->max_dev = 1;
}

static const TypeInfo s32g2_qspi_flash_bus_info = {
    .name          = TYPE_S32G2_QSPI_FLASH_BUS,
    .parent        = TYPE_BUS,
    .instance_size = sizeof(S32G2QspiFlashBus),
    .class_init    = s32g2_qspi_flash_bus_class_init,
};

S32G2QspiFlashState *s32g2_qspi_flash_find(BusState *bus)
{
    BusChild *kid;

    if (!bus) {
        return NULL;
    }

    QTAILQ_FOREACH(kid, &bus->children, sibling) {
        if (object_dynamic_cast(OBJECT(kid->child), TYPE_S32G2_QSPI_FLASH)) {
            return S32G2_QSPI_FLASH(kid->child);
        }
    }
    return NULL;
}

uint32_t s32g2_qspi_flash_get_jedec_id(S32G2QspiFlashState *flash,
                                        uint32_t default_id)
{
    return flash ? flash->jedec_id : default_id;
}

void s32g2_qspi_flash_persist(S32G2QspiFlashState *flash, hwaddr offset,
                               const uint8_t *data, size_t len)
{
    if (!flash || !flash->blk) {
        return;
    }
    if (blk_pwrite(flash->blk, offset, len, data, 0) < 0) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: failed to persist %zu bytes at 0x%" HWADDR_PRIx "\n",
                      __func__, len, offset);
    }
}

static void s32g2_qspi_flash_instance_init(Object *obj)
{
    S32G2QspiFlashState *s = S32G2_QSPI_FLASH(obj);
    S32G2QspiFlashClass *k = S32G2_QSPI_FLASH_GET_CLASS(obj);

    s->jedec_id = k->default_jedec_id;
    s->size = k->default_size;
}

static Property s32g2_qspi_flash_props[] = {
    DEFINE_PROP_UINT32("jedec-id", S32G2QspiFlashState, jedec_id, 0),
    DEFINE_PROP_UINT64("size", S32G2QspiFlashState, size, 0),
    DEFINE_PROP_DRIVE("drive", S32G2QspiFlashState, blk),
    DEFINE_PROP_END_OF_LIST(),
};

static void s32g2_qspi_flash_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->bus_type = TYPE_S32G2_QSPI_FLASH_BUS;
    device_class_set_props(dc, s32g2_qspi_flash_props);
}

static const TypeInfo s32g2_qspi_flash_info = {
    .name          = TYPE_S32G2_QSPI_FLASH,
    .parent        = TYPE_DEVICE,
    .abstract      = true,
    .instance_size = sizeof(S32G2QspiFlashState),
    .instance_init = s32g2_qspi_flash_instance_init,
    .class_size    = sizeof(S32G2QspiFlashClass),
    .class_init    = s32g2_qspi_flash_class_init,
};

static void s32g2_qspi_flash_mx25u51245g_class_init(ObjectClass *klass,
                                                     void *data)
{
    S32G2QspiFlashClass *k = S32G2_QSPI_FLASH_CLASS(klass);

    k->default_jedec_id = MX25U51245G_JEDEC_ID;
    k->default_size = QSPI_FLASH_SIZE;
}

static const TypeInfo s32g2_qspi_flash_mx25u51245g_info = {
    .name          = TYPE_S32G2_QSPI_FLASH_MX25U51245G,
    .parent        = TYPE_S32G2_QSPI_FLASH,
    .class_init    = s32g2_qspi_flash_mx25u51245g_class_init,
};

static void s32g2_qspi_flash_mx25uw51245g_class_init(ObjectClass *klass,
                                                      void *data)
{
    S32G2QspiFlashClass *k = S32G2_QSPI_FLASH_CLASS(klass);

    k->default_jedec_id = MX25UW51245G_JEDEC_ID;
    k->default_size = QSPI_FLASH_SIZE;
}

static const TypeInfo s32g2_qspi_flash_mx25uw51245g_info = {
    .name          = TYPE_S32G2_QSPI_FLASH_MX25UW51245G,
    .parent        = TYPE_S32G2_QSPI_FLASH,
    .class_init    = s32g2_qspi_flash_mx25uw51245g_class_init,
};

static void s32g2_qspi_flash_register(void)
{
    type_register_static(&s32g2_qspi_flash_bus_info);
    type_register_static(&s32g2_qspi_flash_info);
    type_register_static(&s32g2_qspi_flash_mx25u51245g_info);
    type_register_static(&s32g2_qspi_flash_mx25uw51245g_info);
}

type_init(s32g2_qspi_flash_register)
