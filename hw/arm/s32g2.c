/*
 * Allwinner H3 System on Chip emulation
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
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qemu/module.h"
#include "qemu/units.h"
#include "hw/qdev-core.h"
#include "hw/sysbus.h"
#include "hw/char/serial.h"
#include "hw/misc/unimp.h"
#include "hw/usb/hcd-ehci.h"
#include "hw/loader.h"
#include "sysemu/sysemu.h"
#include "hw/arm/s32g2.h"

/* Memory map */
const hwaddr s32g2_memmap[] = {
    [S32G2_DEV_SRAM_A1]    = 0x00000000,
    [S32G2_DEV_SRAM_A2]    = 0x00044000,
    [S32G2_DEV_SRAM_C]     = 0x00010000,
    [S32G2_DEV_SYSCTRL]    = 0x01c00000,
    [S32G2_DEV_MMC0]       = 0x01c0f000,
    [S32G2_DEV_SID]        = 0x01c14000,
    [S32G2_DEV_EHCI0]      = 0x01c1a000,
    [S32G2_DEV_OHCI0]      = 0x01c1a400,
    [S32G2_DEV_EHCI1]      = 0x01c1b000,
    [S32G2_DEV_OHCI1]      = 0x01c1b400,
    [S32G2_DEV_EHCI2]      = 0x01c1c000,
    [S32G2_DEV_OHCI2]      = 0x01c1c400,
    [S32G2_DEV_EHCI3]      = 0x01c1d000,
    [S32G2_DEV_OHCI3]      = 0x01c1d400,
    [S32G2_DEV_CCU]        = 0x01c20000,
    [S32G2_DEV_PIT]        = 0x01c20c00,
    [S32G2_DEV_UART0]      = 0x01c28000,
    [S32G2_DEV_UART1]      = 0x01c28400,
    [S32G2_DEV_UART2]      = 0x01c28800,
    [S32G2_DEV_UART3]      = 0x01c28c00,
    [S32G2_DEV_TWI0]       = 0x01c2ac00,
    [S32G2_DEV_TWI1]       = 0x01c2b000,
    [S32G2_DEV_TWI2]       = 0x01c2b400,
    [S32G2_DEV_EMAC]       = 0x01c30000,
    [S32G2_DEV_DRAMCOM]    = 0x01c62000,
    [S32G2_DEV_DRAMCTL]    = 0x01c63000,
    [S32G2_DEV_DRAMPHY]    = 0x01c65000,
    [S32G2_DEV_GIC_DIST]   = 0x01c81000,
    [S32G2_DEV_GIC_CPU]    = 0x01c82000,
    [S32G2_DEV_GIC_HYP]    = 0x01c84000,
    [S32G2_DEV_GIC_VCPU]   = 0x01c86000,
    [S32G2_DEV_RTC]        = 0x01f00000,
    [S32G2_DEV_CPUCFG]     = 0x01f01c00,
    [S32G2_DEV_R_TWI]      = 0x01f02400,
    [S32G2_DEV_SDRAM]      = 0x40000000
};

