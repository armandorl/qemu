/* WARNING: This file is autogenerated do not modify manually */
/*
 * S32g2 Reset Generation Module
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
#include "hw/misc/s32g2/mc_rgm.h"

static int debug=0;

enum {
	REG_PSTAT3=	0x158,
	REG_PSTAT2=	0x150,
	REG_PSTAT1=	0x148,
	REG_DES=	0x0,
	REG_PRST0=	0x40,
	REG_FES=	0x8,
	REG_PRST2=	0x50,
	REG_PRST3=	0x58,
	REG_PSTAT0=	0x140,
	REG_PRST1=	0x48,
};


#define REG_INDEX(offset)         (offset / sizeof(uint32_t))
#define PERFORM_READ(reg)         s->regs[REG_INDEX(reg)] 
#define PERFORM_WRITE(reg, val)   s->regs[REG_INDEX(reg)] = val



static uint64_t s32g2_mc_rgm_read(void *opaque, hwaddr offset,
                                          unsigned size)
{
    const S32G2mc_rgmState *s = S32G2_MC_RGM(opaque);
    const uint32_t idx = REG_INDEX(offset);

    if (idx >= S32G2_MC_RGM_REGS_NUM) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: out-of-bounds offset 0x%04x\n",
                      __func__, (uint32_t)offset);
        return 0;
    }

    uint64_t retVal = s->regs[idx];
    if(debug)printf("%s offset=0x%lx val=0x%lx\n", __func__, offset, retVal); 
    return retVal;
}

static void s32g2_mc_rgm_write(void *opaque, hwaddr offset,
                                       uint64_t val, unsigned size)
{
    S32G2mc_rgmState *s = S32G2_MC_RGM(opaque);
    const uint32_t idx = REG_INDEX(offset);

    if (idx >= S32G2_MC_RGM_REGS_NUM) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: out-of-bounds offset 0x%04x\n",
                      __func__, (uint32_t)offset);
        return;
    }

    switch (offset) {
    
		case REG_PSTAT3:
			return;
		case REG_PSTAT2:
			return;
		case REG_PSTAT1:
			return;
		case REG_PRST0:
PERFORM_WRITE(REG_PRST0, val);
			PERFORM_WRITE(REG_PSTAT0, val);
;			break;
		case REG_PRST2:
PERFORM_WRITE(REG_PRST2, val);
			PERFORM_WRITE(REG_PSTAT2, val);
;			break;
		case REG_PRST3:
PERFORM_WRITE(REG_PRST3, val);
			PERFORM_WRITE(REG_PSTAT3, val);
;			break;
		case REG_PSTAT0:
			return;
		case REG_PRST1:
PERFORM_WRITE(REG_PRST1, val);
			PERFORM_WRITE(REG_PSTAT1, val);
;			break;

    default:
        printf("%s default action for write offset=%lx val=%lx\n", __func__, offset, val);
        s->regs[idx] = (uint32_t) val;
        return;
    }
    if(debug)printf("%s offset=%lx val=%lx\n", __func__, offset, val);
}

static const MemoryRegionOps s32g2_mc_rgm_ops = {
    .read = s32g2_mc_rgm_read,
    .write = s32g2_mc_rgm_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
    .impl.min_access_size = 4,
};

static void s32g2_mc_rgm_reset(DeviceState *dev)
{
    S32G2mc_rgmState *s = S32G2_MC_RGM(dev); 

    /* Set default values for registers */
    	PERFORM_WRITE(REG_PSTAT3,0x0);
	PERFORM_WRITE(REG_PSTAT2,0x0);
	PERFORM_WRITE(REG_PSTAT1,0x0);
	PERFORM_WRITE(REG_DES,0x1);
	PERFORM_WRITE(REG_PRST0,0x0);
	PERFORM_WRITE(REG_FES,0x0);
	PERFORM_WRITE(REG_PRST2,0x0);
	PERFORM_WRITE(REG_PRST3,0x0);
	PERFORM_WRITE(REG_PSTAT0,0x0);
	PERFORM_WRITE(REG_PRST1,0x0);

}

static void s32g2_mc_rgm_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    S32G2mc_rgmState *s = S32G2_MC_RGM(obj);

    /* Memory mapping */
    memory_region_init_io(&s->iomem, OBJECT(s), &s32g2_mc_rgm_ops, s,
                           TYPE_S32G2_MC_RGM, 0x200);
    sysbus_init_mmio(sbd, &s->iomem);
}

static const VMStateDescription s32g2_mc_rgm_vmstate = {
    .name = "s32g2_mc_rgm",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, S32G2mc_rgmState, S32G2_MC_RGM_REGS_NUM),
        VMSTATE_END_OF_LIST()
    }
};

static void s32g2_mc_rgm_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = s32g2_mc_rgm_reset;
    dc->vmsd = &s32g2_mc_rgm_vmstate;
}

static const TypeInfo s32g2_mc_rgm_info = {
    .name          = TYPE_S32G2_MC_RGM,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_init = s32g2_mc_rgm_init,
    .instance_size = sizeof(S32G2mc_rgmState),
    .class_init    = s32g2_mc_rgm_class_init,
};

static void s32g2_mc_rgm_register(void)
{
    type_register_static(&s32g2_mc_rgm_info);
}

type_init(s32g2_mc_rgm_register)
