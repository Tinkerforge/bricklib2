/* bricklib2
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * uartbb.c: Small driver for bit-banged uart
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
 */

#include "uartbb.h"

#include "configs/config.h"
#if defined(__SAM0__)
#include "port.h"
#elif defined(__XMC1__)
#include "xmc_gpio.h"
#endif

#include <stdlib.h>

#ifndef UARTBB_TX_PIN
#define UARTBB_TX_PIN 17
#endif

#ifndef UARTBB_BIT_TIME
//#define UARTBB_BIT_TIME 1250 // 48000000/38400 perfect
//#define UARTBB_BIT_TIME 417 // 48000000/115200 rounded
//#define UARTBB_BIT_TIME 5000 // 48000000/9600 perfect
//#define UARTBB_BIT_TIME 833 // 8000000/9600 rounded

//#define UARTBB_BIT_TIME 278 // 32000000/115200 rounded
#define UARTBB_BIT_TIME 833 // 32000000/38400 rounded
#endif

static inline void uartbb_wait_1bit(uint32_t start) {
    while((((48000 - SysTick->VAL) - start) % 48000) < UARTBB_BIT_TIME);
}

void uartbb_init(void) {
#if defined(__SAM0__)

	// If we already use the port.c it is cheaper to use the
	// port_ functions.
	// Otherwise we can save 40 byte by doing it by hand.
#ifdef UARTBB_USE_PORT_C
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	pin_conf.direction = PORT_PIN_DIR_OUTPUT;

	port_pin_set_config(UARTBB_TX_PIN, &pin_conf);
	port_pin_set_output_level(UARTBB_TX_PIN, true);
#else
	PortGroup *const port = &PORT->Group[0];
	const uint32_t pin_mask = (1 << UARTBB_TX_PIN);

	const uint32_t lower_pin_mask = (pin_mask & 0xFFFF);
	const uint32_t upper_pin_mask = (pin_mask >> 16);

	// TODO: Decide if lower or upper pin_mask is needed with pre-processor
	//       to save a few bytes

	// Configure lower 16 bits (needed if lower_pin_mask != 0)
	port->WRCONFIG.reg = (lower_pin_mask << PORT_WRCONFIG_PINMASK_Pos) | PORT_WRCONFIG_WRPMUX | PORT_WRCONFIG_WRPINCFG;

	// Configure upper 16 bits  (needed if upper_pin_mask != 0)
	port->WRCONFIG.reg = (upper_pin_mask << PORT_WRCONFIG_PINMASK_Pos) | PORT_WRCONFIG_WRPMUX | PORT_WRCONFIG_WRPINCFG | PORT_WRCONFIG_HWSEL;

	// Direction to output
	port->DIRSET.reg = pin_mask;

	// Default high
	port->OUTSET.reg = (1 << UARTBB_TX_PIN);
#endif
#elif defined(__XMC1__)
	XMC_GPIO_SetMode(UARTBB_TX_PIN, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
#endif
}

void uartbb_tx(uint8_t value) {
#if defined(__SAM0__)
	PortGroup *const port = &PORT->Group[0];
#elif defined(__XMC1__)
#endif

    uint32_t start;
    uint8_t bit_count = 8;

#if defined(__SAM0__)
	cpu_irq_disable();
#elif defined(__XMC1__)
	__disable_irq();
#endif

    start = 48000 - SysTick->VAL;
#if defined(__SAM0__)
   	port->OUTCLR.reg = (1 << UARTBB_TX_PIN);
#elif defined(__XMC1__)
	XMC_GPIO_SetOutputLow(UARTBB_TX_PIN);
#endif
   	uartbb_wait_1bit(start);

    do {
        if(value & 1) {
#if defined(__SAM0__)
        	port->OUTSET.reg = (1 << UARTBB_TX_PIN);
#elif defined(__XMC1__)
	XMC_GPIO_SetOutputHigh(UARTBB_TX_PIN);
#endif
        } else {
#if defined(__SAM0__)
        	port->OUTCLR.reg = (1 << UARTBB_TX_PIN);
#elif defined(__XMC1__)
	XMC_GPIO_SetOutputLow(UARTBB_TX_PIN);
#endif
        }
        start += UARTBB_BIT_TIME;
        uartbb_wait_1bit(start);

        value >>= 1;
    } while (--bit_count);

#if defined(__SAM0__)
    port->OUTSET.reg = (1 << UARTBB_TX_PIN);
	cpu_irq_enable();
#elif defined(__XMC1__)
	XMC_GPIO_SetOutputHigh(UARTBB_TX_PIN);
	__enable_irq();
#endif

	// Wait for at least 30 bits (3 chars) here
    for(uint8_t i = 0; i < 30; i++) {
		start += UARTBB_BIT_TIME;
		uartbb_wait_1bit(start);
    }

}

void uartbb_puts(char *str) {
	uint32_t i = 0;
	while(str[i] != '\0') {
		uartbb_tx(str[i]);
		i++;
	}
}

void uartbb_puti(int32_t value) {
	char str[16] = {'\0'};
	itoa(value, str, 10);
	uint32_t i = 0;
	while(str[i] != '\0') {
		uartbb_tx(str[i]);
		i++;
	}
}
