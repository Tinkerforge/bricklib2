/* bricklib2
 * Copyright (C) 2010-2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * logging.c: Logging functionality for serial console
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

#include "logging.h"
#include "configs/config_logging.h"

#ifdef LOGGING_UARTBB
#include "bricklib2/hal/uartbb/uartbb.h"
#endif

#ifdef LOGGING_SERIAL
#include "usart.h"
#include "stdio_serial.h"
static struct usart_module logging_usart_instance;
#endif

#ifdef LOGGING_USB
#include "stdio_usb.h"
#endif

void logging_init(void) {
#ifdef LOGGING_UARTBB
	uartbb_init();
#endif

#ifdef LOGGING_SERIAL
	struct usart_config config_usart;
	usart_get_config_defaults(&config_usart);

	config_usart.generator_source = LOGGING_CLK_GENERATOR;
	config_usart.baudrate         = LOGGING_BAUDRATE;
	config_usart.mux_setting      = LOGGING_MUX_SETTING;
	config_usart.pinmux_pad0      = LOGGING_PINMUX_PAD0;
	config_usart.pinmux_pad1      = LOGGING_PINMUX_PAD1;
	config_usart.pinmux_pad2      = LOGGING_PINMUX_PAD2;
	config_usart.pinmux_pad3      = LOGGING_PINMUX_PAD3;

	stdio_serial_init(&logging_usart_instance, LOGGING_SERCOM, &config_usart);
	usart_enable(&logging_usart_instance);
#endif

#ifdef LOGGING_USB
	stdio_usb_init();
#endif
}