/* List of unimplemented devices */
struct S32G2Unimplemented {
    const char *device_name;
    hwaddr base;
    hwaddr size;
} s32g2_unimplemented[] = {
    { "d-engine",  0x01000000, 4 * MiB },
    { "d-inter",   0x01400000, 128 * KiB },
    { "dma",       0x01c02000, 4 * KiB },
    { "nfdc",      0x01c03000, 4 * KiB },
    { "ts",        0x01c06000, 4 * KiB },
    { "keymem",    0x01c0b000, 4 * KiB },
    { "lcd0",      0x01c0c000, 4 * KiB },
    { "lcd1",      0x01c0d000, 4 * KiB },
    { "ve",        0x01c0e000, 4 * KiB },
    { "mmc1",      0x01c10000, 4 * KiB },
    { "mmc2",      0x01c11000, 4 * KiB },
    { "crypto",    0x01c15000, 4 * KiB },
    { "msgbox",    0x01c17000, 4 * KiB },
    { "spinlock",  0x01c18000, 4 * KiB },
    { "usb0-otg",  0x01c19000, 4 * KiB },
    { "usb0-phy",  0x01c1a000, 4 * KiB },
    { "usb1-phy",  0x01c1b000, 4 * KiB },
    { "usb2-phy",  0x01c1c000, 4 * KiB },
    { "usb3-phy",  0x01c1d000, 4 * KiB },
    { "smc",       0x01c1e000, 4 * KiB },
    { "pio",       0x01c20800, 1 * KiB },
    { "owa",       0x01c21000, 1 * KiB },
    { "pwm",       0x01c21400, 1 * KiB },
    { "keyadc",    0x01c21800, 1 * KiB },
    { "pcm0",      0x01c22000, 1 * KiB },
    { "pcm1",      0x01c22400, 1 * KiB },
    { "pcm2",      0x01c22800, 1 * KiB },
    { "audio",     0x01c22c00, 2 * KiB },
    { "smta",      0x01c23400, 1 * KiB },
    { "ths",       0x01c25000, 1 * KiB },
    { "uart0",     0x01c28000, 1 * KiB },
    { "uart1",     0x01c28400, 1 * KiB },
    { "uart2",     0x01c28800, 1 * KiB },
    { "uart3",     0x01c28c00, 1 * KiB },
    { "scr",       0x01c2c400, 1 * KiB },
    { "gpu",       0x01c40000, 64 * KiB },
    { "hstmr",     0x01c60000, 4 * KiB },
    { "spi0",      0x01c68000, 4 * KiB },
    { "spi1",      0x01c69000, 4 * KiB },
    { "csi",       0x01cb0000, 320 * KiB },
    { "tve",       0x01e00000, 64 * KiB },
    { "hdmi",      0x01ee0000, 128 * KiB },
    { "r_timer",   0x01f00800, 1 * KiB },
    { "r_intc",    0x01f00c00, 1 * KiB },
    { "r_wdog",    0x01f01000, 1 * KiB },
    { "r_prcm",    0x01f01400, 1 * KiB },
    { "r_twd",     0x01f01800, 1 * KiB },
    { "r_cir-rx",  0x01f02000, 1 * KiB },
    { "r_uart",    0x01f02800, 1 * KiB },
    { "r_pio",     0x01f02c00, 1 * KiB },
    { "r_pwm",     0x01f03800, 1 * KiB },
    { "core-dbg",  0x3f500000, 128 * KiB },
    { "tsgen-ro",  0x3f506000, 4 * KiB },
    { "tsgen-ctl", 0x3f507000, 4 * KiB },
    { "ddr-mem",   0x40000000, 2 * GiB },
    { "n-brom",    0xffff0000, 32 * KiB },
    { "s-brom",    0xffff0000, 64 * KiB }
};

/* Per Processor Interrupts */
enum {
    S32G2_GIC_PPI_MAINT     =  9,
    S32G2_GIC_PPI_HYPTIMER  = 10,
    S32G2_GIC_PPI_VIRTTIMER = 11,
    S32G2_GIC_PPI_SECTIMER  = 13,
    S32G2_GIC_PPI_PHYSTIMER = 14
};

/* Shared Processor Interrupts */
enum {
    S32G2_GIC_SPI_UART0     =  0,
    S32G2_GIC_SPI_UART1     =  1,
    S32G2_GIC_SPI_UART2     =  2,
    S32G2_GIC_SPI_UART3     =  3,
    S32G2_GIC_SPI_TWI0      =  6,
    S32G2_GIC_SPI_TWI1      =  7,
    S32G2_GIC_SPI_TWI2      =  8,
    S32G2_GIC_SPI_TIMER0    = 18,
    S32G2_GIC_SPI_TIMER1    = 19,
    S32G2_GIC_SPI_R_TWI     = 44,
    S32G2_GIC_SPI_MMC0      = 60,
    S32G2_GIC_SPI_EHCI0     = 72,
    S32G2_GIC_SPI_OHCI0     = 73,
    S32G2_GIC_SPI_EHCI1     = 74,
    S32G2_GIC_SPI_OHCI1     = 75,
    S32G2_GIC_SPI_EHCI2     = 76,
    S32G2_GIC_SPI_OHCI2     = 77,
    S32G2_GIC_SPI_EHCI3     = 78,
    S32G2_GIC_SPI_OHCI3     = 79,
    S32G2_GIC_SPI_EMAC      = 82
};

