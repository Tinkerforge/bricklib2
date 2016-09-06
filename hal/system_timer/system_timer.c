/* bricklib2
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * system_timer.c: Simple system tick counter
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

#include "system_timer.h"

#include "configs/config_clocks.h"

#include "bricklib2/logging/logging.h"

static uint32_t system_timer_tick;

void SysTick_Handler(void) {
	system_timer_tick++;
	if(system_timer_tick % 2) {
		PORT->Group[0].OUTSET.reg = (1 << BOOTLOADER_STATUS_LED_PIN);
	} else {
		PORT->Group[0].OUTCLR.reg = (1 << BOOTLOADER_STATUS_LED_PIN);
	}
}

void system_timer_init(const uint32_t main_clock_frequency, const uint32_t system_timer_frequency) {
	// Initialize software counter
	system_timer_tick = 0;

	// Disable the SYSTICK Counter
	SysTick->CTRL &= (~SysTick_CTRL_ENABLE_Msk);

	// Set up SysTick for 1 interrupt per ms
	SysTick_Config(main_clock_frequency/system_timer_frequency);

	// Enable SysTick interrupt
	NVIC_EnableIRQ(SysTick_IRQn);
}

uint32_t system_timer_get_time(void) {
	return system_timer_tick;
}

// This will work even with wraparound up to UIN32_MAX/2 difference.
// E.g.: end - start = 0x00000010 - 0xfffffff = 0x00000011 etc
inline bool system_timer_is_time_elapsed(const uint32_t start_measurement, const uint32_t time_to_be_elapsed) {
	return (system_timer_tick - start_measurement) >= time_to_be_elapsed;
}
