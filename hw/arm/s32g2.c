/*
 * S32g2 emulation
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
#if 0
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
#endif
    [S32G2_DEV_QSPI]  = 0x00000000,
    [S32G2_DEV_SRAM]  = 0x24000000,
    [S32G2_DEV_SSRAM] = 0x34000000,
    [S32G2_DEV_SRAM_CTRL_C0] = 0x4019C000,
    [S32G2_DEV_SRAM_C0] = 0x4019C014,
    [S32G2_DEV_SRAM_CTRL_C1] = 0x401A0000,
    [S32G2_DEV_SRAM_C1] = 0x401A0014,
    [S32G2_DEV_SPI0]  = 0x401d4000,
    [S32G2_DEV_SPI1]  = 0x401d8000,
    [S32G2_DEV_SPI2]  = 0x401dc000,
    [S32G2_DEV_I2C0]  = 0x401E4000,
    [S32G2_DEV_I2C1]  = 0x401E8000,
    [S32G2_DEV_I2C2]  = 0x401Ec000,
    [S32G2_DEV_GIC_DIST]   = 0x50801000,
    [S32G2_DEV_GIC_CPU]    = 0x50802000,
    [S32G2_DEV_GIC_HYP]    = 0x50804000,
    [S32G2_DEV_GIC_VCPU]   = 0x50806000,
    [S32G2_DEV_DRAM]  = 0x80000000

};

/* List of unimplemented devices */
struct S32G2Unimplemented {
    const char *device_name;
    hwaddr base;
    hwaddr size;
} s32g_unimplemented[] = {
#if 0
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
/*    { "ddr-mem",   0x40000000, 2 * GiB }, */
    { "n-brom",    0xffff0000, 32 * KiB },
    { "s-brom",    0xffff0000, 64 * KiB }
#endif
    { "concerto",  0x50400000, 1 * MiB },
    { "MC_RGM",    0x40078000, 12 * KiB },
    { "MC_ME",     0x40088000, 12 * KiB },
    { "SIUL2_0",   0x4009C000, 20 * KiB }

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


typedef struct s32g2_boot_cfg
{
    uint8_t sec_boot :1;
    uint8_t watchdog :1;
#define S32G2_CORTEX_M7   0
#define S32G2_CORTEX_A53  1
    uint8_t boot_target :2;
} s32g2_boot_cfg_t;

typedef struct s32g2_ivt
{
    uint32_t header;
    uint32_t selftest_addr;
    uint32_t selftest_bk_addr;
    uint32_t dcd_addr;
    uint32_t dcd_bk_addr;
    uint32_t fw_start_addr;
    uint32_t fw_start_bk_addr;
    uint32_t app_start_addr;
    uint32_t app_start_bk_addr;
    uint32_t boot_config;
    uint32_t life_cycle_config;
    uint32_t gmac;
} s32g2_ivt_t;

typedef struct s32g2_app_img
{
    uint32_t header;
    uint32_t ram_start;
    uint32_t ram_entry;
    uint32_t length;
} s32g2_app_img_t;

static s32g2_ivt_t s32g2_ivt = {0};
static s32g2_boot_cfg_t s32g2_boot_cfg = {0};
static s32g2_app_img_t s32g2_app_img = {0};
void s32g2_bootrom_setup(S32G2State *s, BlockBackend *blk, hwaddr* code_entry)
{
    const int64_t rom_size = 512 * KiB;
    const int64_t rom_offset = 0 * KiB;
    uint32_t boot_offset = 0;
    
    g_autofree uint8_t *buffer = g_new0(uint8_t, rom_size);

    if (blk_pread(blk, rom_offset, rom_size, buffer, 0) < 0) {
        error_setg(&error_fatal, "%s: failed to read BlockBackend data",
                   __func__);
        return;
    }

    uint32_t* ptr=(uint32_t*)buffer;
    s32g2_ivt.header = ptr[0];
    s32g2_ivt.selftest_addr = ptr[2];
    s32g2_ivt.selftest_bk_addr = ptr[3];
    s32g2_ivt.dcd_addr = ptr[4];
    s32g2_ivt.dcd_bk_addr = ptr[5];
    s32g2_ivt.fw_start_addr = ptr[6];
    s32g2_ivt.fw_start_bk_addr = ptr[7];
    s32g2_ivt.app_start_addr = ptr[8];
    s32g2_ivt.app_start_bk_addr = ptr[9];
    s32g2_ivt.boot_config = ptr[10];
    s32g2_ivt.life_cycle_config = ptr[11];
    s32g2_ivt.gmac = ptr[0xF0/4];


    printf("header=0x%08x\n", s32g2_ivt.header);
    printf("ST=0x%08x\n", s32g2_ivt.selftest_addr);
    printf("ST bk=0x%08x\n", s32g2_ivt.selftest_bk_addr);
    printf("DCD=0x%08x\n", s32g2_ivt.dcd_addr);
    printf("DCD bk=0x%08x\n", s32g2_ivt.dcd_bk_addr);
    printf("FW=0x%08x\n", s32g2_ivt.fw_start_addr);
    printf("FW bk=0x%08x\n", s32g2_ivt.fw_start_bk_addr);
    printf("App=0x%08x\n", s32g2_ivt.app_start_addr);
    printf("App bk=0x%08x\n", s32g2_ivt.app_start_bk_addr);
    printf("Boot=0x%08x\n", s32g2_ivt.boot_config);
    printf("LC=0x%08x\n", s32g2_ivt.life_cycle_config);
    printf("GMAC=0x%08x\n", s32g2_ivt.gmac);

    s32g2_boot_cfg.sec_boot = (s32g2_ivt.boot_config & 0x00000008) >> 3;
    s32g2_boot_cfg.watchdog = (s32g2_ivt.boot_config & 0x00000008) >> 2;
    s32g2_boot_cfg.boot_target = (s32g2_ivt.boot_config & 0x00000003);

    printf("sec boot=0x%08x\n", s32g2_boot_cfg.sec_boot); 
    printf("watchdog=0x%08x\n", s32g2_boot_cfg.watchdog); 
    printf("boottarget=%s\n", ((s32g2_boot_cfg.boot_target)==S32G2_CORTEX_A53)? "Cortex-A53" : "Cortex-M7" );


    if(s32g2_boot_cfg.sec_boot == 0)
    {
        boot_offset = s32g2_ivt.app_start_addr;
    }
    else
    {
        boot_offset = s32g2_ivt.fw_start_addr;
    }

    ptr=(uint32_t*)&buffer[boot_offset];
    s32g2_app_img.header=ptr[0];
    s32g2_app_img.ram_start=ptr[1];
    s32g2_app_img.ram_entry=ptr[2];
    s32g2_app_img.length=ptr[3];

    uint8_t* app_code=(uint8_t*)&ptr[0x40 / 4];
    uint32_t* app_code_curr = (uint32_t*)app_code;

    printf("App header=0x%08x\n", s32g2_app_img.header);
    printf("App ram start=0x%08x\n", s32g2_app_img.ram_start);
    printf("App ram entry=0x%08x\n", s32g2_app_img.ram_entry);
    printf("App length=0x%08x\n", s32g2_app_img.length);
    printf("First code=0x%08x\n", *app_code_curr);

    *code_entry = s32g2_app_img.ram_entry;
    uint32_t entry_offset = (s32g2_app_img.ram_entry - s32g2_app_img.ram_start);
    
    printf("Entry offset=0x%08x\n", entry_offset + boot_offset + 0x40 );

    app_code += entry_offset;
    app_code_curr = (uint32_t*)app_code; 
    printf("First entry code=0x%08x\n", *app_code_curr);
#if 0
    for(int i=0; i < 100; i++)
    {
	    if( ptr[i] != 0 )
		    printf("%d: 0x%08x\n", i, ptr[i]);
    }
#endif
    rom_add_blob("s32g2.bootrom", app_code, s32g2_app_img.length,
                  s32g2_app_img.length, s32g2_app_img.ram_entry,
                  NULL, NULL, NULL, NULL, false);
}

static void s32g2_init(Object *obj)
{
    S32G2State *s = S32G2(obj);

    s->memmap = s32g2_memmap;

    for (int i = 0; i < S32G2_NUM_CPUS; i++) {
        object_initialize_child(obj, "cpu[*]", &s->cpus[i],
                                ARM_CPU_TYPE_NAME("cortex-a53"));
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
    object_initialize_child(obj, "sram_ctrl_c0", &s->sram_ctrl_c0, TYPE_S32G2_SRAMC);
    object_initialize_child(obj, "sram_ctrl_c1", &s->sram_ctrl_c1, TYPE_S32G2_SRAMC);

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
    sysbus_realize(SYS_BUS_DEVICE(&s->sram_ctrl_c0), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->sram_ctrl_c0), 0, s->memmap[S32G2_DEV_SRAM_CTRL_C0]);

    sysbus_realize(SYS_BUS_DEVICE(&s->sram_ctrl_c1), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->sram_ctrl_c1), 0, s->memmap[S32G2_DEV_SRAM_CTRL_C1]);

    memory_region_init_ram(&s->sram_a1, OBJECT(dev), "sram",
                            32 * KiB, &error_abort);
    memory_region_init_ram(&s->sram_c0, OBJECT(dev), "sram_c0",
                            12 * KiB, &error_abort);
    memory_region_init_ram(&s->sram_c1, OBJECT(dev), "sram_c1",
                            12 * KiB, &error_abort);
    memory_region_init_ram(&s->sram_a2, OBJECT(dev), "ssram",
                            8 * MiB, &error_abort);
    memory_region_init_ram(&s->sram_c, OBJECT(dev), "dram",
                            1 * GiB, &error_abort);
    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_SRAM],
                                &s->sram_a1);
    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_SSRAM],
                                &s->sram_a2);
    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_DRAM],
                                &s->sram_c);

    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_SRAM_C0],
                                &s->sram_c0);
    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_SRAM_C1],
                                &s->sram_c1);
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
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->mmc0), 0, s->memmap[S32G2_DEV_QSPI]);
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
#endif
    /* Unimplemented devices */
    for (i = 0; i < ARRAY_SIZE(s32g_unimplemented); i++) {
        create_unimplemented_device(s32g_unimplemented[i].device_name,
                                    s32g_unimplemented[i].base,
                                    s32g_unimplemented[i].size);
    }
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