/* Allwinner H3 general constants */
enum {
    S32G2_GIC_NUM_SPI       = 128
};

void s32g2_bootrom_setup(S32G2State *s, BlockBackend *blk)
{
    const int64_t rom_size = 32 * KiB;
    const int64_t rom_offset = 0 * KiB;
    
    g_autofree uint8_t *buffer = g_new0(uint8_t, rom_size);

    if (blk_pread(blk, rom_offset, rom_size, buffer, 0) < 0) {
        error_setg(&error_fatal, "%s: failed to read BlockBackend data",
                   __func__);
        return;
    }

    uint32_t* ptr=(uint32_t*)buffer;
    for(int i=0; i < 100; i++)
    {
	    printf("%d: 0x%08x\n", i, ptr[i]);
    }
    rom_add_blob("s32g2.bootrom", buffer, rom_size,
                  rom_size, s->memmap[S32G2_DEV_SRAM_A1],
                  NULL, NULL, NULL, NULL, false);
}

static void s32g2_init(Object *obj)
{
    S32G2State *s = S32G2(obj);

    s->memmap = s32g2_memmap;

    for (int i = 0; i < S32G2_NUM_CPUS; i++) {
        object_initialize_child(obj, "cpu[*]", &s->cpus[i],
                                ARM_CPU_TYPE_NAME("cortex-a7"));
    }

    object_initialize_child(obj, "gic", &s->gic, TYPE_ARM_GIC);
#if 0
    object_initialize_child(obj, "timer", &s->timer, TYPE_AW_A10_PIT);
    object_property_add_alias(obj, "clk0-freq", OBJECT(&s->timer),
                              "clk0-freq");
    object_property_add_alias(obj, "clk1-freq", OBJECT(&s->timer),
                              "clk1-freq");

    object_initialize_child(obj, "ccu", &s->ccu, TYPE_S32G2_CCU);

    object_initialize_child(obj, "sysctrl", &s->sysctrl, TYPE_S32G2_SYSCTRL);

    object_initialize_child(obj, "cpucfg", &s->cpucfg, TYPE_AW_CPUCFG);

    object_initialize_child(obj, "sid", &s->sid, TYPE_AW_SID);
    object_property_add_alias(obj, "identifier", OBJECT(&s->sid),
                              "identifier");
#endif
    object_initialize_child(obj, "mmc0", &s->mmc0, TYPE_AW_SDHOST_SUN5I);

#if 0
    object_initialize_child(obj, "emac", &s->emac, TYPE_AW_SUN8I_EMAC);
    object_initialize_child(obj, "dramc", &s->dramc, TYPE_S32G2_DRAMC);
    object_property_add_alias(obj, "ram-addr", OBJECT(&s->dramc),
                             "ram-addr");
    object_property_add_alias(obj, "ram-size", OBJECT(&s->dramc),
                              "ram-size");
    object_initialize_child(obj, "rtc", &s->rtc, TYPE_AW_RTC_SUN6I);

    object_initialize_child(obj, "twi0",  &s->i2c0,  TYPE_AW_I2C_SUN6I);
    object_initialize_child(obj, "twi1",  &s->i2c1,  TYPE_AW_I2C_SUN6I);
    object_initialize_child(obj, "twi2",  &s->i2c2,  TYPE_AW_I2C_SUN6I);
    object_initialize_child(obj, "r_twi", &s->r_twi, TYPE_AW_I2C_SUN6I);
#endif
}

