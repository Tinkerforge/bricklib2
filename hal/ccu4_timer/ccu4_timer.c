/* bricklib2
 * Copyright (C) 2017 Olaf Lüke <olaf@tinkerforge.com>
 *
 * ccu4_timer.c: Simple 64-bit XMC1X00 CCU4 timer
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

#include "ccu4_timer.h"

#include "xmc_ccu4.h"

uint32_t ccu4_timer_get_value_32bit(void) {
	uint16_t t1 = XMC_CCU4_SLICE_GetTimerValue(CCU40_CC41);
	uint16_t t0 = XMC_CCU4_SLICE_GetTimerValue(CCU40_CC40);

	while(t1 != XMC_CCU4_SLICE_GetTimerValue(CCU40_CC41)) {
		t1 = XMC_CCU4_SLICE_GetTimerValue(CCU40_CC41);
		t0 = XMC_CCU4_SLICE_GetTimerValue(CCU40_CC40);
	}

	return (((uint64_t)t1) << 16) | (((uint64_t)t0) << 0);
}

uint64_t ccu4_timer_get_value_64bit(void) {
	uint16_t t3 = XMC_CCU4_SLICE_GetTimerValue(CCU40_CC43);
	uint16_t t2 = XMC_CCU4_SLICE_GetTimerValue(CCU40_CC42);
	uint16_t t1 = XMC_CCU4_SLICE_GetTimerValue(CCU40_CC41);
	uint16_t t0 = XMC_CCU4_SLICE_GetTimerValue(CCU40_CC40);

	while((t3 != XMC_CCU4_SLICE_GetTimerValue(CCU40_CC43)) ||
	      (t2 != XMC_CCU4_SLICE_GetTimerValue(CCU40_CC42))||
	      (t1 != XMC_CCU4_SLICE_GetTimerValue(CCU40_CC41))) {
		t3 = XMC_CCU4_SLICE_GetTimerValue(CCU40_CC43);
		t2 = XMC_CCU4_SLICE_GetTimerValue(CCU40_CC42);
		t1 = XMC_CCU4_SLICE_GetTimerValue(CCU40_CC41);
		t0 = XMC_CCU4_SLICE_GetTimerValue(CCU40_CC40);
	}

	return (((uint64_t)t3) << 48) | (((uint64_t)t2) << 32) | (((uint64_t)t1) << 16) | (((uint64_t)t0) << 0);
}

bool ccu4_timer_is_time_elapsed_64bit(const uint64_t start_measurement, const uint64_t time_to_be_elapsed) {
	return (uint64_t)(ccu4_timer_get_value_64bit() - start_measurement) >= time_to_be_elapsed;
}

bool ccu4_timer_is_time_elapsed_32bit(const uint32_t start_measurement, const uint32_t time_to_be_elapsed) {
	return (uint32_t)(ccu4_timer_get_value_32bit() - start_measurement) >= time_to_be_elapsed;
}

void ccu4_timer_init(const XMC_CCU4_SLICE_PRESCALER_t prescaler, const uint16_t first_slice_period_match) {
	XMC_CCU4_SLICE_COMPARE_CONFIG_t timer0_config = {
		.timer_mode          = XMC_CCU4_SLICE_TIMER_COUNT_MODE_EA,
		.monoshot            = XMC_CCU4_SLICE_TIMER_REPEAT_MODE_REPEAT,
		.shadow_xfer_clear   = false,
		.dither_timer_period = false,
		.dither_duty_cycle   = false,
		.prescaler_mode      = XMC_CCU4_SLICE_PRESCALER_MODE_NORMAL,
		.mcm_enable          = false,
		.prescaler_initval   = prescaler, // Use prescaler 1 to get fccu4 = mclk*2
		.float_limit         = 0U,
		.dither_limit        = 0U,
		.passive_level       = XMC_CCU4_SLICE_OUTPUT_PASSIVE_LEVEL_LOW,
		.timer_concatenation = false
	};

	XMC_CCU4_SLICE_COMPARE_CONFIG_t timer123_config = {
		.timer_mode          = XMC_CCU4_SLICE_TIMER_COUNT_MODE_EA,
		.monoshot            = XMC_CCU4_SLICE_TIMER_REPEAT_MODE_REPEAT,
		.shadow_xfer_clear   = false,
		.dither_timer_period = false,
		.dither_duty_cycle   = false,
		.prescaler_mode      = XMC_CCU4_SLICE_PRESCALER_MODE_NORMAL,
		.mcm_enable          = false,
		.prescaler_initval   = prescaler,
		.float_limit         = 0U,
		.dither_limit        = 0U,
		.passive_level       = XMC_CCU4_SLICE_OUTPUT_PASSIVE_LEVEL_LOW,
		.timer_concatenation = true
	};

	XMC_CCU4_Init(CCU40, XMC_CCU4_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
	XMC_CCU4_StartPrescaler(CCU40);

	XMC_CCU4_EnableClock(CCU40, 0);
	XMC_CCU4_SLICE_CompareInit(CCU40_CC40, &timer0_config);
	XMC_CCU4_SLICE_SetTimerPeriodMatch(CCU40_CC40, first_slice_period_match);
	XMC_CCU4_SLICE_SetTimerCompareMatch(CCU40_CC40, 0);
	XMC_CCU4_EnableShadowTransfer(CCU40, XMC_CCU4_SHADOW_TRANSFER_SLICE_0 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_0);

	XMC_CCU4_EnableClock(CCU40, 1);
	XMC_CCU4_SLICE_CompareInit(CCU40_CC41, &timer123_config);
	XMC_CCU4_SLICE_SetTimerPeriodMatch(CCU40_CC41, 0xFFFF);
	XMC_CCU4_SLICE_SetTimerCompareMatch(CCU40_CC41, 0);
	XMC_CCU4_EnableShadowTransfer(CCU40, XMC_CCU4_SHADOW_TRANSFER_SLICE_1 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_1);

	XMC_CCU4_EnableClock(CCU40, 2);
	XMC_CCU4_SLICE_CompareInit(CCU40_CC42, &timer123_config);
	XMC_CCU4_SLICE_SetTimerPeriodMatch(CCU40_CC42, 0xFFFF);
	XMC_CCU4_SLICE_SetTimerCompareMatch(CCU40_CC42, 0);
	XMC_CCU4_EnableShadowTransfer(CCU40, XMC_CCU4_SHADOW_TRANSFER_SLICE_2 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_2);

	XMC_CCU4_EnableClock(CCU40, 3);
	XMC_CCU4_SLICE_CompareInit(CCU40_CC43, &timer123_config);
	XMC_CCU4_SLICE_SetTimerPeriodMatch(CCU40_CC43, 0xFFFF);
	XMC_CCU4_SLICE_SetTimerCompareMatch(CCU40_CC43, 0);
	XMC_CCU4_EnableShadowTransfer(CCU40, XMC_CCU4_SHADOW_TRANSFER_SLICE_3 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_3);

	XMC_CCU4_SLICE_StartTimer(CCU40_CC40);
	XMC_CCU4_SLICE_StartTimer(CCU40_CC41);
	XMC_CCU4_SLICE_StartTimer(CCU40_CC42);
	XMC_CCU4_SLICE_StartTimer(CCU40_CC43);
}
