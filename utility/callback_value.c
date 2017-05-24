/* bricklib2
 * Copyright (C) 2017 Olaf Lüke <olaf@tinkerforge.com>
 *
 * callback_value.c: Helper functions for Bricklet communication
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

#include "callback_value.h"

#include <stdint.h>
#include <stdbool.h>

#include "bricklib2/hal/system_timer/system_timer.h"

#define CALLBACK_VALUE_DEBOUNCE_PERIOD_DEFAULT 100

void callback_value_init(CallbackValue *callback_value, CallbackValueGetter callback_value_getter) {
	callback_value->get_callback_value    = callback_value_getter;
	callback_value->value_last            = CALLBACK_VALUE_MAX;

	callback_value->period                = 0;
	callback_value->last_time             = 0;
	callback_value->value_has_to_change   = false;

	callback_value->threshold_min         = 0;
	callback_value->threshold_max         = 0;
	callback_value->threshold_option      = 'x';

	callback_value->threshold_min_user    = 0;
	callback_value->threshold_max_user    = 0;
	callback_value->threshold_option_user = 'x';
}

BootloaderHandleMessageResponse get_callback_value(const GetCallbackValue *data, GetCallbackValue_Response *response, CallbackValue *callback_value) {
	const callback_value_t value_current = callback_value->get_callback_value();

	response->header.length = sizeof(GetCallbackValue_Response);
	response->value = value_current;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_callback_value_callback_configuration(const SetCallbackValueCallbackConfiguration *data, CallbackValue *callback_value) {
	if(data->option == 'o' || data->option == 'i' || data->option == 'x') {
		callback_value->threshold_option = data->option;
		callback_value->threshold_min = data->min;
		callback_value->threshold_max = data->max;
	} else if(data->option == '<') {
		callback_value->threshold_option = 'o';
		callback_value->threshold_min = data->min;
		callback_value->threshold_max = CALLBACK_VALUE_MAX;
	} else if(data->option == '>') {
		callback_value->threshold_option = 'o';
		callback_value->threshold_min = -CALLBACK_VALUE_MIN;
		callback_value->threshold_max = data->min;
	} else {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	callback_value->threshold_option_user = data->option;
	callback_value->threshold_min_user = data->min;
	callback_value->threshold_max_user = data->max;

	callback_value->period = data->period;
	callback_value->value_has_to_change = data->value_has_to_change;

	// We make sure to always send the first piece of data
	// immediately after a new callback configuration
	callback_value->last_time = system_timer_get_ms() - data->period;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_callback_value_callback_configuration(const GetCallbackValueCallbackConfiguration *data, GetCallbackValueCallbackConfiguration_Response *response, CallbackValue *callback_value) {
	response->header.length = sizeof(GetCallbackValueCallbackConfiguration_Response);
	response->period = callback_value->period;
	response->value_has_to_change = callback_value->value_has_to_change;
	response->option = callback_value->threshold_option_user;
	response->min = callback_value->threshold_min_user;
	response->max = callback_value->threshold_min_user;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

bool handle_callback_value_callback(CallbackValue *callback_value, const uint8_t fid) {
	static bool is_buffered = false;
	static CallbackValue_Callback cb;

	if(!is_buffered) {
		const callback_value_t value_current = callback_value->get_callback_value();

		tfp_make_default_header(&cb.header, bootloader_get_uid(), sizeof(CallbackValue_Callback), fid);

		// If period is 0 callback is off
		// If period > 0 and not elapsed we don't send
		if((callback_value->period == 0) ||
		   (!system_timer_is_time_elapsed_ms(callback_value->last_time, callback_value->period))) {
			return false;
		}

		// If value has to change and current value is equal to last value we don't send
		if(callback_value->value_has_to_change &&
		   (value_current == callback_value->value_last)) {
			return false;
		}

		// If outside-threshold is defined but not fulfilled we son't send
		if((callback_value->threshold_option == 'o') &&
		   ((value_current > callback_value->threshold_min) &&
		    (value_current < callback_value->threshold_max))) {
			return false;
		}

		// If inside-threshold is defined but not fulfilled we son't send
		if((callback_value->threshold_option == 'i') &&
		   ((value_current < callback_value->threshold_min) ||
		    (value_current > callback_value->threshold_max))) {
			return false;
		}

		// If none of the above returned, we know that a callback is triggered
		callback_value->last_time  = system_timer_get_ms();
		callback_value->value_last = value_current;
		cb.value                   = value_current;
	}

	if(bootloader_spitfp_is_send_possible(&bootloader_status.st)) {
		bootloader_spitfp_send_ack_and_message(&bootloader_status, (uint8_t*)&cb, sizeof(CallbackValue_Callback));
		is_buffered = false;
		return true;
	} else {
		is_buffered = true;
	}

	return false;
}
