/* WARNING: This file is autogenerated do not modify manually */
/*
 * S32g2 System Integration Unit Lite2
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
#include "qemu/timer.h"
#include "qemu/module.h"
#include "hw/misc/s32g2/siul2.h"

static int debug=0;

enum {
	REG_MIDR1=	0x4,
	REG_MIDR2=	0x8,
};


#define REG_INDEX(offset)         (offset / sizeof(uint32_t))
#define PERFORM_READ(reg)         s->regs[REG_INDEX(reg)] 
#define PERFORM_WRITE(reg, val)   s->regs[REG_INDEX(reg)] = val



static uint64_t s32g2_siul2_read(void *opaque, hwaddr offset,
                                          unsigned size)
{
    const S32G2siul2State *s = S32G2_SIUL2(opaque);
    const uint32_t idx = REG_INDEX(offset);

    if (idx >= S32G2_SIUL2_REGS_NUM) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: out-of-bounds offset 0x%04x\n",
                      __func__, (uint32_t)offset);
        return 0;
    }

    uint64_t retVal = s->regs[idx];
    if(debug)printf("%s offset=0x%lx val=0x%lx\n", __func__, offset, retVal); 
    return retVal;
}

static void s32g2_siul2_write(void *opaque, hwaddr offset,
                                       uint64_t val, unsigned size)
{
    S32G2siul2State *s = S32G2_SIUL2(opaque);
    const uint32_t idx = REG_INDEX(offset);

    if (idx >= S32G2_SIUL2_REGS_NUM) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: out-of-bounds offset 0x%04x\n",
                      __func__, (uint32_t)offset);
        return;
    }

    switch (offset) {
    
		case REG_MIDR1:
			return;
		case REG_MIDR2:
			return;

    default:
        printf("%s default action for write offset=%lx val=%lx\n", __func__, offset, val);
        s->regs[idx] = (uint32_t) val;
        return;
    }
    if(debug)printf("%s offset=%lx val=%lx\n", __func__, offset, val);
}

static const MemoryRegionOps s32g2_siul2_ops = {
    .read = s32g2_siul2_read,
    .write = s32g2_siul2_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
    .impl.min_access_size = 4,
};

static void s32g2_siul2_reset(DeviceState *dev)
{
    S32G2siul2State *s = S32G2_SIUL2(dev); 

    /* Set default values for registers */
    	PERFORM_WRITE(REG_MIDR1,0x1D120011);
	PERFORM_WRITE(REG_MIDR2,0x48BB0000);

}

static void s32g2_siul2_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    S32G2siul2State *s = S32G2_SIUL2(obj);

    /* Memory mapping */
    memory_region_init_io(&s->iomem, OBJECT(s), &s32g2_siul2_ops, s,
                           TYPE_S32G2_SIUL2, 0x2000);
    sysbus_init_mmio(sbd, &s->iomem);
}

static const VMStateDescription s32g2_siul2_vmstate = {
    .name = "s32g2_siul2",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, S32G2siul2State, S32G2_SIUL2_REGS_NUM),
        VMSTATE_END_OF_LIST()
    }
};

static void s32g2_siul2_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = s32g2_siul2_reset;
    dc->vmsd = &s32g2_siul2_vmstate;
}

static const TypeInfo s32g2_siul2_info = {
    .name          = TYPE_S32G2_SIUL2,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_init = s32g2_siul2_init,
    .instance_size = sizeof(S32G2siul2State),
    .class_init    = s32g2_siul2_class_init,
};

static void s32g2_siul2_register(void)
{
    type_register_static(&s32g2_siul2_info);
}

type_init(s32g2_siul2_register)
