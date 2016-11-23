/* bricklib2
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * led_flicker.c: Functions for fancy LED flickering
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

#include "led_flicker.h"

#include "bricklib2/hal/system_timer/system_timer.h"

void led_flicker_tick(LEDFlickerState *led_flicker_state, uint32_t current_time, XMC_GPIO_PORT_t *const port, const uint8_t pin) {
	if(led_flicker_state->config == LED_FLICKER_CONFIG_ACTIVE) {
		if(led_flicker_state->start > 0) {
			if((current_time - led_flicker_state->start) >= LED_FLICKER_OFFTIME_MAX) {
				XMC_GPIO_SetOutputLow(port, pin);
				led_flicker_state->start = 0;
			}
		} else if(led_flicker_state->counter > LED_FLICKER_COUNTER_MAX) {
			XMC_GPIO_SetOutputHigh(port, pin);
			led_flicker_state->start = current_time;
			led_flicker_state->counter = 0;
		}
	}
}
