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
#include "XMC1100.h"
#include "system_XMC1100.h"

#include "bricklib2/bootloader/bootloader.h"

#define DCO1_FREQUENCY 64000000U

uint32_t SystemCoreClock __attribute__((section(".no_init")));

void SystemInit(void) {
#ifdef STARTUP_SYSTEM_INIT_ALREADY_DONE
	bootloader_init();
#else
	SystemCoreSetup();
	SystemCoreClockSetup();
#endif
}

void SystemCoreSetup(void) {
#ifndef USE_DYNAMIC_FLASH_WS
	// Fix flash wait states to 1 cycle (see DS Addendum)
	NVM->NVMCONF |= NVM_NVMCONF_WS_Msk;
	NVM->CONFIG1 |= NVM_CONFIG1_FIXWS_Msk;
#endif
}

void SystemCoreClockSetup(void) {
	// Override values of CLOCK_VAL1 and CLOCK_VAL2 defined in vector table
	// MCLK = 32MHz, PCLK = 64MHz

	SCU_GENERAL->PASSWD = 0x000000C0UL; // disable bit protection
	SCU_CLK->CLKCR = (0 << SCU_CLK_CLKCR_FDIV_Pos) | // No fractional div
	                 (1 << SCU_CLK_CLKCR_IDIV_Pos) | // Div by 1 => MCLK = 32MHz
					 (1 << SCU_CLK_CLKCR_PCLKSEL_Pos) | // PCLK = 2x MCLK
					 (0 << SCU_CLK_CLKCR_RTCCLKSEL_Pos) | // RTC standby
					 (0x3FF << SCU_CLK_CLKCR_CNTADJ_Pos) | // Full counter adjustment
					 (0 << SCU_CLK_CLKCR_VDDC2LOW_Pos) | // VDCC not too low
					 (0 << SCU_CLK_CLKCR_VDDC2HIGH_Pos); // VDCC not too high

	while((SCU_CLK->CLKCR & SCU_CLK_CLKCR_VDDC2LOW_Msk));
	SCU_GENERAL->PASSWD = 0x000000C3UL; // enable bit protection

	SystemCoreClockUpdate();
}

void SystemCoreClockUpdate(void) {
	uint32_t IDIV = ((SCU_CLK->CLKCR) & SCU_CLK_CLKCR_IDIV_Msk) >> SCU_CLK_CLKCR_IDIV_Pos;
	uint32_t FDIV = ((SCU_CLK->CLKCR) & SCU_CLK_CLKCR_FDIV_Msk) >> SCU_CLK_CLKCR_FDIV_Pos;

	if(IDIV != 0) {
		// Fractional divider is enabled and used
		SystemCoreClock = ((DCO1_FREQUENCY << 6U) / ((IDIV << 8) + FDIV)) << 1U;
	} else {
		// Fractional divider bypassed. Simply divide DCO_DCLK by 2
		SystemCoreClock = DCO1_FREQUENCY >> 1U;
	}
}
