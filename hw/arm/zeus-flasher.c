/*
 * Orange Pi emulation
 *
 * Copyright (C) 2019 Niek Linnenbank <nieklinnenbank@gmail.com>
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
#include "exec/address-spaces.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "hw/arm/s32g2.h"
#include "hw/sd/sd.h"

static struct arm_boot_info zeusflasher_binfo;

static void zeusflasher_init(MachineState *machine)
{
    S32G2State *s32g2_st;
    DriveInfo *di;
    BlockBackend *blk;
    BusState *bus;
    DeviceState *carddev;

    /* BIOS is not supported by this board */
    if (machine->firmware) {
        error_report("BIOS not supported for this machine");
        exit(1);
    }

    /* Only allow Cortex-A53 for this board */
    if ((strcmp(machine->cpu_type, ARM_CPU_TYPE_NAME("cortex-a53")) != 0) && 
	(strcmp(machine->cpu_type, ARM_CPU_TYPE_NAME("cortex-m7")) != 0)  ) {
        error_report("This board can only be used with cortex-a53 or cortex-m7 CPU");
        exit(1);
    }

    s32g2_st = S32G2(object_new(TYPE_S32G2));
    object_property_add_child(OBJECT(machine), "soc", OBJECT(s32g2_st));
    object_unref(OBJECT(s32g2_st));

#if 0
    /* Setup timer properties */
    object_property_set_int(OBJECT(s32g2_st), "clk0-freq", 32768, &error_abort);
    object_property_set_int(OBJECT(s32g2_st), "clk1-freq", 24 * 1000 * 1000,
                            &error_abort);

    /* Setup SID properties. Currently using a default fixed SID identifier. */
    if (qemu_uuid_is_null(&s32g2_st->sid.identifier)) {
        qdev_prop_set_string(DEVICE(s32g2_st), "identifier",
                             "02c00081-1111-2222-3333-000044556677");
    } else if (ldl_be_p(&s32g2_st->sid.identifier.data[0]) != 0x02c00081) {
        warn_report("Security Identifier value does not include H3 prefix");
    }

    /* Setup EMAC properties */
    object_property_set_int(OBJECT(&s32g2_st->emac), "phy-addr", 1, &error_abort);

    /* DRAMC */
    object_property_set_uint(OBJECT(s32g2_st), "ram-addr", s32g2_st->memmap[S32G2_DEV_SDRAM],
                             &error_abort);
    object_property_set_int(OBJECT(s32g2_st), "ram-size", machine->ram_size / MiB,
                            &error_abort);

#endif
    /* Mark H3 object realized */
    qdev_realize(DEVICE(s32g2_st), NULL, &error_abort);

    /* Retrieve SD bus */
    di = drive_get(IF_SD, 0, 0);
    blk = di ? blk_by_legacy_dinfo(di) : NULL;
    bus = qdev_get_child_bus(DEVICE(s32g2_st), "sd-bus");

    /* Plug in SD card */
    carddev = qdev_new(TYPE_SD_CARD);
    qdev_prop_set_drive_err(carddev, "drive", blk, &error_fatal);
    qdev_realize_and_unref(carddev, bus, &error_fatal);

    hwaddr entry = 0;
    /* Load target kernel or start using BootROM */
    if (!machine->kernel_filename && blk && blk_is_available(blk)) {
        /* Use Boot ROM to copy data from SD card to SRAM */
        s32g2_bootrom_setup(s32g2_st, blk, &entry);
    }
    zeusflasher_binfo.loader_start = entry;
    zeusflasher_binfo.ram_size = machine->ram_size;
    zeusflasher_binfo.psci_conduit = QEMU_PSCI_CONDUIT_SMC;
    zeusflasher_binfo.firmware_loaded = 1;
    zeusflasher_binfo.entry = entry;
    arm_load_kernel(ARM_CPU(first_cpu), machine, &zeusflasher_binfo);
    
    CPUState *cs = first_cpu;
    for (cs = first_cpu; cs; cs = CPU_NEXT(cs)) {
        ARM_CPU(cs)->env.boot_info = &zeusflasher_binfo;
    }
}

static void zeusflasher_machine_init(MachineClass *mc)
{
    mc->desc = "Zeus Flasher S32G2 (Cortex-A53 / Cortex-M7)";
    mc->init = zeusflasher_init;
    mc->block_default_type = IF_SD;
    mc->units_per_default_bus = 1;
    mc->min_cpus = S32G2_NUM_CPUS;
    mc->max_cpus = S32G2_NUM_CPUS;
    mc->default_cpus = S32G2_NUM_CPUS;
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("cortex-a53");
    mc->default_ram_size = 2 * GiB;
    mc->default_ram_id = "zeus-flasher.ram";
}

DEFINE_MACHINE("zeus-flasher", zeusflasher_machine_init)
