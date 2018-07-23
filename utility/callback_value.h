/* bricklib2
 * Copyright (C) 2017 Olaf Lüke <olaf@tinkerforge.com>
 * Copyright (C) 2018 Ishraq Ibne Ashraf <ishraq@tinkerforge.com>
 * Copyright (C) 2018 Matthias Bolte <matthias@tinkerforge.com>
 *
 * callback_value.h: Helper functions for Bricklet communication
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

// no header guard, this file is supposed to be includable multiple times with
// different CALLBACK_VALUE_TYPEs

#include <stdint.h>
#include <stdbool.h>

#include "bricklib2/protocols/tfp/tfp.h"
#include "bricklib2/bootloader/bootloader.h"
#include "bricklib2/hal/system_timer/system_timer.h"

#define CALLBACK_VALUE_TYPE_INT8   0
#define CALLBACK_VALUE_TYPE_UINT8  1
#define CALLBACK_VALUE_TYPE_INT16  2
#define CALLBACK_VALUE_TYPE_UINT16 3
#define CALLBACK_VALUE_TYPE_INT32  4
#define CALLBACK_VALUE_TYPE_UINT32 5

#undef CALLBACK_VALUE_T
#undef CALLBACK_VALUE_MIN
#undef CALLBACK_VALUE_MAX

#ifndef CALLBACK_VALUE_TYPE
#error CALLBACK_VALUE_TYPE is undefined
#elif CALLBACK_VALUE_TYPE == CALLBACK_VALUE_TYPE_INT8
#define CALLBACK_VALUE_T int8_t
#define CALLBACK_VALUE_MIN INT8_MIN
#define CALLBACK_VALUE_MAX INT8_MAX
#elif CALLBACK_VALUE_TYPE == CALLBACK_VALUE_TYPE_UINT8
#define CALLBACK_VALUE_T uint8_t
#define CALLBACK_VALUE_MIN 0
#define CALLBACK_VALUE_MAX UINT8_MAX
#elif CALLBACK_VALUE_TYPE == CALLBACK_VALUE_TYPE_INT16
#define CALLBACK_VALUE_T int16_t
#define CALLBACK_VALUE_MIN INT16_MIN
#define CALLBACK_VALUE_MAX INT16_MAX
#elif CALLBACK_VALUE_TYPE == CALLBACK_VALUE_TYPE_UINT16
#define CALLBACK_VALUE_T uint16_t
#define CALLBACK_VALUE_MIN 0
#define CALLBACK_VALUE_MAX UINT16_MAX
#elif CALLBACK_VALUE_TYPE == CALLBACK_VALUE_TYPE_INT32
#define CALLBACK_VALUE_T int32_t
#define CALLBACK_VALUE_MIN INT32_MIN
#define CALLBACK_VALUE_MAX INT32_MAX
#elif CALLBACK_VALUE_TYPE == CALLBACK_VALUE_TYPE_UINT32
#define CALLBACK_VALUE_T uint32_t
#define CALLBACK_VALUE_MIN 0
#define CALLBACK_VALUE_MAX UINT32_MAX
#else
#error CALLBACK_VALUE_TYPE is invalid
#endif

// https://stackoverflow.com/questions/1489932/how-to-concatenate-twice-with-the-c-preprocessor-and-expand-a-macro-as-in-arg
#define CALLBACK_VALUE_SUFFIX_PASTER(x, y) x ## _ ## y
#define CALLBACK_VALUE_SUFFIX_EVALUATOR(x, y) CALLBACK_VALUE_SUFFIX_PASTER(x, y)
#define CALLBACK_VALUE_SUFFIX(name) CALLBACK_VALUE_SUFFIX_EVALUATOR(name, CALLBACK_VALUE_T)

#ifdef CALLBACK_VALUE_CHANNEL_NUM
typedef CALLBACK_VALUE_T (*CALLBACK_VALUE_SUFFIX(CallbackValueGetter))(uint8_t channel);
#else
typedef CALLBACK_VALUE_T (*CALLBACK_VALUE_SUFFIX(CallbackValueGetter))(void);
#endif

typedef struct {
	CALLBACK_VALUE_SUFFIX(CallbackValueGetter) get_callback_value;
	CALLBACK_VALUE_T value_last;

	uint32_t period;
	uint32_t last_time;
	bool value_has_to_change;

	CALLBACK_VALUE_T threshold_min;
	CALLBACK_VALUE_T threshold_max;
	char threshold_option;

	CALLBACK_VALUE_T threshold_min_user;
	CALLBACK_VALUE_T threshold_max_user;
	char threshold_option_user;
} CALLBACK_VALUE_SUFFIX(CallbackValue);

typedef struct {
	TFPMessageHeader header;
#ifdef CALLBACK_VALUE_CHANNEL_NUM
	uint8_t channel;
#endif
} __attribute__((__packed__)) CALLBACK_VALUE_SUFFIX(GetCallbackValue);

typedef struct {
	TFPMessageHeader header;
	CALLBACK_VALUE_T value;
} __attribute__((__packed__)) CALLBACK_VALUE_SUFFIX(GetCallbackValue_Response);

typedef struct {
	TFPMessageHeader header;
#ifdef CALLBACK_VALUE_CHANNEL_NUM
	uint8_t channel;
#endif
	uint32_t period;
	bool value_has_to_change;
	char option;
	CALLBACK_VALUE_T min;
	CALLBACK_VALUE_T max;
} __attribute__((__packed__)) CALLBACK_VALUE_SUFFIX(SetCallbackValueCallbackConfiguration);

typedef struct {
	TFPMessageHeader header;
#ifdef CALLBACK_VALUE_CHANNEL_NUM
	uint8_t channel;
#endif
} __attribute__((__packed__)) CALLBACK_VALUE_SUFFIX(GetCallbackValueCallbackConfiguration);

typedef struct {
	TFPMessageHeader header;
	uint32_t period;
	bool value_has_to_change;
	char option;
	CALLBACK_VALUE_T min;
	CALLBACK_VALUE_T max;
} __attribute__((__packed__)) CALLBACK_VALUE_SUFFIX(GetCallbackValueCallbackConfiguration_Response);

typedef struct {
	TFPMessageHeader header;
#ifdef CALLBACK_VALUE_CHANNEL_NUM
	uint8_t channel;
#endif
	CALLBACK_VALUE_T value;
} __attribute__((__packed__)) CALLBACK_VALUE_SUFFIX(CallbackValue_Callback);

void CALLBACK_VALUE_SUFFIX(callback_value_init)(CALLBACK_VALUE_SUFFIX(CallbackValue) *callback_values,
                                                CALLBACK_VALUE_SUFFIX(CallbackValueGetter) callback_value_getter) {
#ifdef CALLBACK_VALUE_CHANNEL_NUM
	for (uint8_t channel = 0; channel < CALLBACK_VALUE_CHANNEL_NUM; ++channel) {
		CALLBACK_VALUE_SUFFIX(CallbackValue) *callback_value = &callback_values[channel];
#else
		CALLBACK_VALUE_SUFFIX(CallbackValue) *callback_value = callback_values;
#endif

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
#ifdef CALLBACK_VALUE_CHANNEL_NUM
	}
#endif
}

BootloaderHandleMessageResponse
CALLBACK_VALUE_SUFFIX(get_callback_value)(const CALLBACK_VALUE_SUFFIX(GetCallbackValue) *data,
                                          CALLBACK_VALUE_SUFFIX(GetCallbackValue_Response) *response,
                                          CALLBACK_VALUE_SUFFIX(CallbackValue) *callback_values) {
#ifdef CALLBACK_VALUE_CHANNEL_NUM
	if (data->channel >= CALLBACK_VALUE_CHANNEL_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	const CALLBACK_VALUE_T value_current = callback_values[data->channel].get_callback_value(data->channel);
#else
	const CALLBACK_VALUE_T value_current = callback_values->get_callback_value();
#endif

	response->header.length = sizeof(CALLBACK_VALUE_SUFFIX(GetCallbackValue_Response));
	response->value         = value_current;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse
CALLBACK_VALUE_SUFFIX(set_callback_value_callback_configuration)(const CALLBACK_VALUE_SUFFIX(SetCallbackValueCallbackConfiguration) *data,
                                                                 CALLBACK_VALUE_SUFFIX(CallbackValue) *callback_values) {
#ifdef CALLBACK_VALUE_CHANNEL_NUM
	if (data->channel >= CALLBACK_VALUE_CHANNEL_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	CALLBACK_VALUE_SUFFIX(CallbackValue) *callback_value = &callback_values[data->channel];
#else
	CALLBACK_VALUE_SUFFIX(CallbackValue) *callback_value = callback_values;
#endif

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
		callback_value->threshold_min = CALLBACK_VALUE_MIN;
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

BootloaderHandleMessageResponse
CALLBACK_VALUE_SUFFIX(get_callback_value_callback_configuration)(const CALLBACK_VALUE_SUFFIX(GetCallbackValueCallbackConfiguration) *data,
                                                                 CALLBACK_VALUE_SUFFIX(GetCallbackValueCallbackConfiguration_Response) *response,
                                                                 CALLBACK_VALUE_SUFFIX(CallbackValue) *callback_values) {
#ifdef CALLBACK_VALUE_CHANNEL_NUM
	if (data->channel >= CALLBACK_VALUE_CHANNEL_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	CALLBACK_VALUE_SUFFIX(CallbackValue) *callback_value = &callback_values[data->channel];
#else
	CALLBACK_VALUE_SUFFIX(CallbackValue) *callback_value = callback_values;
#endif

	response->header.length       = sizeof(CALLBACK_VALUE_SUFFIX(GetCallbackValueCallbackConfiguration_Response));
	response->period              = callback_value->period;
	response->value_has_to_change = callback_value->value_has_to_change;
	response->option              = callback_value->threshold_option_user;
	response->min                 = callback_value->threshold_min_user;
	response->max                 = callback_value->threshold_max_user;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

bool CALLBACK_VALUE_SUFFIX(handle_callback_value_callback)(CALLBACK_VALUE_SUFFIX(CallbackValue) *callback_values,
#ifdef CALLBACK_VALUE_CHANNEL_NUM
                                                           const uint8_t channel,
#endif
                                                           const uint8_t fid) {
	static bool is_buffered = false;
	static CALLBACK_VALUE_SUFFIX(CallbackValue_Callback) cb;

	if(!is_buffered) {
#ifdef CALLBACK_VALUE_CHANNEL_NUM
		CALLBACK_VALUE_SUFFIX(CallbackValue) *callback_value = &callback_values[channel];
		const CALLBACK_VALUE_T value_current = callback_values->get_callback_value(channel);
#else
		CALLBACK_VALUE_SUFFIX(CallbackValue) *callback_value = callback_values;
		const CALLBACK_VALUE_T value_current = callback_values->get_callback_value();
#endif

		tfp_make_default_header(&cb.header, bootloader_get_uid(), sizeof(CALLBACK_VALUE_SUFFIX(CallbackValue_Callback)), fid);

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
		   ((value_current >= callback_value->threshold_min) &&
		    (value_current <= callback_value->threshold_max))) {
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
#ifdef CALLBACK_VALUE_CHANNEL_NUM
		cb.channel                 = channel;
#endif
		cb.value                   = value_current;
	}

	if(bootloader_spitfp_is_send_possible(&bootloader_status.st)) {
		bootloader_spitfp_send_ack_and_message(&bootloader_status, (uint8_t*)&cb, sizeof(CALLBACK_VALUE_SUFFIX(CallbackValue_Callback)));
		is_buffered = false;
		return true;
	} else {
		is_buffered = true;
	}

	return false;
}
