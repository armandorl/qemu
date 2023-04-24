/*
 * S32g2 SRAM controller emulation
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
#include "hw/sysbus.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/misc/s32g2/sramc.h"

/* System Control register offsets */
enum {
    REG_PRAMCR            = 0x0,  /* Control Register */
    REG_PRAMIAS           = 0x4,  /* Adress Start */
    REG_PRAMIAE           = 0x8,  /* Adress End */
    REG_PRAMSR            = 0xC,  /* Status */
    REG_PRAMECCA          = 0x10, /* ECC address */
};

#define REG_INDEX(offset)   (offset / sizeof(uint32_t))


static uint64_t s32g2_sramc_read(void *opaque, hwaddr offset,
                                          unsigned size)
{
    const S32G2SramcState *s = S32G2_SRAMC(opaque);
    const uint32_t idx = REG_INDEX(offset);

    if (idx >= S32G2_SRAMC_REGS_NUM) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: out-of-bounds offset 0x%04x\n",
                      __func__, (uint32_t)offset);
        return 0;
    }

    return s->regs[idx];
}

static void s32g2_sramc_write(void *opaque, hwaddr offset,
                                       uint64_t val, unsigned size)
{
    S32G2SramcState *s = S32G2_SRAMC(opaque);
    const uint32_t idx = REG_INDEX(offset);

    if (idx >= S32G2_SRAMC_REGS_NUM) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: out-of-bounds offset 0x%04x\n",
                      __func__, (uint32_t)offset);
        return;
    }

    switch (offset) {
    case REG_PRAMCR:
	if( (val & 0x01) == 0x1)
		s->regs[REG_INDEX(REG_PRAMSR)] = 0x1;
        break;
    default:
        s->regs[idx] = (uint32_t) val;
        break;
    }
}

static const MemoryRegionOps s32g2_sramc_ops = {
    .read = s32g2_sramc_read,
    .write = s32g2_sramc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
    .impl.min_access_size = 4,
};

static void s32g2_sramc_reset(DeviceState *dev)
{
    S32G2SramcState *s = S32G2_SRAMC(dev);

    /* Set default values for registers */
    s->regs[REG_INDEX(REG_PRAMCR)] = 0;
    s->regs[REG_INDEX(REG_PRAMIAS)] = 0;
    s->regs[REG_INDEX(REG_PRAMIAE)] = 0;
    s->regs[REG_INDEX(REG_PRAMSR)] = 0;
    s->regs[REG_INDEX(REG_PRAMECCA)] = 0;
}

static void s32g2_sramc_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    S32G2SramcState *s = S32G2_SRAMC(obj);

    /* Memory mapping */
    memory_region_init_io(&s->iomem, OBJECT(s), &s32g2_sramc_ops, s,
                           TYPE_S32G2_SRAMC, 4 * KiB);
    sysbus_init_mmio(sbd, &s->iomem);
}

static const VMStateDescription s32g2_sramc_vmstate = {
    .name = "s32g2_sramc",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, S32G2SramcState, S32G2_SRAMC_REGS_NUM),
        VMSTATE_END_OF_LIST()
    }
};

static void s32g2_sramc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = s32g2_sramc_reset;
    dc->vmsd = &s32g2_sramc_vmstate;
}

static const TypeInfo s32g2_sramc_info = {
    .name          = TYPE_S32G2_SRAMC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_init = s32g2_sramc_init,
    .instance_size = sizeof(S32G2SramcState),
    .class_init    = s32g2_sramc_class_init,
};

static void s32g2_sramc_register(void)
{
    type_register_static(&s32g2_sramc_info);
}

type_init(s32g2_sramc_register)
