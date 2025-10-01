/* bricklib2
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 * Copyright (C) 2018 Matthias Bolte <matthias@tinkerforge.com>
 *
 * uartbb.c: Small driver for bit-banged UART
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

#ifdef UARTBB_PRINTF_ADVANCED
#include "bricklib2/utility/util_definitions.h"
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#ifndef UARTBB_TX_PIN
#define UARTBB_TX_PIN 17
#endif

#ifndef UARTBB_BIT_TIME
//#define UARTBB_BIT_TIME 1250 // 48000000/38400 perfect
//#define UARTBB_BIT_TIME 417 // 48000000/115200 rounded
//#define UARTBB_BIT_TIME 5000 // 48000000/9600 perfect
//#define UARTBB_BIT_TIME 833 // 8000000/9600 rounded

//#define UARTBB_BIT_TIME 320
//#define UARTBB_BIT_TIME 278 // 32000000/115200 rounded
//#define UARTBB_BIT_TIME 833 // 32000000/38400 rounded
//#define UARTBB_BIT_TIME 1250 // 48000000/38400 perfect

// Differentiate between XMC1400 and XMC1100/XMC1300
#if UC_SERIES == XMC14
#define UARTBB_BIT_TIME 417 // 48000000/115200 rounded
#define UARTBB_COUNT_TO_IN_1MS 48000
#else
#define UARTBB_BIT_TIME 278 // 32000000/115200 rounded
#define UARTBB_COUNT_TO_IN_1MS 32000
#endif
#endif

#if defined(STM32F0)
#define UARTBB_BIT_TIME 417 // 48000000/115200 rounded
#define UARTBB_COUNT_TO_IN_1MS 48000
#endif

#if defined(UARTBB_TX_PORTA)
#define UARTBB_TX_PORT GPIOA
#elif defined(UARTBB_TX_PORTB)
#define UARTBB_TX_PORT GPIOB
#elif defined(UARTBB_TX_PORTC)
#define UARTBB_TX_PORT GPIOC
#elif defined(UARTBB_TX_PORTF)
#define UARTBB_TX_PORT GPIOF
#endif

#ifndef CUSTOM_ENABLE_IRQ
#define CUSTOM_ENABLE_IRQ() __enable_irq()
#endif

#ifndef CUSTOM_DISABLE_IRQ
#define CUSTOM_DISABLE_IRQ() __disable_irq()
#endif

static inline void uartbb_wait_1bit(int32_t start) {
	while(true) {
		int32_t current = UARTBB_COUNT_TO_IN_1MS - (int32_t)SysTick->VAL;
		int32_t result = current - start;
		if(result < 0) {
			result += UARTBB_COUNT_TO_IN_1MS;
		}

		if(result >= UARTBB_BIT_TIME) {
			break;
		}
	}
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
	XMC_GPIO_CONFIG_t uartbb_pin;
	uartbb_pin.mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL;
	uartbb_pin.output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH;
	XMC_GPIO_Init(UARTBB_TX_PIN, &uartbb_pin);
#elif defined(STM32F0)
#if defined(UARTBB_TX_PORTA)
	__HAL_RCC_GPIOA_CLK_ENABLE();
#elif defined(UARTBB_TX_PORTB)
	__HAL_RCC_GPIOB_CLK_ENABLE();
#elif defined(UARTBB_TX_PORTC)
	__HAL_RCC_GPIOC_CLK_ENABLE();
#elif defined(UARTBB_TX_PORTF)
	__HAL_RCC_GPIOF_CLK_ENABLE();
#endif
	GPIO_InitTypeDef gpio_pp = {
		.Pin   = UARTBB_TX_PIN,
		.Mode  = GPIO_MODE_OUTPUT_PP,
		.Speed = GPIO_SPEED_FREQ_LOW
	};

	HAL_GPIO_Init(UARTBB_TX_PORT, &gpio_pp);
	HAL_GPIO_WritePin(UARTBB_TX_PORT, UARTBB_TX_PIN, GPIO_PIN_SET);
#endif

}

void uartbb_tx(const uint8_t value) {
#if defined(__SAM0__)
	PortGroup *const port = &PORT->Group[0];
#elif defined(__XMC1__)
#endif

  uint16_t value16 = 0 | (value << 1) | 1 << 9;

  int32_t start;
  uint8_t bit_count = 10;

#if defined(__SAM0__)
	cpu_irq_disable();
#elif defined(__XMC1__) || defined(STM32F0)
	CUSTOM_DISABLE_IRQ();
#endif

  start = UARTBB_COUNT_TO_IN_1MS - (int32_t)SysTick->VAL;

  do {
    if(value16 & 1) {
#if defined(__SAM0__)
      port->OUTSET.reg = (1 << UARTBB_TX_PIN);
#elif defined(__XMC1__)
	XMC_GPIO_SetOutputHigh(UARTBB_TX_PIN);
#elif defined(STM32F0)
	HAL_GPIO_WritePin(UARTBB_TX_PORT, UARTBB_TX_PIN, GPIO_PIN_SET);
#endif
    } else {
#if defined(__SAM0__)
      port->OUTCLR.reg = (1 << UARTBB_TX_PIN);
#elif defined(__XMC1__)
      XMC_GPIO_SetOutputLow(UARTBB_TX_PIN);
#elif defined(STM32F0)
	HAL_GPIO_WritePin(UARTBB_TX_PORT, UARTBB_TX_PIN, GPIO_PIN_RESET);
#endif
    }

    uartbb_wait_1bit(start);
    start += UARTBB_BIT_TIME;

    value16 >>= 1;
  } while (--bit_count);

#if defined(__SAM0__)
	cpu_irq_enable();
#elif defined(__XMC1__) || defined(STM32F0)
	CUSTOM_ENABLE_IRQ();
#endif
}

void uartbb_putarru8(const char *name, const uint8_t *data, const uint32_t length) {
	uartbb_puts(name); uartbb_puts(": ");
	for(uint32_t i = 0; i < length; i++) {
		uartbb_putu(data[i]); uartbb_puts(", ");
	}
	uartbb_putnl();
}

void uartbb_puts(const char *str) {
	uint32_t i = 0;
	while(str[i] != '\0') {
		uartbb_tx(str[i]);
		i++;
	}
}

#ifdef UARTBB_PRINTF_ADVANCED

void uartbb_puts_advanced(const char *str, const uint32_t zero_padding, const uint32_t grouping) {
	uint32_t i;
	uint32_t k = grouping + 1;

	if(zero_padding > 0) {
		uint32_t str_length = strlen(str);
		uint32_t padding_length = str_length < zero_padding ? zero_padding - str_length : 0;

		if(grouping > 0) {
			k -= grouping - (MAX(str_length, zero_padding) % grouping);

			if(k <= 1) {
				k = grouping + 1;
			}
		}

		for(i = 0; i < padding_length; i++) {
			if(grouping > 0 && --k == 0) {
				k = grouping;
				uartbb_tx('_');
			}

			uartbb_tx('0');
		}

	} else if(grouping > 0) {
		k -= grouping - (strlen(str) % grouping);

		if(k <= 1) {
			k = grouping + 1;
		}
	}

	for(i = 0; str[i] != '\0'; ++i) {
		if(grouping > 0 && --k == 0) {
			k = grouping;
			uartbb_tx('_');
		}

		uartbb_tx(str[i]);
	}
}

#endif

void uartbb_puti(const int32_t value) {
	char str[16] = {'\0'};
	itoa(value, str, 10);
	uint32_t i = 0;
	while(str[i] != '\0') {
		uartbb_tx(str[i]);
		i++;
	}
}

void uartbb_putu(const uint32_t value) {
	char str[16] = {'\0'};
	utoa(value, str, 10);
	uint32_t i = 0;
	while(str[i] != '\0') {
		uartbb_tx(str[i]);
		i++;
	}
}

void uartbb_putnl(void) {
	uartbb_puts("\n\r");
}

// Very minimalistic printf: optional zero-padding and grouping, but no l-modifier or similar and no float
void uartbb_printf(char const *fmt, ...) {
	va_list va;
	va_start(va, fmt);

	// Evaluates to 33. The + 1 is required to fit the null-terminator written by utoa when printing 32 binary digits.
	char buffer[sizeof(int) * 8 + 1];
	char character;
#ifdef UARTBB_PRINTF_ADVANCED
	uint32_t zero_padding;
	uint32_t grouping;
#endif

	while((character = *(fmt++))) {
		if(character != '%') {
			uartbb_tx(character);
		} else {
			character = *(fmt++);

#ifdef UARTBB_PRINTF_ADVANCED
			zero_padding = 0;

			if(character == '0') {
				character = *(fmt++);

				while(character >= '0' && character <= '9') {
					zero_padding = zero_padding * 10 + (character - '0');
					character = *(fmt++);
				}
			}

			grouping = 0;

			if(character == '_') {
				character = *(fmt++);

				while(character >= '0' && character <= '9') {
					grouping = grouping * 10 + (character - '0');
					character = *(fmt++);
				}
			}
#endif

			switch(character) {
				case '\0': {
					return;
				}

				case 'u': {
					uint32_t value = va_arg(va, uint32_t);

					utoa(value, buffer, 10);
#ifdef UARTBB_PRINTF_ADVANCED
					uartbb_puts_advanced(buffer, zero_padding, grouping);
#else
					uartbb_puts(buffer);
#endif
					break;
				}

				case 'b': {
					uint32_t value = va_arg(va, uint32_t);

					utoa(value, buffer, 2);
#ifdef UARTBB_PRINTF_ADVANCED
					uartbb_puts_advanced(buffer, zero_padding, grouping);
#else
					uartbb_puts(buffer);
#endif
					break;
				}

				case 'd': {
					int32_t value = va_arg(va, int32_t);

					itoa(value, buffer, 10);
#ifdef UARTBB_PRINTF_ADVANCED
					uartbb_puts_advanced(buffer, zero_padding, grouping);
#else
					uartbb_puts(buffer);
#endif
					break;
				}

				case 'x': {
					uint32_t value = va_arg(va, uint32_t);

					utoa(value, buffer, 16);
#ifdef UARTBB_PRINTF_ADVANCED
					uartbb_puts_advanced(buffer, zero_padding, grouping);
#else
					uartbb_puts(buffer);
#endif
					break;
				}

				case 'c' : {
					uartbb_tx((char)(va_arg(va, int)));
					break;
				}

				case 's' : {
					uartbb_puts(va_arg(va, char*));
					break;
				}

				default:
					uartbb_tx(character);
					break;
			}
		}
	}

    va_end(va);
}
