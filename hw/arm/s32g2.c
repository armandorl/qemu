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
#include "hw/spi/spi.h"

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
    [S32G2_DEV_QSPI_BUFFER]  = 0x00000000,
    [S32G2_DEV_SRAM]  = 0x24000000,
    [S32G2_DEV_SSRAM] = 0x34000000,
    [S32G2_DEV_SRAM_CTRL_C0] = 0x4019C000,
    [S32G2_DEV_SRAM_C0] = 0x4019C014,
    [S32G2_DEV_SRAM_CTRL_C1] = 0x401A0000,
    [S32G2_DEV_SRAM_C1] = 0x401A0014,
    [S32G2_DEV_SRAM_STDBY] = 0x44028014,
    [S32G2_DEV_PIT]   = 0x40188000,
    [S32G2_DEV_SPI0]  = 0x401d4000,
    [S32G2_DEV_SPI1]  = 0x401d8000,
    [S32G2_DEV_SPI2]  = 0x401dc000,
    [S32G2_DEV_I2C0]  = 0x401E4000,
    [S32G2_DEV_I2C1]  = 0x401E8000,
    [S32G2_DEV_I2C2]  = 0x401Ec000,
    [S32G2_DEV_ADC0]  = 0x401F8000,
    [S32G2_DEV_MC_CGM]  = 0x40030000,
    [S32G2_DEV_MC_CGM1]  = 0x40034000,
    [S32G2_DEV_MC_CGM5]  = 0x40068000,
    [S32G2_DEV_PLL]     = 0x40038000,
    [S32G2_DEV_PERIPH_PLL]     = 0x4003C000,
    [S32G2_DEV_ACC_PLL]     = 0x40040000,
    [S32G2_DEV_DDR_PLL]     = 0x40044000,
    [S32G2_DEV_XOSC]    = 0x40050000,
    [S32G2_DEV_DFS]    = 0x40054000,
    [S32G2_DEV_PERIPH_DFS]    = 0x40058000,
    [S32G2_DEV_RTC]     = 0x40060000,
    [S32G2_DEV_MC_RGM]  = 0x40078000,
    [S32G2_DEV_SRC]    = 0x4007C000,
    [S32G2_DEV_RDC]    = 0x40080000,
    [S32G2_DEV_MC_ME]  = 0x40088000,
    [S32G2_DEV_WKPU]  =  0x40090000,
    [S32G2_DEV_SIUL2]  =  0x4009C000,
    [S32G2_DEV_QSPI_REGS]  =  0x40134000,
    [S32G2_DEV_SERDES0]  =  0x40480000,
    [S32G2_DEV_LINFLEX0]   = 0x401C8000,
    [S32G2_DEV_LINFLEX1]   = 0x401CC000,
    [S32G2_DEV_HSEMU0]     = 0x40210000,
    [S32G2_DEV_HSEMU1]     = 0x40211000,
    [S32G2_DEV_HSEMU2]     = 0x40212000,
    [S32G2_DEV_HSEMU3]     = 0x40213000,
    [S32G2_DEV_LINFLEX2]   = 0x402BC000,
    [S32G2_DEV_ADC1]       = 0x402E8000,
    [S32G2_DEV_DDRPHY]     = 0x40380000,
    [S32G2_DEV_DDRSS]      = 0x403C0000,
    [S32G2_DEV_SIUL2_1]  =  0x44010000,
    [S32G2_DEV_MC_CGM2]  =  0x44018000,
    [S32G2_DEV_SRAM_CTRL_STDBY] = 0x44028000,
    [S32G2_DEV_SERDES1]  =  0x44180000,
    [S32G2_DEV_GIC_DIST]   = 0x50800000,
    [S32G2_DEV_GIC_RDIST0]   = 0x50880000,
    [S32G2_DEV_GIC_RDIST1]   = 0x508A0000,
    [S32G2_DEV_GIC_RDIST2]   = 0x508C0000,
    [S32G2_DEV_GIC_RDIST3]   = 0x508E0000,
    [S32G2_DEV_GIC_CPU]    = 0x50400000,
    [S32G2_DEV_GIC_HYP]    = 0x50410000,
    [S32G2_DEV_GIC_VCPU]   = 0x50420000,
    [S32G2_DEV_DRAM]  = 0x80000000,
    [S32G2_DEV_DRAM2]  = 0x0880000000

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
#if 0
    { "PLLAcc",    0x40040000, 12 * KiB },
    { "RTC",       0x40060000, 4 * KiB },
