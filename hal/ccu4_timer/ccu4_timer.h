/* bricklib2
 * Copyright (C) 2017 Olaf Lüke <olaf@tinkerforge.com>
 *
 * ccu4_timer.h: Simple 64-bit XMC1X00 CCU4 timer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
**/

#ifndef CCU4_TIMER_H
#define CCU4_TIMER_H

#include <stdint.h>
#include "xmc_ccu4.h"

uint32_t ccu4_timer_get_value_32bit(void);
uint64_t ccu4_timer_get_value_64bit(void);

bool ccu4_timer_is_time_elapsed_64bit(const uint64_t start_measurement, const uint64_t time_to_be_elapsed);
bool ccu4_timer_is_time_elapsed_32bit(const uint32_t start_measurement, const uint32_t time_to_be_elapsed);

void ccu4_timer_init(const XMC_CCU4_SLICE_PRESCALER_t prescaler, const uint16_t first_slice_period_match);

#endif
