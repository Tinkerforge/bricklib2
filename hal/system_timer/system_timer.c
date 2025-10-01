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

#include "bricklib2/utility/util_definitions.h"

#ifdef SYSTEM_TIMER_USE_64BIT_US
static volatile uint64_t system_timer_tick;
#else
static volatile uint32_t system_timer_tick;
#endif

#ifdef SYSTEM_TIMER_CALLBACK_ENABLED
void system_timer_callback();
#endif

void
#ifdef SYSTEM_TIMER_IS_RAMFUNC
__attribute__ ((section (".ram_code")))
#endif
SysTick_Handler(void) {
	system_timer_tick++;
#ifdef SYSTEM_TIMER_CALLBACK_ENABLED
	system_timer_callback();
#endif
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

uint32_t
#ifdef SYSTEM_TIMER_IS_RAMFUNC
__attribute__ ((section (".ram_code")))
#endif
system_timer_get_ms(void) {
	return system_timer_tick;
}

#ifdef SYSTEM_TIMER_USE_64BIT_US
uint64_t
#ifdef SYSTEM_TIMER_IS_RAMFUNC
__attribute__ ((section (".ram_code")))
#endif
__attribute__((optimize("-O3")))
system_timer_get_us(void) {
	uint64_t ms1 = 0;
	uint64_t ms2 = 0;
	uint32_t us  = 0;

	// At 48 MHz the VAL value goes from 47999 to 0. At 0 the SysTick interrupt is generated.
	// To get the time in us we take the ms from the system timer and add the current VAL count
	// If we devide it by 48 we get a decrementing value beteween 999 and 0. Thus we can get the
	// current us with 999 - VAL/48.

	// Additionally we have to make sure that the SysTick interrupt is not triggered right between
	// we read it and VAL.
	do {
		ms1 = system_timer_tick;
#ifdef SYSTEM_TIMER_MAIN_CLOCK_MHZ_48
		// This is about three times as fast as the division by 48.
		us  = 999 - (SysTick->VAL*21845 >> 20);
#else
		us  = 999 - SysTick->VAL/SYSTEM_TIMER_MAIN_CLOCK_MHZ;
#endif
		ms2 = system_timer_tick;
	} while(ms1 != ms2);

	return ms1*1000 + us;
}
#endif

// This will work even with wrap-around up to UINT32_MAX/2 difference.
// E.g.: end - start = 0x00000010 - 0xfffffff = 0x00000011 etc
inline bool
#ifdef SYSTEM_TIMER_IS_RAMFUNC
__attribute__ ((section (".ram_code")))
#endif
 system_timer_is_time_elapsed_ms(const uint32_t start_measurement, const uint32_t time_to_be_elapsed) {
	return (uint32_t)(system_timer_get_ms() - start_measurement) >= time_to_be_elapsed;
}

void system_timer_sleep_ms(const uint32_t sleep) {
	const uint32_t time = system_timer_get_ms();
	while(!system_timer_is_time_elapsed_ms(time, sleep));
}

#ifdef SYSTEM_TIMER_USE_64BIT_US
inline bool
#ifdef SYSTEM_TIMER_IS_RAMFUNC
__attribute__ ((section (".ram_code")))
#endif
 system_timer_is_time_elapsed_us(const uint64_t start_measurement, const uint64_t time_to_be_elapsed) {
	return (system_timer_get_us() - start_measurement) >= time_to_be_elapsed;
}

void system_timer_sleep_us(const uint64_t sleep) {
	const uint64_t time = system_timer_get_us();
	while(!system_timer_is_time_elapsed_us(time, sleep));
}
#else
// Only works for sleep values <= 500
static void system_timer_sleep_us_internal(const uint32_t sleep) {
	// We have about 5us function overhead, remove them from the sleep time
	const uint32_t sleep_value = sleep <= 5 ? SysTick->LOAD/1000 : (sleep-5)*(SysTick->LOAD/1000);
	const uint16_t start_value = (SysTick->LOAD - SysTick->VAL);
	while(true) {
		const uint16_t new_value = (SysTick->LOAD - SysTick->VAL);
		if(new_value > start_value) {
			const uint16_t diff = new_value - start_value;
			if(diff > sleep_value) {
				return;
			}
		} else {
			if((new_value + SysTick->LOAD - start_value) > sleep_value) {
				return;
			}
		}
	}
}

// TODO: This is too hacky, there is to much overhead...
void system_timer_sleep_us(const uint32_t sleep) {
	uint32_t remaining_sleep = sleep;
	while(remaining_sleep > 0) {
		uint32_t sleep_segmentation = MIN(500, remaining_sleep);
		system_timer_sleep_us_internal(sleep_segmentation);
		remaining_sleep -= sleep_segmentation;
	}
}
#endif

// The STM32F0 CubeMX HAL code implements a system timer similar to the one in the bricklib.
// They can't both be used at the same time. For the STM32 HAL to function we need to provide
// the HAL_InitTick and HAL_GetTick hooks.
#if defined(STM32F0)

#include "stm32f0xx_hal_def.h"

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
	system_timer_init(HAL_RCC_GetHCLKFreq(), SYSTEM_TIMER_FREQUENCY);
	return HAL_OK;
}

uint32_t HAL_GetTick(void) {
	return system_timer_get_ms();
}
#endif