#endif
    { "OCOTP",     0x400A4000, 4 * KiB }, /* On chip One Time Programmable - eFuses*/
#if 0
    { "STM0",      0x40110000, 12 * KiB },
    { "STM1",      0x40120000, 12 * KiB },
    { "STM2",      0x40124000, 12 * KiB },
    { "STM3",      0x40128000, 12 * KiB },
#endif
    { "DMAMUX0",   0x4012C000, 12 * KiB },
    { "DMAMUX1",   0x40130000, 12 * KiB },
    { "EDMA0",     0x40144000, 12 * KiB },
    { "EDMA0CHAN", 0x40148000, 128 * KiB },
    { "CAN0",      0x401B4000, 40 * KiB },
    { "CAN1",      0x401BE000, 40 * KiB },
    { "CAN2",      0x402A8000, 40 * KiB },
    { "CAN3",      0x402B2000, 40 * KiB },
#if 0
    { "SPI1",      0x401D8000, 12 * KiB },
#endif
    { "PWM0",      0x401F4000, 12 * KiB },
    { "ADC_0",     0x401F8000,  4 * KiB },
    { "PWM1",      0x402E4000, 12 * KiB },
#if 0
    { "HSE",       0x40210000, 16 * KiB },
#endif
#if 0
    { "MC_CGM2",   0x44018000, 12 * KiB },
    { "MC_CGM5",   0x40068000, 12 * KiB },
#endif
    { "DMAMUX2",   0x4022C000, 12 * KiB },
    { "DMAMUX3",   0x40230000, 12 * KiB },
    { "EDMA1",     0x40244000, 12 * KiB },
    { "EDMA1CHAN", 0x40248000, 128 * KiB },
    { "I2C4",      0x402DC000, 4 * KiB },
    { "ADC_1",     0x402E8000, 4 * KiB },
    { "FCCU",      0x4030C000, 12 * KiB }, /* Fault collection and control unit */
#if 0
    { "SRC",       0x4007C000, 12 * KiB }, /* Src control registers (i.e. OS timer tick source)*/
#endif
    { "LLCE",      0x43000000, 16 * MiB }, /* Low Latency communication engine (FIFO for LinFlex SPI FlexRay SPI) */
    { "DDRSS1",    0x40390000, 0x20000 },
    { "DDRSS2",    0x403A0000, 0x20000 },
    { "DDRSS3",    0x403D0000, 0x20000 },
    { "SERDES0",   0x40400000, 1 * MiB },
    { "LLCE",      0x43000000, 16 * MiB }

};

/* Per Processor Interrupts */
enum {
    S32G2_GIC_PPI_PMU       = 23,
    S32G2_GIC_PPI_MAINT     = 25,
    S32G2_GIC_PPI_HYPTIMER  = 26,
    S32G2_GIC_PPI_VIRTTIMER = 27,
    S32G2_GIC_PPI_EL2_VIRTTIMER  = 28,
    S32G2_GIC_PPI_EL3_VIRTTIMER = 29,
    S32G2_GIC_PPI_PHYSTIMER = 30
};