static void s32g2_realize(DeviceState *dev, Error **errp)
{
    S32G2State *s = S32G2(dev);
    unsigned i;

    /* CPUs */
    for (i = 0; i < S32G2_NUM_CPUS; i++) {

        /*
         * Disable secondary CPUs. Guest EL3 firmware will start
         * them via CPU reset control registers.
         */
        qdev_prop_set_bit(DEVICE(&s->cpus[i]), "start-powered-off",
                          i > 0);

        /* All exception levels required */
        qdev_prop_set_bit(DEVICE(&s->cpus[i]), "has_el3", true);
        qdev_prop_set_bit(DEVICE(&s->cpus[i]), "has_el2", true);

        /* Mark realized */
        qdev_realize(DEVICE(&s->cpus[i]), NULL, &error_fatal);
    }

    /* Generic Interrupt Controller */
    qdev_prop_set_uint32(DEVICE(&s->gic), "num-irq", S32G2_GIC_NUM_SPI +
                                                     GIC_INTERNAL);
    qdev_prop_set_uint32(DEVICE(&s->gic), "revision", 2);
    qdev_prop_set_uint32(DEVICE(&s->gic), "num-cpu", S32G2_NUM_CPUS);
    qdev_prop_set_bit(DEVICE(&s->gic), "has-security-extensions", false);
    qdev_prop_set_bit(DEVICE(&s->gic), "has-virtualization-extensions", true);
    sysbus_realize(SYS_BUS_DEVICE(&s->gic), &error_fatal);

    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gic), 0, s->memmap[S32G2_DEV_GIC_DIST]);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gic), 1, s->memmap[S32G2_DEV_GIC_CPU]);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gic), 2, s->memmap[S32G2_DEV_GIC_HYP]);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gic), 3, s->memmap[S32G2_DEV_GIC_VCPU]);

    /*
     * Wire the outputs from each CPU's generic timer and the GICv3
     * maintenance interrupt signal to the appropriate GIC PPI inputs,
     * and the GIC's IRQ/FIQ/VIRQ/VFIQ interrupt outputs to the CPU's inputs.
     */
    for (i = 0; i < S32G2_NUM_CPUS; i++) {
        DeviceState *cpudev = DEVICE(&s->cpus[i]);
        int ppibase = S32G2_GIC_NUM_SPI + i * GIC_INTERNAL + GIC_NR_SGIS;
        int irq;
        /*
         * Mapping from the output timer irq lines from the CPU to the
         * GIC PPI inputs used for this board.
         */
        const int timer_irq[] = {
            [GTIMER_PHYS] = S32G2_GIC_PPI_PHYSTIMER,
            [GTIMER_VIRT] = S32G2_GIC_PPI_VIRTTIMER,
            [GTIMER_HYP]  = S32G2_GIC_PPI_HYPTIMER,
            [GTIMER_SEC]  = S32G2_GIC_PPI_SECTIMER,
        };

        /* Connect CPU timer outputs to GIC PPI inputs */
        for (irq = 0; irq < ARRAY_SIZE(timer_irq); irq++) {
            qdev_connect_gpio_out(cpudev, irq,
                                  qdev_get_gpio_in(DEVICE(&s->gic),
                                                   ppibase + timer_irq[irq]));
        }

        /* Connect GIC outputs to CPU interrupt inputs */
        sysbus_connect_irq(SYS_BUS_DEVICE(&s->gic), i,
                           qdev_get_gpio_in(cpudev, ARM_CPU_IRQ));
        sysbus_connect_irq(SYS_BUS_DEVICE(&s->gic), i + S32G2_NUM_CPUS,
                           qdev_get_gpio_in(cpudev, ARM_CPU_FIQ));
        sysbus_connect_irq(SYS_BUS_DEVICE(&s->gic), i + (2 * S32G2_NUM_CPUS),
                           qdev_get_gpio_in(cpudev, ARM_CPU_VIRQ));
        sysbus_connect_irq(SYS_BUS_DEVICE(&s->gic), i + (3 * S32G2_NUM_CPUS),
                           qdev_get_gpio_in(cpudev, ARM_CPU_VFIQ));

        /* GIC maintenance signal */
        sysbus_connect_irq(SYS_BUS_DEVICE(&s->gic), i + (4 * S32G2_NUM_CPUS),
                           qdev_get_gpio_in(DEVICE(&s->gic),
                                            ppibase + S32G2_GIC_PPI_MAINT));
    }
