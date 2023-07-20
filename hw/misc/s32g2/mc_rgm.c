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
#include "hw/core/cpu.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/timer.h"
#include "qemu/module.h"
#include "hw/misc/s32g2/mc_rgm.h"

static int debug=1;

enum {
	REG_DES=	0x0,
	REG_FES=	0x8,
	REG_PRST0=	0x40,
	REG_PRST1=	0x48,
	REG_PRST2=	0x50,
	REG_PRST3=	0x58,
	REG_PSTAT0=	0x140,
	REG_PSTAT1=	0x148,
	REG_PSTAT2=	0x150,
	REG_PSTAT3=	0x158,
};


#define REG_INDEX(offset)         (offset / sizeof(uint32_t))
#define PERFORM_READ(reg)         s->regs[REG_INDEX(reg)] 
#define PERFORM_WRITE(reg, val)   s->regs[REG_INDEX(reg)] = val

static QEMUTimer timer1;
static uint32_t prev_rst1=0;
static uint32_t event=0;
static void trigger_hardware_init(void* opaque){
	S32G2mc_rgmState *s = S32G2_MC_RGM(opaque);
	uint32_t tmp_event=event;
	uint32_t tmp = PERFORM_READ(REG_PRST1);
	if(tmp_event && ((tmp_event&tmp)==0))
	{
		PERFORM_WRITE(REG_PSTAT1,  PERFORM_READ(REG_PSTAT1) & ~(event&0x1F));
	}
	else
	{
		PERFORM_WRITE(REG_PSTAT1,  PERFORM_READ(REG_PSTAT1) | (tmp&0x1F));
	}
	timer_del(&timer1);
	timer_deinit(&timer1);
	CPUState *cs;
	if(debug)printf("Trigger called event=0x%x tmp=0x%x\n", tmp_event, tmp);
	for (cs = first_cpu;
			cs;
			cs = CPU_NEXT(cs)){
		if(debug)printf("Trigger loop event=0x%x tmp=0x%x\n", tmp_event, tmp);
		if(tmp_event & BIT(1)){
			if(tmp & BIT(1)){
				if(debug)printf("Halted\n");
				qatomic_set(&cs->halted, true);
			}
			else {
				if(debug)printf("Continue\n");
				qatomic_set(&cs->halted, false);
				qemu_cpu_kick(cs);
			}
		}
		tmp=tmp>>1;
		tmp_event=tmp_event>>1;
	}
}


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
	if(debug)printf("%s offset=0x%lx val=0x%lx size=%d\n", __func__, offset, retVal, size); 
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

		case REG_PRST0:
			PERFORM_WRITE(REG_PRST0, val);
			PERFORM_WRITE(REG_PSTAT0, val);
			;			break;
		case REG_PRST1:
			PERFORM_WRITE(REG_PRST1, val);
			PERFORM_WRITE(REG_PRST1, val&0x1F); 
			event = (val&0x1F)^(prev_rst1); 
			prev_rst1=val;
			if(debug)printf("PRST1 val=0x%lx event=0x%x\n", val, event);
			if(event && ((event&val)==0)) 
			{ 
				if(debug)printf("Timer started\n");
				timer_init_us(&timer1, QEMU_CLOCK_VIRTUAL, trigger_hardware_init, s);
				timer_mod(&timer1, qemu_clock_get_us(QEMU_CLOCK_VIRTUAL) + 1000);
			}
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
		case REG_PSTAT1:
			return;
		case REG_PSTAT2:
			return;
		case REG_PSTAT3:
			return;

		default:
			printf("%s default action for write offset=%lx val=%lx size=%d\n", __func__, offset, val, size);
			s->regs[idx] = (uint32_t) val;
			return;
	}
	if(debug)printf("%s offset=%lx val=%lx size=%d\n", __func__, offset, val, size);
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
	PERFORM_WRITE(REG_DES,0x1);
	PERFORM_WRITE(REG_FES,0x0);
	PERFORM_WRITE(REG_PRST0,0x0);
	PERFORM_WRITE(REG_PRST1,0x0);
	PERFORM_WRITE(REG_PRST2,0x0);
	PERFORM_WRITE(REG_PRST3,0x0);
	PERFORM_WRITE(REG_PSTAT0,0x0);
	PERFORM_WRITE(REG_PSTAT1,0x1F);
	PERFORM_WRITE(REG_PSTAT2,0x0);
	PERFORM_WRITE(REG_PSTAT3,0x0);

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