/* Shared Processor Interrupts */
enum {
    S32G2_GIC_SPI_UART0     =  114 - GIC_INTERNAL,
    S32G2_GIC_SPI_UART1     =  115 - GIC_INTERNAL,
    S32G2_GIC_SPI_UART2     =  116 - GIC_INTERNAL,
    S32G2_GIC_SPI_SPI0      =  117 - GIC_INTERNAL,
    S32G2_GIC_SPI_SPI1      =  118 - GIC_INTERNAL,
#if 1
    S32G2_GIC_SPI_TIMER0    = 101 - GIC_INTERNAL,
    S32G2_GIC_SPI_TIMER1    = 102 - GIC_INTERNAL,
#else
    S32G2_GIC_SPI_TIMER0    = 85 - GIC_INTERNAL,
    S32G2_GIC_SPI_TIMER1    = 86 - GIC_INTERNAL,
#endif
    S32G2_GIC_SIUL2_1       = 242 - GIC_INTERNAL,
    S32G2_GIC_SPI_MMC0      = 246 - GIC_INTERNAL
};

/* General constants */
enum {
    S32G2_GIC_NUM_SPI       = 480
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
    const int64_t rom_size = 64 * MiB;
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

    /* app_code += entry_offset;
    app_code_curr = (uint32_t*)app_code; 
    printf("First entry code=0x%08x\n", *app_code_curr); */
#if 0
    for(int i=0; i < 100; i++)
    {
	    if( ptr[i] != 0 )
		    printf("%d: 0x%08x\n", i, ptr[i]);
    }
#endif
    rom_add_blob("s32g2.bootrom", app_code, s32g2_app_img.length + entry_offset,
                  s32g2_app_img.length + entry_offset, s32g2_app_img.ram_start,
                  NULL, NULL, NULL, NULL, false);
    rom_add_blob("qspi.bootrom", buffer, rom_size,
                  rom_size, 0,
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

    object_initialize_child(obj, "gic", &s->gic, TYPE_ARM_GICV3);
    object_initialize_child(obj, "timer", &s->timer, TYPE_S32G2_PIT);
#if 0
    object_property_add_alias(obj, "clk0-freq", OBJECT(&s->timer),
                              "clk0-freq");
    object_property_add_alias(obj, "clk1-freq", OBJECT(&s->timer),
                              "clk1-freq");
#endif
#if 0
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
    object_initialize_child(obj, "sram_ctrl_stdby", &s->sram_ctrl_stdby, TYPE_S32G2_SRAMC);
    object_initialize_child(obj, "mc_cgm", &s->mc_cgm, TYPE_S32G2_MC_CGM);
    object_initialize_child(obj, "mc_cgm1", &s->mc_cgm1, TYPE_S32G2_MC_CGM1);
#if 1
    object_initialize_child(obj, "mc_cgm5", &s->mc_cgm5, TYPE_S32G2_MC_CGM5);
#endif
    object_initialize_child(obj, "mc_rgm", &s->mc_rgm, TYPE_S32G2_MC_RGM);
    object_initialize_child(obj, "mc_me", &s->mc_me, TYPE_S32G2_MC_ME);
    object_initialize_child(obj, "wkpu", &s->wkpu, TYPE_S32G2_WKPU);
    object_initialize_child(obj, "xosc", &s->xosc, TYPE_S32G2_XOSC);
    object_initialize_child(obj, "adc0", &s->adc0, TYPE_S32G2_ADC);
    object_initialize_child(obj, "adc1", &s->adc1, TYPE_S32G2_ADC);
    object_initialize_child(obj, "dfs", &s->dfs, TYPE_S32G2_DFS);
    object_initialize_child(obj, "periph_dfs", &s->periph_dfs, TYPE_S32G2_DFS);
    object_initialize_child(obj, "siul2", &s->siul2, TYPE_S32G2_SIUL2);
    object_initialize_child(obj, "siul2_1", &s->siul2_1, TYPE_S32G2_SIUL2_1);
    object_initialize_child(obj, "pll", &s->pll, TYPE_S32G2_PLL);
    object_initialize_child(obj, "qspi", &s->qspi, TYPE_S32G2_QSPI);
    object_initialize_child(obj, "spi0", &s->spi0, TYPE_S32G2_SPI);
    object_initialize_child(obj, "spi1", &s->spi1, TYPE_S32G2_SPI);
    object_initialize_child(obj, "spi2", &s->spi2, TYPE_S32G2_SPI);
    object_initialize_child(obj, "spi3", &s->spi3, TYPE_S32G2_SPI);
    object_initialize_child(obj, "spi4", &s->spi4, TYPE_S32G2_SPI);
    object_initialize_child(obj, "spi5", &s->spi5, TYPE_S32G2_SPI);
    object_initialize_child(obj, "serdes0", &s->serdes0, TYPE_S32G2_SERDES);
    object_initialize_child(obj, "serdes1", &s->serdes1, TYPE_S32G2_SERDES);
    object_initialize_child(obj, "periph_pll", &s->periph_pll, TYPE_S32G2_PLL);
    object_initialize_child(obj, "ddr_pll", &s->ddr_pll, TYPE_S32G2_PLL);
    object_initialize_child(obj, "acc_pll", &s->acc_pll, TYPE_S32G2_PLL);
    object_initialize_child(obj, "src", &s->src, TYPE_S32G2_SRC);
    object_initialize_child(obj, "rdc", &s->rdc, TYPE_S32G2_RDC);
    object_initialize_child(obj, "hsemu0", &s->hsemu0, TYPE_S32G2_HSEMU);
    object_initialize_child(obj, "hsemu1", &s->hsemu1, TYPE_S32G2_HSEMU);
    object_initialize_child(obj, "hsemu2", &s->hsemu2, TYPE_S32G2_HSEMU);
    object_initialize_child(obj, "hsemu3", &s->hsemu3, TYPE_S32G2_HSEMU);
    object_initialize_child(obj, "linflex0", &s->linflex0, TYPE_S32G2_LINFLEX);
    object_initialize_child(obj, "linflex1", &s->linflex1, TYPE_S32G2_LINFLEX);
    object_initialize_child(obj, "linflex2", &s->linflex2, TYPE_S32G2_LINFLEX);
    object_initialize_child(obj, "ddrss", &s->ddrss, TYPE_S32G2_DDRSS);
    object_initialize_child(obj, "ddrphy", &s->ddrphy, TYPE_S32G2_DDRPHY);

#if 1
    object_initialize_child(obj, "mc_cgm2", &s->mc_cgm2, TYPE_S32G2_MC_CGM2);
#endif
#if 0
    object_initialize_child(obj, "emac", &s->emac, TYPE_AW_SUN8I_EMAC);
    object_initialize_child(obj, "dramc", &s->dramc, TYPE_S32G2_DRAMC);
    object_property_add_alias(obj, "ram-addr", OBJECT(&s->dramc),
                             "ram-addr");
    object_property_add_alias(obj, "ram-size", OBJECT(&s->dramc),
                              "ram-size");
#endif
    object_initialize_child(obj, "rtc", &s->rtc, TYPE_S32G2_RTC);
#if 0
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
    qdev_prop_set_uint32(DEVICE(&s->gic), "num-irq", S32G2_GIC_NUM_SPI + GIC_INTERNAL);
    qdev_prop_set_uint32(DEVICE(&s->gic), "revision", 3);
    qdev_prop_set_uint32(DEVICE(&s->gic), "num-cpu", S32G2_NUM_CPUS);
    qdev_prop_set_uint32(DEVICE(&s->gic), "len-redist-region-count", 1);
    qdev_prop_set_uint32(DEVICE(&s->gic), "redist-region-count[0]", 4);
#if 0
    qdev_prop_set_uint32(DEVICE(&s->gic), "redist-region-count[1]", 2);
    qdev_prop_set_uint32(DEVICE(&s->gic), "redist-region-count[2]", 1);
    qdev_prop_set_uint32(DEVICE(&s->gic), "redist-region-count[3]", 1);
#endif
    qdev_prop_set_bit(DEVICE(&s->gic), "has-security-extensions", true);
    sysbus_realize(SYS_BUS_DEVICE(&s->gic), &error_fatal);

    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gic), 0, s->memmap[S32G2_DEV_GIC_DIST]);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gic), 1, s->memmap[S32G2_DEV_GIC_RDIST0]);
