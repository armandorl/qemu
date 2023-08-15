/*
 * S32G274 emulation
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
#include "exec/address-spaces.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "hw/arm/s32g2.h"
#include "hw/sd/sd.h"
#include "hw/spi/spi.h"
#include "hw/net/atwilc.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_host.h"
#include "net/net.h"
#include "hw/pci-host/gpex.h"

static struct arm_boot_info s32g_vnp_rdb2_binfo;

static void create_pcie(MachineState *ms, PCIBus *bus)
{
    hwaddr base_mmio = 0x5800000000 ;
    hwaddr size_mmio = 16*1024*1024;
    hwaddr base_ecam, size_ecam;
    MemoryRegion *ecam_alias;
    MemoryRegion *ecam_reg;
    MemoryRegion *mmio_alias;
    MemoryRegion *mmio_reg;
    DeviceState *dev;
    int i;
    PCIHostState *pci;

    dev = qdev_new(TYPE_GPEX_HOST);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);

    base_ecam = 0x40400000;
    size_ecam = 4096;
    /* Map only the first size_ecam bytes of ECAM space */
    ecam_alias = g_new0(MemoryRegion, 1);
    ecam_reg = sysbus_mmio_get_region(SYS_BUS_DEVICE(dev), 0);
    memory_region_init_alias(ecam_alias, OBJECT(dev), "pcie-ecam",
                             ecam_reg, 0, size_ecam);
    memory_region_add_subregion(get_system_memory(), base_ecam, ecam_alias);


    /* Map the MMIO window into system address space so as to expose
     * the section of PCI MMIO space which starts at the same base address
     * (ie 1:1 mapping for that part of PCI MMIO space visible through
     * the window).
     */
    mmio_alias = g_new0(MemoryRegion, 1);
    mmio_reg = sysbus_mmio_get_region(SYS_BUS_DEVICE(dev), 1);
    memory_region_init_alias(mmio_alias, OBJECT(dev), "pcie-mmio",
                             mmio_reg, base_mmio, size_mmio);
    memory_region_add_subregion(get_system_memory(), base_mmio, mmio_alias);

#if 0
    for (i = 0; i < GPEX_NUM_IRQS; i++) {
        sysbus_connect_irq(SYS_BUS_DEVICE(dev), i,
                           qdev_get_gpio_in(vms->gic, irq + i));
        gpex_set_irq_num(GPEX_HOST(dev), i, irq + i);
    }
#endif

    pci = PCI_HOST_BRIDGE(dev);
   /*  pci->bypass_iommu = vms->default_bus_bypass_iommu; */
    bus = pci->bus;
    if (bus) {
        for (i = 0; i < nb_nics; i++) {
            NICInfo *nd = &nd_table[i];

            if (!nd->model) {
                nd->model = g_strdup("virtio");
            }

            pci_nic_init_nofail(nd, pci->bus, nd->model, NULL);
        }
    }
}

static void s32g_vnp_rdb2_init(MachineState *machine)
{
    S32G2State *s32g2_st;
    ATWILC1000State *atwilc;
    DriveInfo *di;
    BlockBackend *blk;
    BusState *bus;
    DeviceState *carddev;

    /* BIOS is not supported by this board */
    if (machine->firmware) {
        error_report("BIOS not supported for this machine");
        exit(1);
    }

#if 0
    /* Only allow Cortex-A53 for this board */
    if ((strcmp(machine->cpu_type, ARM_CPU_TYPE_NAME("cortex-a53")) != 0) || 
	(strcmp(machine->cpu_type, ARM_CPU_TYPE_NAME("cortex-m7")) != 0)  ) {
        error_report("This board can only be used with cortex-a53 or cortex-m7 CPU (%s)", machine->cpu_type);
        exit(1);
    }
#endif

    s32g2_st = S32G2(object_new(TYPE_S32G2));
    object_property_add_child(OBJECT(machine), "soc", OBJECT(s32g2_st));
    object_unref(OBJECT(s32g2_st));

    atwilc = ATWILC1000(object_new(TYPE_ATWILC1000));
    machine->smp.max_cpus = 7;

    create_pcie(machine, s32g2_st->bus);
#if 0
    object_property_add_child(OBJECT(machine), "wifi", OBJECT(atwilc));
    object_unref(OBJECT(atwilc));
    /* Setup timer properties */
    /* Freq comes from XBAR_DIV3_CLK */
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
    /* Mark S32G2 object realized */
    qdev_realize(DEVICE(s32g2_st), NULL, &error_abort);

    /* Mark ATWILC object realized */
    bus = qdev_get_child_bus(DEVICE(s32g2_st), "spi-bus.0");
    qdev_realize(DEVICE(atwilc), bus, &error_abort);
#if 0
    uint32_t atwilc_spi_address = 1;
    object_property_set_uint(OBJECT(atwilc), "address", atwilc_spi_address,
                             &error_abort);
#endif

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
    s32g_vnp_rdb2_binfo.loader_start = entry;
    s32g_vnp_rdb2_binfo.ram_size = machine->ram_size;
    s32g_vnp_rdb2_binfo.psci_conduit = QEMU_PSCI_CONDUIT_SMC;
    s32g_vnp_rdb2_binfo.firmware_loaded = 1;
    s32g_vnp_rdb2_binfo.entry = entry;
    arm_load_kernel(ARM_CPU(first_cpu), machine, &s32g_vnp_rdb2_binfo);
    
    CPUState *cs = first_cpu;
    for (cs = first_cpu; cs; cs = CPU_NEXT(cs)) {
        ARM_CPU(cs)->env.boot_info = &s32g_vnp_rdb2_binfo;
    }
}

static void s32g_vnp_rdb2_machine_init(MachineClass *mc)
{
    mc->desc = "S32G2-VNP-RDB2 (Cortex-A53 / Cortex-M7)";
    mc->init = s32g_vnp_rdb2_init;
    mc->block_default_type = IF_SD;
    mc->units_per_default_bus = 1;
    mc->min_cpus = S32G2_NUM_CPUS;
    mc->max_cpus = S32G2_NUM_CPUS + 3; // Plus 3 cortex-m7
    mc->default_cpus = S32G2_NUM_CPUS + 3;
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("cortex-a53");
    mc->default_ram_size = 2 * GiB;
    mc->default_ram_id = "s32g_vnp_rdb2.ram";
}

DEFINE_MACHINE("s32g_vnp_rdb2", s32g_vnp_rdb2_machine_init)
