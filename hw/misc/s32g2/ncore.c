/* WARNING: This file is autogenerated do not modify manually */
/*
 * S32g2 Ncore concerto config
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
#include "hw/misc/s32g2/ncore.h"

static int debug=0;

enum {
	REG_STAT=	0x0,
};


#define REG_INDEX(offset)         (offset / sizeof(uint32_t))
#define PERFORM_READ(reg)         s->regs[REG_INDEX(reg)] 
#define PERFORM_WRITE(reg, val)   s->regs[REG_INDEX(reg)] = val



static uint64_t s32g2_ncore_read(void *opaque, hwaddr offset,
		unsigned size)
{
	const S32G2ncoreState *s = S32G2_NCORE(opaque);
	const uint32_t idx = REG_INDEX(offset);

	if (idx >= S32G2_NCORE_REGS_NUM) {
		qemu_log_mask(LOG_GUEST_ERROR, "%s: out-of-bounds offset 0x%04x\n",
				__func__, (uint32_t)offset);
		return 0;
	}

	uint64_t retVal = s->regs[idx];
	if(debug)printf("%s offset=0x%lx val=0x%lx size=%d\n", __func__, offset, retVal, size); 
	return retVal;
}

static void s32g2_ncore_write(void *opaque, hwaddr offset,
		uint64_t val, unsigned size)
{
	S32G2ncoreState *s = S32G2_NCORE(opaque);
	const uint32_t idx = REG_INDEX(offset);

	if (idx >= S32G2_NCORE_REGS_NUM) {
		qemu_log_mask(LOG_GUEST_ERROR, "%s: out-of-bounds offset 0x%04x\n",
				__func__, (uint32_t)offset);
		return;
	}

	if(debug)printf("%s offset=%lx val=%lx size=%d\n", __func__, offset, val, size);
	switch (offset) {


		default:
			printf("%s default action for write offset=%lx val=%lx size=%d\n", __func__, offset, val, size);
			s->regs[idx] = (uint32_t) val;
			return;
	}
}

static const MemoryRegionOps s32g2_ncore_ops = {
	.read = s32g2_ncore_read,
	.write = s32g2_ncore_write,
	.endianness = DEVICE_NATIVE_ENDIAN,
	.valid = {
		.min_access_size = 1,
		.max_access_size = 8,
	},
	.impl.min_access_size = 1,
};

static void s32g2_ncore_reset(DeviceState *dev)
{
	S32G2ncoreState *s = S32G2_NCORE(dev); 

	/* Set default values for registers */
	PERFORM_WRITE(REG_STAT,0x0);

}

static void s32g2_ncore_init(Object *obj)
{
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
	S32G2ncoreState *s = S32G2_NCORE(obj);

	/* Memory mapping */
	memory_region_init_io(&s->iomem, OBJECT(s), &s32g2_ncore_ops, s,
			TYPE_S32G2_NCORE, 0x100000);
	sysbus_init_mmio(sbd, &s->iomem);
}

static const VMStateDescription s32g2_ncore_vmstate = {
	.name = "s32g2_ncore",
	.version_id = 1,
	.minimum_version_id = 1,
	.fields = (VMStateField[]) {
		VMSTATE_UINT32_ARRAY(regs, S32G2ncoreState, S32G2_NCORE_REGS_NUM),
		VMSTATE_END_OF_LIST()
	}
};

static void s32g2_ncore_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->reset = s32g2_ncore_reset;
	dc->vmsd = &s32g2_ncore_vmstate;
}

static const TypeInfo s32g2_ncore_info = {
	.name          = TYPE_S32G2_NCORE,
	.parent        = TYPE_SYS_BUS_DEVICE,
	.instance_init = s32g2_ncore_init,
	.instance_size = sizeof(S32G2ncoreState),
	.class_init    = s32g2_ncore_class_init,
};

static void s32g2_ncore_register(void)
{
	type_register_static(&s32g2_ncore_info);
}

type_init(s32g2_ncore_register)