#if 0
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gic), 2, s->memmap[S32G2_DEV_GIC_RDIST1]);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gic), 3, s->memmap[S32G2_DEV_GIC_RDIST2]);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->gic), 4, s->memmap[S32G2_DEV_GIC_RDIST3]);
#endif

    /*
     * Wire the outputs from each CPU's generic timer and the GICv3
     * maintenance interrupt signal to the appropriate GIC PPI inputs,
     * and the GIC's IRQ/FIQ/VIRQ/VFIQ interrupt outputs to the CPU's inputs.
     */
    for (i = 0; i < S32G2_NUM_CPUS; i++) {
        DeviceState *cpudev = DEVICE(&s->cpus[i]);
	int ppibase = S32G2_GIC_NUM_SPI + (i * GIC_INTERNAL);
        int irq;
        /*
         * Mapping from the output timer irq lines from the CPU to the
         * GIC PPI inputs used for this board.
         */
        const int timer_irq[] = {
            [GTIMER_PHYS] = S32G2_GIC_PPI_PHYSTIMER,
            [GTIMER_VIRT] = S32G2_GIC_PPI_VIRTTIMER,
            [GTIMER_HYP]  = S32G2_GIC_PPI_HYPTIMER,
            [GTIMER_SEC]  = S32G2_GIC_PPI_EL3_VIRTTIMER,
            [GTIMER_HYPVIRT]  = S32G2_GIC_PPI_EL2_VIRTTIMER
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
        qemu_irq maint_irq;
        maint_irq = qdev_get_gpio_in(DEVICE(&s->gic),
                                        ppibase + S32G2_GIC_PPI_MAINT);
        qdev_connect_gpio_out_named(cpudev, "gicv3-maintenance-interrupt",
                                    0, maint_irq);

        qdev_connect_gpio_out_named(cpudev, "pmu-interrupt",
                                    0, qdev_get_gpio_in(DEVICE(&s->gic), ppibase + S32G2_GIC_PPI_PMU));
    }
    /* Timer */
    sysbus_realize(SYS_BUS_DEVICE(&s->timer), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->timer), 0, s->memmap[S32G2_DEV_PIT]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->timer), 0,
                       qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_TIMER0));
