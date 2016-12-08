/* bricklib2
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * communication_callback.c: Helper functions for Bricklet communication
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

#include "communication_callback.h"

#include "bricklib2/hal/system_timer/system_timer.h"

static communication_callback_handler_t communication_callbacks[COMMUNICATION_CALLBACK_HANDLER_NUM] = {
	COMMUNICATION_CALLBACK_LIST_INIT
};

void communication_callback_tick(void) {
	static uint32_t last_tick = 0;

	if(!system_timer_is_time_elapsed_ms(last_tick, COMMUNICATION_CALLBACK_TICK_WAIT_MS)) {
		return;
	}
	last_tick = system_timer_get_ms();

	uint32_t cb_index = 0;
	for(uint32_t _ = 0; _ < COMMUNICATION_CALLBACK_HANDLER_NUM; _++) {
		if(communication_callbacks[cb_index]()) {
			communication_callback_handler_t current = communication_callbacks[cb_index];
			for(uint32_t i = cb_index; i < COMMUNICATION_CALLBACK_HANDLER_NUM-1; i++) {
				communication_callbacks[i] = communication_callbacks[i+1];
			}
			communication_callbacks[COMMUNICATION_CALLBACK_HANDLER_NUM-1] = current;
		} else {
			cb_index++;
		}
	}
}

void communication_callback_init(void) {

}