#if 0
    /* Timer */
    sysbus_realize(SYS_BUS_DEVICE(&s->timer), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->timer), 0, s->memmap[S32G2_DEV_PIT]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->timer), 0,
                       qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_TIMER0));
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->timer), 1,
                       qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_TIMER1));
#endif
    /* SRAM */
    memory_region_init_ram(&s->sram_a1, OBJECT(dev), "sram A1",
                            64 * KiB, &error_abort);
    memory_region_init_ram(&s->sram_a2, OBJECT(dev), "sram A2",
                            32 * KiB, &error_abort);
    memory_region_init_ram(&s->sram_c, OBJECT(dev), "sram C",
                            44 * KiB, &error_abort);
    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_SRAM_A1],
                                &s->sram_a1);
    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_SRAM_A2],
                                &s->sram_a2);
    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_SRAM_C],
                                &s->sram_c);
#if 0
    /* Clock Control Unit */
    sysbus_realize(SYS_BUS_DEVICE(&s->ccu), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->ccu), 0, s->memmap[S32G2_DEV_CCU]);

    /* System Control */
    sysbus_realize(SYS_BUS_DEVICE(&s->sysctrl), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->sysctrl), 0, s->memmap[S32G2_DEV_SYSCTRL]);

    /* CPU Configuration */
    sysbus_realize(SYS_BUS_DEVICE(&s->cpucfg), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->cpucfg), 0, s->memmap[S32G2_DEV_CPUCFG]);

    /* Security Identifier */
    sysbus_realize(SYS_BUS_DEVICE(&s->sid), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->sid), 0, s->memmap[S32G2_DEV_SID]);

#endif
    /* SD/MMC */
    object_property_set_link(OBJECT(&s->mmc0), "dma-memory",
                             OBJECT(get_system_memory()), &error_fatal);
    sysbus_realize(SYS_BUS_DEVICE(&s->mmc0), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->mmc0), 0, s->memmap[S32G2_DEV_MMC0]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->mmc0), 0,
                       qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_MMC0));

    object_property_add_alias(OBJECT(s), "sd-bus", OBJECT(&s->mmc0),
                              "sd-bus");
#if 0
    /* EMAC */
    /* FIXME use qdev NIC properties instead of nd_table[] */
    if (nd_table[0].used) {
        qemu_check_nic_model(&nd_table[0], TYPE_AW_SUN8I_EMAC);
        qdev_set_nic_properties(DEVICE(&s->emac), &nd_table[0]);
    }
    object_property_set_link(OBJECT(&s->emac), "dma-memory",
                             OBJECT(get_system_memory()), &error_fatal);
    sysbus_realize(SYS_BUS_DEVICE(&s->emac), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->emac), 0, s->memmap[S32G2_DEV_EMAC]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->emac), 0,
                       qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_EMAC));

    /* Universal Serial Bus */
    sysbus_create_simple(TYPE_S32G2_EHCI, s->memmap[S32G2_DEV_EHCI0],
                         qdev_get_gpio_in(DEVICE(&s->gic),
                                          S32G2_GIC_SPI_EHCI0));
    sysbus_create_simple(TYPE_S32G2_EHCI, s->memmap[S32G2_DEV_EHCI1],
                         qdev_get_gpio_in(DEVICE(&s->gic),
                                          S32G2_GIC_SPI_EHCI1));
    sysbus_create_simple(TYPE_S32G2_EHCI, s->memmap[S32G2_DEV_EHCI2],
                         qdev_get_gpio_in(DEVICE(&s->gic),
                                          S32G2_GIC_SPI_EHCI2));
    sysbus_create_simple(TYPE_S32G2_EHCI, s->memmap[S32G2_DEV_EHCI3],
                         qdev_get_gpio_in(DEVICE(&s->gic),
                                          S32G2_GIC_SPI_EHCI3));

    sysbus_create_simple("sysbus-ohci", s->memmap[S32G2_DEV_OHCI0],
                         qdev_get_gpio_in(DEVICE(&s->gic),
                                          S32G2_GIC_SPI_OHCI0));
    sysbus_create_simple("sysbus-ohci", s->memmap[S32G2_DEV_OHCI1],
                         qdev_get_gpio_in(DEVICE(&s->gic),
                                          S32G2_GIC_SPI_OHCI1));
    sysbus_create_simple("sysbus-ohci", s->memmap[S32G2_DEV_OHCI2],
                         qdev_get_gpio_in(DEVICE(&s->gic),
                                          S32G2_GIC_SPI_OHCI2));
    sysbus_create_simple("sysbus-ohci", s->memmap[S32G2_DEV_OHCI3],
                         qdev_get_gpio_in(DEVICE(&s->gic),
                                          S32G2_GIC_SPI_OHCI3));
