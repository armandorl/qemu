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

/*
 * Based on the Allwinner H3 implementation
 */

#ifndef HW_ARM_S32G2_H
#define HW_ARM_S32G2_H

#include "qom/object.h"
#include "hw/arm/boot.h"
//#include "hw/timer/allwinner-a10-pit.h"
#include "hw/intc/arm_gic.h"
//#include "hw/misc/s32g2-ccu.h"
//#include "hw/misc/allwinner-cpucfg.h"
//#include "hw/misc/s32g2-dramc.h"
//#include "hw/misc/s32g2-sysctrl.h"
//#include "hw/misc/allwinner-sid.h"
#include "hw/sd/allwinner-sdhost.h"
//#include "hw/net/allwinner-sun8i-emac.h"
//#include "hw/rtc/allwinner-rtc.h"
//#include "hw/i2c/allwinner-i2c.h"
#include "target/arm/cpu.h"
#include "sysemu/block-backend.h"
#include "hw/misc/s32g2/sramc.h"


/**
 * Allwinner H3 device list
 *
 * This enumeration is can be used refer to a particular device in the
 * Allwinner H3 SoC. For example, the physical memory base address for
 * each device can be found in the S32G2State object in the memmap member
 * using the device enum value as index.
 *
 * @see S32G2State
 */
enum {
#if 0
    S32G2_DEV_SRAM_A1,
    S32G2_DEV_SRAM_A2,
    S32G2_DEV_SRAM_C,
    S32G2_DEV_SYSCTRL,
    S32G2_DEV_MMC0,
    S32G2_DEV_SID,
    S32G2_DEV_EHCI0,
    S32G2_DEV_OHCI0,
    S32G2_DEV_EHCI1,
    S32G2_DEV_OHCI1,
    S32G2_DEV_EHCI2,
    S32G2_DEV_OHCI2,
    S32G2_DEV_EHCI3,
    S32G2_DEV_OHCI3,
    S32G2_DEV_CCU,
    S32G2_DEV_PIT,
    S32G2_DEV_UART0,
    S32G2_DEV_UART1,
    S32G2_DEV_UART2,
    S32G2_DEV_UART3,
    S32G2_DEV_EMAC,
    S32G2_DEV_TWI0,
    S32G2_DEV_TWI1,
    S32G2_DEV_TWI2,
    S32G2_DEV_DRAMCOM,
    S32G2_DEV_DRAMCTL,
    S32G2_DEV_DRAMPHY,
    S32G2_DEV_GIC_DIST,
    S32G2_DEV_GIC_CPU,
    S32G2_DEV_GIC_HYP,
    S32G2_DEV_GIC_VCPU,
    S32G2_DEV_RTC,
    S32G2_DEV_CPUCFG,
    S32G2_DEV_R_TWI,
    S32G2_DEV_SDRAM
#endif
    S32G2_DEV_GIC_DIST,
    S32G2_DEV_GIC_CPU,
    S32G2_DEV_GIC_HYP,
    S32G2_DEV_GIC_VCPU,
    S32G2_DEV_SRAM,
    S32G2_DEV_SSRAM,
    S32G2_DEV_DRAM,
    S32G2_DEV_SRAM_CTRL_C0,
    S32G2_DEV_SRAM_CTRL_C1,
    S32G2_DEV_SRAM_C0,
    S32G2_DEV_SRAM_C1,
    S32G2_DEV_SPI0,
    S32G2_DEV_SPI1,
    S32G2_DEV_SPI2,
    S32G2_DEV_QSPI,
    S32G2_DEV_I2C0,
    S32G2_DEV_I2C1,
    S32G2_DEV_I2C2
};

/** Total number of CPU cores in the H3 SoC */
#define S32G2_NUM_CPUS      (4)

/**
 * Allwinner H3 object model
 * @{
 */

/** Object type for the Allwinner H3 SoC */
#define TYPE_S32G2 "s32g2"

/** Convert input object to Allwinner H3 state object */
OBJECT_DECLARE_SIMPLE_TYPE(S32G2State, S32G2)

/** @} */

/**
 * Allwinner H3 object
 *
 * This struct contains the state of all the devices
 * which are currently emulated by the H3 SoC code.
 */
struct S32G2State {
    /*< private >*/
    DeviceState parent_obj;
    /*< public >*/

    ARMCPU cpus[S32G2_NUM_CPUS];
    const hwaddr *memmap;
//    AwA10PITState timer;
//    S32G2ClockCtlState ccu;
//    AwCpuCfgState cpucfg;
//    S32G2DramCtlState dramc;
//    S32G2SysCtrlState sysctrl;
//    AwSidState sid;
    AwSdHostState mmc0;
//    AWI2CState i2c0;
//    AWI2CState i2c1;
//    AWI2CState i2c2;
//    AWI2CState r_twi;
//    AwSun8iEmacState emac;
//    AwRtcState rtc;
    GICState gic;
    MemoryRegion sram_a1;
    MemoryRegion sram_a2;
    MemoryRegion sram_c;
    MemoryRegion sram_c0;
    MemoryRegion sram_c1;

    S32G2SramcState sram_ctrl_c0;
    S32G2SramcState sram_ctrl_c1;

};

/**
 * Emulate Boot ROM firmware setup functionality.
 *
 * A real Allwinner H3 SoC contains a Boot ROM
 * which is the first code that runs right after
 * the SoC is powered on. The Boot ROM is responsible
 * for loading user code (e.g. a bootloader) from any
 * of the supported external devices and writing the
 * downloaded code to internal SRAM. After loading the SoC
 * begins executing the code written to SRAM.
 *
 * This function emulates the Boot ROM by copying 32 KiB
 * of data from the given block device and writes it to
 * the start of the first internal SRAM memory.
 *
 * @s: Allwinner H3 state object pointer
 * @blk: Block backend device object pointer
 */
void s32g2_bootrom_setup(S32G2State *s, BlockBackend *blk, hwaddr* code_entry);

#endif /* HW_ARM_S32G2_H */
