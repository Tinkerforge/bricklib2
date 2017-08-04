/* bricklib2
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * uartbb.h: Small driver for bit-banged uart
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

#ifndef UARTBB_H
#define UARTBB_H

#include <stdint.h>

void uartbb_init(void);
void uartbb_tx(const uint8_t value);
void uartbb_putarru8(const char *name, const uint8_t *data, const uint32_t length);
void uartbb_puts(const char *str);
void uartbb_puti(const int32_t value);
void uartbb_putu(const uint32_t value);
void uartbb_putnl(void);
void uartbb_printf(char const *fmt, ...);

#endif