#if 0
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->timer), 1,
                       qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_TIMER1));
#endif
    /* SRAM */
    sysbus_realize(SYS_BUS_DEVICE(&s->sram_ctrl_c0), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->sram_ctrl_c0), 0, s->memmap[S32G2_DEV_SRAM_CTRL_C0]);

    sysbus_realize(SYS_BUS_DEVICE(&s->sram_ctrl_c1), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->sram_ctrl_c1), 0, s->memmap[S32G2_DEV_SRAM_CTRL_C1]);

    sysbus_realize(SYS_BUS_DEVICE(&s->sram_ctrl_stdby), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->sram_ctrl_stdby), 0, s->memmap[S32G2_DEV_SRAM_CTRL_STDBY]);

    sysbus_realize(SYS_BUS_DEVICE(&s->mc_cgm), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->mc_cgm), 0, s->memmap[S32G2_DEV_MC_CGM]);

    sysbus_realize(SYS_BUS_DEVICE(&s->mc_cgm1), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->mc_cgm1), 0, s->memmap[S32G2_DEV_MC_CGM1]);
#if 1
    sysbus_realize(SYS_BUS_DEVICE(&s->mc_cgm5), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->mc_cgm5), 0, s->memmap[S32G2_DEV_MC_CGM5]);
#endif
    sysbus_realize(SYS_BUS_DEVICE(&s->xosc), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->xosc), 0, s->memmap[S32G2_DEV_XOSC]);

    sysbus_realize(SYS_BUS_DEVICE(&s->adc0), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->adc0), 0, s->memmap[S32G2_DEV_ADC0]);

    sysbus_realize(SYS_BUS_DEVICE(&s->adc1), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->adc1), 0, s->memmap[S32G2_DEV_ADC1]);

    sysbus_realize(SYS_BUS_DEVICE(&s->dfs), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->dfs), 0, s->memmap[S32G2_DEV_DFS]);

    sysbus_realize(SYS_BUS_DEVICE(&s->periph_dfs), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->periph_dfs), 0, s->memmap[S32G2_DEV_PERIPH_DFS]);

    sysbus_realize(SYS_BUS_DEVICE(&s->siul2), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->siul2), 0, s->memmap[S32G2_DEV_SIUL2]);

    sysbus_realize(SYS_BUS_DEVICE(&s->siul2_1), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->siul2_1), 0, s->memmap[S32G2_DEV_SIUL2_1]);

    sysbus_realize(SYS_BUS_DEVICE(&s->qspi), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->qspi), 0, s->memmap[S32G2_DEV_QSPI_REGS]);

    sysbus_realize(SYS_BUS_DEVICE(&s->spi0), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->spi0), 0, s->memmap[S32G2_DEV_SPI0]);

    object_property_add_alias(OBJECT(s), "spi-bus", OBJECT(&s->spi0),
                              "spi-bus");

    sysbus_realize(SYS_BUS_DEVICE(&s->spi1), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->spi1), 0, s->memmap[S32G2_DEV_SPI1]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->spi1), 0,
                       qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_SPI1));

    sysbus_realize(SYS_BUS_DEVICE(&s->spi2), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->spi2), 0, s->memmap[S32G2_DEV_SPI2]);

    sysbus_realize(SYS_BUS_DEVICE(&s->spi3), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->spi3), 0, s->memmap[S32G2_DEV_SPI3]);

    sysbus_realize(SYS_BUS_DEVICE(&s->spi4), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->spi4), 0, s->memmap[S32G2_DEV_SPI4]);

    sysbus_realize(SYS_BUS_DEVICE(&s->spi5), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->spi5), 0, s->memmap[S32G2_DEV_SPI5]);

    sysbus_realize(SYS_BUS_DEVICE(&s->serdes0), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->serdes0), 0, s->memmap[S32G2_DEV_SERDES0]);

    sysbus_realize(SYS_BUS_DEVICE(&s->serdes1), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->serdes1), 0, s->memmap[S32G2_DEV_SERDES1]);

    sysbus_realize(SYS_BUS_DEVICE(&s->pll), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->pll), 0, s->memmap[S32G2_DEV_PLL]);

    sysbus_realize(SYS_BUS_DEVICE(&s->periph_pll), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->periph_pll), 0, s->memmap[S32G2_DEV_PERIPH_PLL]);

    sysbus_realize(SYS_BUS_DEVICE(&s->ddr_pll), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->ddr_pll), 0, s->memmap[S32G2_DEV_DDR_PLL]);

    sysbus_realize(SYS_BUS_DEVICE(&s->acc_pll), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->acc_pll), 0, s->memmap[S32G2_DEV_ACC_PLL]);

    sysbus_realize(SYS_BUS_DEVICE(&s->mc_rgm), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->mc_rgm), 0, s->memmap[S32G2_DEV_MC_RGM]);

    sysbus_realize(SYS_BUS_DEVICE(&s->mc_me), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->mc_me), 0, s->memmap[S32G2_DEV_MC_ME]);

    sysbus_realize(SYS_BUS_DEVICE(&s->wkpu), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->wkpu), 0, s->memmap[S32G2_DEV_WKPU]);

    sysbus_realize(SYS_BUS_DEVICE(&s->src), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->src), 0, s->memmap[S32G2_DEV_SRC]);

    sysbus_realize(SYS_BUS_DEVICE(&s->rdc), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->rdc), 0, s->memmap[S32G2_DEV_RDC]);

    sysbus_realize(SYS_BUS_DEVICE(&s->hsemu0), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->hsemu0), 0, s->memmap[S32G2_DEV_HSEMU0]);

    sysbus_realize(SYS_BUS_DEVICE(&s->hsemu1), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->hsemu1), 0, s->memmap[S32G2_DEV_HSEMU1]);

    sysbus_realize(SYS_BUS_DEVICE(&s->hsemu2), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->hsemu2), 0, s->memmap[S32G2_DEV_HSEMU2]);

    sysbus_realize(SYS_BUS_DEVICE(&s->hsemu3), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->hsemu3), 0, s->memmap[S32G2_DEV_HSEMU3]);

    s32g2_linflex_props_init(&s->linflex0,
                   qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_UART0),
                   serial_hd(0));

    sysbus_realize(SYS_BUS_DEVICE(&s->linflex0), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->linflex0), 0, s->memmap[S32G2_DEV_LINFLEX0]);

