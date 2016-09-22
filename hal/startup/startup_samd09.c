/* bricklib2
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * startup_samd09.c: System entry and startup functions for samd09
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

#include "samd09.h"

#include <stdbool.h>

#include "configs/config.h"

#include "system.h"

// Segments
extern uint32_t _sfixed;
extern uint32_t _efixed;
extern uint32_t _etext;
extern uint32_t _srelocate;
extern uint32_t _erelocate;
extern uint32_t _szero;
extern uint32_t _ezero;
extern uint32_t _sstack;
extern uint32_t _estack;

int main(void);
void __libc_init_array(void);

void samd09_entry(void) {
	uint32_t *src, *dest;

	// Initialize the relocate segment
	src = &_etext;
	dest = &_srelocate;

	if(src != dest) {
		for (; dest < &_erelocate;) {
			*dest++ = *src++;
		}
	}

	// Clear the zero segment
	for (dest = &_szero; dest < &_ezero;) {
		*dest++ = 0;
	}

	// Set the vector table base address
	src = (uint32_t *) &_sfixed;
	SCB->VTOR = ((uint32_t) src & SCB_VTOR_TBLOFF_Msk);

#ifndef STARTUP_SYSTEM_INIT_ALREADY_DONE // In systems with bootloader and firmware it is OK to only do this in bootloader
	// Change default QOS values to have the best performance
	SBMATRIX->SFR[SBMATRIX_SLAVE_HMCRAMC0].reg = 2;
	DMAC->QOSCTRL.bit.DQOS = 2;
	DMAC->QOSCTRL.bit.FQOS = 2;
	DMAC->QOSCTRL.bit.WRBQOS = 2;

	// Overwriting the default value of the NVMCTRL.CTRLB.MANW bit (errata reference 13134)
	NVMCTRL->CTRLB.bit.MANW = 1;
#endif

	// Initialize the C library
#ifndef NOSTARTFILES // We only call __libc_init_array if -nostartfiles is not defined
	__libc_init_array();
#endif

	// Initialize system (clocks, general hardware controllers etc)
#ifndef STARTUP_SYSTEM_INIT_ALREADY_DONE // In systems with bootloader and firmware it is OK to only do this in bootloader
	system_init();
#endif

	// We have to call bootloader_init before anything that does division!
#ifdef STARTUP_SYSTEM_INIT_ALREADY_DONE
	bootloader_init();
#endif

	// Branch to main function
	main();

	// Infinite loop
	while(true);
}

// Empty handler
void Dummy_Handler(void) {
	while(true);
}

// Cortex-M0+ core handlers
void NMI_Handler             ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void HardFault_Handler       ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void SVC_Handler             ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void PendSV_Handler          ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void SysTick_Handler         ( void ) __attribute__ ((weak, alias("Dummy_Handler")));

// Peripherals handlers
void PM_Handler              ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void SYSCTRL_Handler         ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void WDT_Handler             ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void RTC_Handler             ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void EIC_Handler             ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void NVMCTRL_Handler         ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void DMAC_Handler            ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
#ifdef ID_USB
void USB_Handler             ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
#endif
void EVSYS_Handler           ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void SERCOM0_Handler         ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void SERCOM1_Handler         ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
#ifdef ID_SERCOM2
void SERCOM2_Handler         ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
#endif
void TC1_Handler             ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void TC2_Handler             ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
void ADC_Handler             ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
#ifdef ID_DAC
void DAC_Handler             ( void ) __attribute__ ((weak, alias("Dummy_Handler")));
#endif
void PTC_Handler             ( void ) __attribute__ ((weak, alias("Dummy_Handler")));

// Exception Table
__attribute__ ((section(".vectors")))
const DeviceVectors exception_table = {
	// Configure Initial Stack Pointer, using linker-generated symbols
	(void*) (&_estack),

	(void*) samd09_entry,
	(void*) NMI_Handler,
	(void*) HardFault_Handler,
	(void*) (0UL), // Reserved
	(void*) (0UL), // Reserved
	(void*) (0UL), // Reserved
	(void*) (0UL), // Reserved
	(void*) (0UL), // Reserved
	(void*) (0UL), // Reserved
	(void*) (0UL), // Reserved
	(void*) SVC_Handler,
	(void*) (0UL), // Reserved
	(void*) (0UL), // Reserved
	(void*) PendSV_Handler,
	(void*) SysTick_Handler,

	// Configurable interrupts
	(void*) PM_Handler,             //  0 Power Manager
	(void*) SYSCTRL_Handler,        //  1 System Control
	(void*) WDT_Handler,            //  2 Watchdog Timer
	(void*) RTC_Handler,            //  3 Real-Time Counter
	(void*) EIC_Handler,            //  4 External Interrupt Controller
	(void*) NVMCTRL_Handler,        //  5 Non-Volatile Memory Controller
	(void*) DMAC_Handler,           //  6 Direct Memory Access Controller
#ifdef ID_USB
	(void*) USB_Handler,            //  7 Universal Serial Bus
#else
	(void*) (0UL), // Reserved
#endif
	(void*) EVSYS_Handler,          //  8 Event System Interface
	(void*) SERCOM0_Handler,        //  9 Serial Communication Interface 0
	(void*) SERCOM1_Handler,        // 10 Serial Communication Interface 1
#ifdef ID_SERCOM2
	(void*) SERCOM2_Handler,        // 11 Serial Communication Interface 2
#else
	(void*) (0UL), // Reserved
#endif
	(void*) (0UL), // Reserved
	(void*) TC1_Handler,            // 13 Basic Timer Counter 0
	(void*) TC2_Handler,            // 14 Basic Timer Counter 1
	(void*) ADC_Handler,            // 15 Analog Digital Converter
	(void*) (0UL), // Reserved
#ifdef ID_DAC
	(void*) DAC_Handler,            // 17 Digital Analog Converter
#else
	(void*) (0UL), // Reserved
#endif
	(void*) PTC_Handler             // 18 Peripheral Touch Controller
};

