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

#ifndef HW_MISC_S32G2_QSPI_FLASH_H
#define HW_MISC_S32G2_QSPI_FLASH_H

#include "qom/object.h"
#include "hw/qdev-core.h"
#include "sysemu/block-backend.h"
#include "exec/hwaddr.h"

/**
 * @name Flash attachment bus
 *
 * The QSPI controller exposes a single-device bus so that a simulated
 * external flash chip can be attached with e.g.
 * -device mx25uw51245g,bus=qspi-flash-bus.0
 * @{
 */

#define TYPE_S32G2_QSPI_FLASH_BUS "s32g2-qspi-flash-bus"
OBJECT_DECLARE_SIMPLE_TYPE(S32G2QspiFlashBus, S32G2_QSPI_FLASH_BUS)

struct S32G2QspiFlashBus {
    BusState parent_obj;
};

/** @} */

/**
 * @name Flash device model
 * @{
 */

#define TYPE_S32G2_QSPI_FLASH "s32g2-qspi-flash"
OBJECT_DECLARE_TYPE(S32G2QspiFlashState, S32G2QspiFlashClass, S32G2_QSPI_FLASH)

/** Concrete flash chip type names, selectable with -device */
#define TYPE_S32G2_QSPI_FLASH_MX25U51245G   "mx25u51245g"
#define TYPE_S32G2_QSPI_FLASH_MX25UW51245G  "mx25uw51245g"

struct S32G2QspiFlashState {
    /*< private >*/
    DeviceState parent_obj;
    /*< public >*/

    /** 3-byte JEDEC READ ID response: manufacturer | type<<8 | capacity<<16 */
    uint32_t jedec_id;

    /** Simulated array size in bytes */
    uint64_t size;

    /** Optional backing image; writes are persisted here if attached */
    BlockBackend *blk;
};

struct S32G2QspiFlashClass {
    /*< private >*/
    DeviceClass parent_class;
    /*< public >*/

    /** Per chip-model defaults, applied at instance_init */
    uint32_t default_jedec_id;
    uint64_t default_size;
};

/** @} */

/**
 * s32g2_qspi_flash_find:
 * @bus: the flash attachment bus of a QSPI controller
 *
 * Returns the flash device attached to @bus, or NULL if none is attached.
 */
S32G2QspiFlashState *s32g2_qspi_flash_find(BusState *bus);

/**
 * s32g2_qspi_flash_get_jedec_id:
 * @flash: a flash device, or NULL
 * @default_id: value returned when @flash is NULL
 *
 * Returns the JEDEC READ ID response reported by @flash, or @default_id
 * when no flash device is attached.
 */
uint32_t s32g2_qspi_flash_get_jedec_id(S32G2QspiFlashState *flash,
                                        uint32_t default_id);

/**
 * s32g2_qspi_flash_persist:
 * @flash: a flash device, or NULL (no-op)
 * @offset: byte offset within the flash array
 * @data: bytes written by the guest at @offset
 * @len: number of bytes in @data
 *
 * Persists a guest write to @flash's backing image (if it has a "drive"
 * attached). No-op if @flash is NULL or has no backing drive.
 */
void s32g2_qspi_flash_persist(S32G2QspiFlashState *flash, hwaddr offset,
                               const uint8_t *data, size_t len);

#endif /* HW_MISC_S32G2_QSPI_FLASH_H */
