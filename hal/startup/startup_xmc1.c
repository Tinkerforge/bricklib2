/* bricklib2
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * startup_samd09.c: Startup functions for xmc1
 *                   System entry is done in startup_XMC1100.S
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

#include "configs/config.h"
#include "xmc_device.h"
#include "xmc_scu.h"

#ifdef STARTUP_SYSTEM_INIT_ALREADY_DONE
#include "bricklib2/bootloader/bootloader.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#endif

#if UC_SERIES == XMC11 || UC_SERIES == XMC12 || UC_SERIES == XMC13
#define DCO1_FREQUENCY 64000000U
#elif UC_SERIES == XMC14

#define DCO1_FREQUENCY  48000000
#ifndef OSCHP_FREQUENCY
#define OSCHP_FREQUENCY 16000000 // External crystal frequency [Hz]
#endif

// DCLK clock source selection: "Internal oscillator DCO1 (48MHz)" or "External crystal oscillator"
#define DCLK_CLOCK_SRC_DCO1 0
#define DCLK_CLOCK_SRC_EXT_XTAL 1
#ifndef DCLK_CLOCK_SRC
#define DCLK_CLOCK_SRC DCLK_CLOCK_SRC_DCO1
#endif

// SCHP external oscillator mode: "Crystal mode" or "External clock direct input mode"
#define OSCHP_MODE_XTAL 0
#define OSCHP_MODE_DIRECT 1
#ifndef OSCHP_MODE
#define OSCHP_MODE OSCHP_MODE_XTAL
#endif

// RTC clock source selection: "Internal oscillator DCO2 (32768Hz)" or "External crystal oscillator"
#define RTC_CLOCK_SRC_DCO2 0
#define RTC_CLOCK_SRC_EXT_XTAL 5
#ifndef RTC_CLOCK_SRC
#define RTC_CLOCK_SRC RTC_CLOCK_SRC_DCO2
#endif

// PCLK clock source selection: MCLK or 2xMCLK
#define PCLK_CLOCK_SRC_MCLK 0
#define PCLK_CLOCK_SRC_2XMCLK 1
#ifndef PCLK_CLOCK_SRC
#define PCLK_CLOCK_SRC PCLK_CLOCK_SRC_2XMCLK
#endif

// DCO1 calibration selection: None or External clock
#define DCO1_CAL_SRC_NONE 0
#define DCO1_CAL_SRC_EXT  1
#ifndef DCO1_CAL_SRC
#define DCO1_CAL_SRC DCO1_CAL_SRC_EXT
#endif

// Prescaler 1000 and syn preload 3000 = 16MHz external reference
#ifndef EXT_REF_PRESCALER
#define EXT_REF_PRESCALER 1000
#endif

#ifndef EXT_REF_SYN_PRELOAD
#define EXT_REF_SYN_PRELOAD 3000
#endif
#endif


uint32_t SystemCoreClock __attribute__((section(".no_init")));

void SystemInit(void) {
#ifdef STARTUP_SYSTEM_INIT_ALREADY_DONE
	bootloader_init();
	system_timer_init(SystemCoreClock, SYSTEM_TIMER_FREQUENCY);
#else
	SystemCoreSetup();
	SystemCoreClockSetup();
#endif
}

void SystemCoreSetup(void) {
#if UC_SERIES == XMC11 || UC_SERIES == XMC12 || UC_SERIES == XMC13
#ifndef USE_DYNAMIC_FLASH_WS
	// Fix flash wait states to 1 cycle (see DS Addendum)
	NVM->NVMCONF |= NVM_NVMCONF_WS_Msk;
	NVM->CONFIG1 |= NVM_CONFIG1_FIXWS_Msk;
#endif
#elif UC_SERIES == XMC14
	// Enable Prefetch unit
	SCU_GENERAL->PFUCR &= ~SCU_GENERAL_PFUCR_PFUBYP_Msk;
#endif
}

void SystemCoreClockSetup(void) {
	// Override values of CLOCK_VAL1 and CLOCK_VAL2 defined in vector table
	// MCLK = 32MHz, PCLK = 64MHz

	SCU_GENERAL->PASSWD = 0x000000C0UL; // disable bit protection

#if UC_SERIES == XMC11 || UC_SERIES == XMC12 || UC_SERIES == XMC13
	SCU_CLK->CLKCR = (0 << SCU_CLK_CLKCR_FDIV_Pos) | // No fractional div
	                 (1 << SCU_CLK_CLKCR_IDIV_Pos) | // Div by 1 => MCLK = 32MHz
					 (1 << SCU_CLK_CLKCR_PCLKSEL_Pos) | // PCLK = 2x MCLK
					 (0 << SCU_CLK_CLKCR_RTCCLKSEL_Pos) | // RTC standby
					 (0x3FF << SCU_CLK_CLKCR_CNTADJ_Pos) | // Full counter adjustment
					 (0 << SCU_CLK_CLKCR_VDDC2LOW_Pos) | // VDCC not too low
					 (0 << SCU_CLK_CLKCR_VDDC2HIGH_Pos); // VDCC not too high

	while((SCU_CLK->CLKCR & SCU_CLK_CLKCR_VDDC2LOW_Msk));
#elif UC_SERIES == XMC14
#if DCLK_CLOCK_SRC != DCLK_CLOCK_SRC_DCO1
	if(OSCHP_FREQUENCY > 20000000U) {
		SCU_ANALOG->ANAOSCHPCTRL |= SCU_ANALOG_ANAOSCHPCTRL_HYSCTRL_Msk;
	}

	// OSCHP source selection - OSC mode
	SCU_ANALOG->ANAOSCHPCTRL = (SCU_ANALOG->ANAOSCHPCTRL & ~(SCU_ANALOG_ANAOSCHPCTRL_SHBY_Msk | SCU_ANALOG_ANAOSCHPCTRL_MODE_Msk)) |
	                           (OSCHP_MODE << SCU_ANALOG_ANAOSCHPCTRL_MODE_Pos);

	// Enable OSC_HP oscillator watchdog
	SCU_CLK->OSCCSR |= SCU_CLK_OSCCSR_XOWDEN_Msk;

	do {
		// Restart OSC_HP oscillator watchdog
		SCU_INTERRUPT->SRCLR1 = SCU_INTERRUPT_SRCLR1_LOECI_Msk;
		SCU_CLK->OSCCSR |= SCU_CLK_OSCCSR_XOWDRES_Msk;

		// Wait a few DCO2 cycles for the update of the clock detection result
		for(uint32_t cycles = 0; cycles < 2500; cycles++) {
			__NOP();
		}

	// check if clock is ok
	} while(SCU_INTERRUPT->SRRAW1 & SCU_INTERRUPT_SRRAW1_LOECI_Msk);

#if DCO1_CAL_SRC == DCO1_CAL_SRC_EXT
	SCU_ANALOG->ANAOSCLPCTRL = 0;
#endif


	// DCLK source using OSC_HP
#if DCO1_CAL_SRC == DCO1_CAL_SRC_EXT
	SCU_CLK->CLKCR1 = (SCU_CLK->CLKCR1 & ~SCU_CLK_CLKCR1_DCLKSEL_Msk);
#else
	SCU_CLK->CLKCR1 |= SCU_CLK_CLKCR1_DCLKSEL_Msk;
#endif

#else
	// DCLK source using DCO1
	SCU_CLK->CLKCR1 &= ~SCU_CLK_CLKCR1_DCLKSEL_Msk;
#endif

#if RTC_CLOCK_SRC == RTC_CLOCK_SRC_EXT_XTAL
	// Enable OSC_LP
	SCU_ANALOG->ANAOSCLPCTRL &= ~SCU_ANALOG_ANAOSCLPCTRL_MODE_Msk;
#endif

	// Update PCLK selection mux.
	// Fractional divider enabled, MCLK frequency equal DCO1 frequency or external crystal frequency
	SCU_CLK->CLKCR = (1023UL <<SCU_CLK_CLKCR_CNTADJ_Pos) |
	                 (RTC_CLOCK_SRC << SCU_CLK_CLKCR_RTCCLKSEL_Pos) |
	                 (PCLK_CLOCK_SRC << SCU_CLK_CLKCR_PCLKSEL_Pos) |
	                 0x100U; // IDIV = 1

#if DCO1_CAL_SRC == DCO1_CAL_SRC_EXT
	SCU_CLK->CLKCR = (SCU_CLK->CLKCR & (uint32_t)~(SCU_CLK_CLKCR_PCLKSEL_Msk | SCU_CLK_CLKCR_RTCCLKSEL_Msk)) | 65536;
#endif

#endif

	SCU_GENERAL->PASSWD = 0x000000C3UL; // enable bit protection

	SystemCoreClockUpdate();

#if DCO1_CAL_SRC == DCO1_CAL_SRC_EXT
	XMC_SCU_CLOCK_EnableDCO1ExtRefCalibration(XMC_SCU_CLOCK_SYNC_CLKSRC_OSCHP, EXT_REF_PRESCALER, EXT_REF_SYN_PRELOAD);
#endif
}

void SystemCoreClockUpdate(void) {
	uint32_t IDIV = ((SCU_CLK->CLKCR) & SCU_CLK_CLKCR_IDIV_Msk) >> SCU_CLK_CLKCR_IDIV_Pos;
	uint32_t FDIV = ((SCU_CLK->CLKCR) & SCU_CLK_CLKCR_FDIV_Msk) >> SCU_CLK_CLKCR_FDIV_Pos;

	if(IDIV != 0) {
#if UC_SERIES == XMC11 || UC_SERIES == XMC12 || UC_SERIES == XMC13
		// Fractional divider is enabled and used
		SystemCoreClock = ((DCO1_FREQUENCY << 6U) / ((IDIV << 8) + FDIV)) << 1U;
#elif UC_SERIES == XMC14
		FDIV |= ((SCU_CLK->CLKCR1) & SCU_CLK_CLKCR1_FDIV_Msk) << 8;

		// Fractional divider is enabled and used
		if(((SCU_CLK->CLKCR1) & SCU_CLK_CLKCR1_DCLKSEL_Msk) == 0U) {
			SystemCoreClock = ((uint32_t)((DCO1_FREQUENCY << 6U) / ((IDIV << 10) + FDIV))) << 4U;
		} else {
			SystemCoreClock = ((uint32_t)((OSCHP_FREQUENCY << 6U) / ((IDIV << 10) + FDIV))) << 4U;
		}
#endif
	} else {
#if UC_SERIES == XMC11 || UC_SERIES == XMC12 || UC_SERIES == XMC13
		// Fractional divider bypassed. Simply divide DCO_DCLK by 2
		SystemCoreClock = DCO1_FREQUENCY >> 1U;
#elif UC_SERIES == XMC14
		// Fractional divider bypassed.
		if (((SCU_CLK->CLKCR1) & SCU_CLK_CLKCR1_DCLKSEL_Msk) == 0U) {
			SystemCoreClock = DCO1_FREQUENCY;
		} else {
			SystemCoreClock = OSCHP_FREQUENCY;
		}
#endif
	}
}
