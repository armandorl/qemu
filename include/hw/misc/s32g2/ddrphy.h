/* WARNING: This file is autogenerated do not modify manually */
/*
 * S32g2 DDR PHY emulation
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

#ifndef HW_MISC_S32G2_DDRPHY_H
#define HW_MISC_S32G2_DDRPHY_H

#include "qom/object.h"
#include "hw/sysbus.h"

/**
 * @name Constants
 * @{
 */

/** Highest register address used by DDRPHY device */
#define S32G2_DDRPHY_REGS_MAXADDR   (0x10000)

/** Total number of known registers */
#define S32G2_DDRPHY_REGS_NUM       ((S32G2_DDRPHY_REGS_MAXADDR / \
                                      sizeof(uint32_t)) + 1)

/** @} */

/**
 * @name Object model
 * @{
 */

#define TYPE_S32G2_DDRPHY    "s32g2-ddrphy"
OBJECT_DECLARE_SIMPLE_TYPE(S32G2ddrphyState, S32G2_DDRPHY)

/** @} */

/**
 * S32G2 DDR PHY object instance state
 */
struct S32G2ddrphyState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    /** Maps I/O registers in physical memory */
    MemoryRegion iomem;

    /** Array of hardware registers */
    uint32_t regs[S32G2_DDRPHY_REGS_NUM];

};

#endif /* HW_MISC_S32G2_DDRPHY_H */
