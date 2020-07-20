/* TNG
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tng_led.h: TNG status/channel LED driver
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

#ifndef TNG_LED_H
#define TNG_LED_H

#include "configs/config_tng_led.h"

#include <stdint.h>
#include <stdbool.h>

#define TNG_LED_STATUS_NUM 3

typedef enum {
	TNG_LED_STATUS_R = 0,
	TNG_LED_STATUS_G = 1,
	TNG_LED_STATUS_B = 2
} TNGLEDStatus;

typedef enum {
	TNG_LED_CHANNEL_0  =  0,
	TNG_LED_CHANNEL_1  =  1,
	TNG_LED_CHANNEL_2  =  2,
	TNG_LED_CHANNEL_3  =  3,
	TNG_LED_CHANNEL_4  =  4,
	TNG_LED_CHANNEL_5  =  5,
	TNG_LED_CHANNEL_6  =  6,
	TNG_LED_CHANNEL_7  =  7,
	TNG_LED_CHANNEL_8  =  8,
	TNG_LED_CHANNEL_9  =  9,
	TNG_LED_CHANNEL_10 = 10,
	TNG_LED_CHANNEL_11 = 11
} TNGLEDChannel;


void tng_led_status_set(const uint8_t r, const uint8_t g, const uint8_t b);
void tng_led_channel_set(const TNGLEDChannel channel, const bool enable);

void tng_led_tick(void);
void tng_led_init(void);

#endif