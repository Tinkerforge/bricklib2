/* TNG
 * Copyright (C) 2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tng.h: TNG system standard init/tick
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

#ifndef TNG_H
#define TNG_H

#include <stdint.h>

typedef enum {
	HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE = 0,
	HANDLE_MESSAGE_RESPONSE_EMPTY = 1,
	HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED = 2,
	HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER = 3,
	HANDLE_MESSAGE_RESPONSE_NONE = 4,
} TNGHandleMessageResponse;

void tng_init(void);
void tng_tick(void);
uint32_t tng_get_uid(void);
void tng_try_usb_recv(void);

#endif
