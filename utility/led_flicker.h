/* bricklib2
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * led_flicker.h: Functions for fancy LED flickering
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

#ifndef LED_FLICKER
#define LED_FLICKER

#include <stdint.h>

#include "xmc_gpio.h"

#define LED_FLICKER_CONFIG_OFF          0
#define LED_FLICKER_CONFIG_ON           1
#define LED_FLICKER_CONFIG_HEARTBEAT    2
#define LED_FLICKER_CONFIG_STATUS       3
#define LED_FLICKER_CONFIG_EXTERNAL     4

#define LED_FLICKER_STATUS_COUNTER_MAX  40 // flicker for every 40 packets
#define LED_FLICKER_STATUS_ONTIME_MIN   35 // flicker off for 35ms
#define LED_FLICKER_STATUS_OFFTIME_MAX  35 // flicker off for 35ms

#define LED_FLICKER_HEARTBEAT_DURATION  1000 // heartbeat once per second
#define LED_FLICKER_HEARTBEAT_ONTIME    100  // ontime of heartbeat 100ms
#define LED_FLICKER_HEARTBEAT_OFFTIME   60   // offtime of heartbeat 60ms

typedef struct {
	uint32_t start;
	uint8_t  counter;
	uint8_t  config;
} LEDFlickerState;

void led_flicker_tick(LEDFlickerState *led_flicker_state, uint32_t current_time, XMC_GPIO_PORT_t *const port, const uint8_t pin);
void led_flicker_increase_counter(LEDFlickerState *led_flicker_state);

#endif
