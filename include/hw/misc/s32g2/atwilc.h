/*
 * S32g2 ATWILC emulation
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

#ifndef HW_MISC_S32G2_ATWILC_H
#define HW_MISC_S32G2_ATWILC_H

#include <stdlib.h>
#include <semaphore.h>

#define BUF_SIZE   1024

typedef struct atwilc_device_t {
	sem_t dev_sem; /* Semaphore for the device side */
	sem_t host_sem; /* Semaphore for the host side */
	size_t cnt;
	size_t head;
	size_t tail;
	char buf[BUF_SIZE];
} atwilc_device_t;


#endif /* HW_MISC_S32G2_ATWILC_H */