#if 1
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->linflex0), 0,
                       qdev_get_gpio_in(DEVICE(&s->gic), S32G2_GIC_SPI_UART0));
#endif

    sysbus_realize(SYS_BUS_DEVICE(&s->linflex1), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->linflex1), 0, s->memmap[S32G2_DEV_LINFLEX1]);

    sysbus_realize(SYS_BUS_DEVICE(&s->linflex2), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->linflex2), 0, s->memmap[S32G2_DEV_LINFLEX2]);

    sysbus_realize(SYS_BUS_DEVICE(&s->ddrss), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->ddrss), 0, s->memmap[S32G2_DEV_DDRSS]);

    sysbus_realize(SYS_BUS_DEVICE(&s->ddrphy), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->ddrphy), 0, s->memmap[S32G2_DEV_DDRPHY]);

#if 1
    sysbus_realize(SYS_BUS_DEVICE(&s->mc_cgm2), &error_abort);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->mc_cgm2), 0, s->memmap[S32G2_DEV_MC_CGM2]);
#endif

    memory_region_init_ram(&s->sram_a1, OBJECT(dev), "sram",
                            32 * KiB, &error_abort);
    memory_region_init_ram(&s->sram_c0, OBJECT(dev), "sram_c0",
                            12 * KiB, &error_abort);
    memory_region_init_ram(&s->sram_c1, OBJECT(dev), "sram_c1",
                            12 * KiB, &error_abort);
    memory_region_init_ram(&s->sram_stdby, OBJECT(dev), "sram_stdby",
                            12 * KiB, &error_abort);
    memory_region_init_ram(&s->sram_a2, OBJECT(dev), "ssram",
                            8 * MiB, &error_abort);
    memory_region_init_ram(&s->ddr, OBJECT(dev), "dram",
                            2 * GiB, &error_abort);
    memory_region_init_ram(&s->ddr2, OBJECT(dev), "dram2",
                            2 * GiB, &error_abort);
    memory_region_init_ram(&s->qspi_buffer, OBJECT(dev), "qspi_buffer",
                            64 * MiB, &error_abort);
    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_SRAM],
                                &s->sram_a1);
    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_SSRAM],
                                &s->sram_a2);
    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_DRAM],
                                &s->ddr);
    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_DRAM2],
                                &s->ddr2);


    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_SRAM_C0],
                                &s->sram_c0);
    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_SRAM_C1],
                                &s->sram_c1);
    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_SRAM_STDBY],
                                &s->sram_stdby);
    memory_region_add_subregion(get_system_memory(), s->memmap[S32G2_DEV_QSPI_BUFFER],
                                &s->qspi_buffer);
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

#endif
    /* RTC */
    sysbus_realize(SYS_BUS_DEVICE(&s->rtc), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->rtc), 0, s->memmap[S32G2_DEV_RTC]);
#if 0
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