#endif
    /* UART0. For future clocktree API: All UARTS are connected to APB2_CLK. */
    serial_mm_init(get_system_memory(), s->memmap[S32G2_DEV_UART0], 2,
                   qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_UART0),
                   115200, serial_hd(0), DEVICE_NATIVE_ENDIAN);
    /* UART1 */
    serial_mm_init(get_system_memory(), s->memmap[S32G2_DEV_UART1], 2,
                   qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_UART1),
                   115200, serial_hd(1), DEVICE_NATIVE_ENDIAN);
    /* UART2 */
    serial_mm_init(get_system_memory(), s->memmap[S32G2_DEV_UART2], 2,
                   qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_UART2),
                   115200, serial_hd(2), DEVICE_NATIVE_ENDIAN);
    /* UART3 */
    serial_mm_init(get_system_memory(), s->memmap[S32G2_DEV_UART3], 2,
                   qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_UART3),
                   115200, serial_hd(3), DEVICE_NATIVE_ENDIAN);
#if 0
    /* DRAMC */
    sysbus_realize(SYS_BUS_DEVICE(&s->dramc), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->dramc), 0, s->memmap[S32G2_DEV_DRAMCOM]);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->dramc), 1, s->memmap[S32G2_DEV_DRAMCTL]);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->dramc), 2, s->memmap[S32G2_DEV_DRAMPHY]);

    /* RTC */
    sysbus_realize(SYS_BUS_DEVICE(&s->rtc), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->rtc), 0, s->memmap[S32G2_DEV_RTC]);

    /* I2C */
    sysbus_realize(SYS_BUS_DEVICE(&s->i2c0), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->i2c0), 0, s->memmap[S32G2_DEV_TWI0]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->i2c0), 0,
                       qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_TWI0));

    sysbus_realize(SYS_BUS_DEVICE(&s->i2c1), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->i2c1), 0, s->memmap[S32G2_DEV_TWI1]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->i2c1), 0,
                       qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_TWI1));

    sysbus_realize(SYS_BUS_DEVICE(&s->i2c2), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->i2c2), 0, s->memmap[S32G2_DEV_TWI2]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->i2c2), 0,
                       qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_TWI2));

    sysbus_realize(SYS_BUS_DEVICE(&s->r_twi), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->r_twi), 0, s->memmap[S32G2_DEV_R_TWI]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->r_twi), 0,
                       qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_R_TWI));
    /* Unimplemented devices */
    for (i = 0; i < ARRAY_SIZE(unimplemented); i++) {
        create_unimplemented_device(unimplemented[i].device_name,
                                    unimplemented[i].base,
                                    unimplemented[i].size);
    }
#endif
}

static void s32g2_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->realize = s32g2_realize;
    /* Reason: uses serial_hd() in realize function */
    dc->user_creatable = false;
}

static const TypeInfo s32g2_type_info = {
    .name = TYPE_S32G2,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(S32G2State),
    .instance_init = s32g2_init,
    .class_init = s32g2_class_init,
};

static void s32g2_register_types(void)
{
    type_register_static(&s32g2_type_info);
}

type_init(s32g2_register_types)
