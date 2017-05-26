/* bricklib2
 * Copyright (C) 2017 Olaf Lüke <olaf@tinkerforge.com>
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

#ifndef CALLBACK_VALUE
#define CALLBACK_VALUE

#include "configs/config.h"

#include "bricklib2/protocols/tfp/tfp.h"
#include "bricklib2/bootloader/bootloader.h"


#define CALLBACK_VALUE_TYPE_INT8   0
#define CALLBACK_VALUE_TYPE_UINT8  1
#define CALLBACK_VALUE_TYPE_INT16  2
#define CALLBACK_VALUE_TYPE_UINT16 3
#define CALLBACK_VALUE_TYPE_INT32  4
#define CALLBACK_VALUE_TYPE_UINT32 5

#ifndef CALLBACK_VALUE_TYPE
#define CALLBACK_VALUE_TYPE CALLBACK_VALUE_TYPE_INT16
#endif


#if CALLBACK_VALUE_TYPE == CALLBACK_VALUE_TYPE_INT8
typedef int8_t callback_value_t;
#define CALLBACK_VALUE_MIN INT8_MIN
#define CALLBACK_VALUE_MAX INT8_MAX
#elif CALLBACK_VALUE_TYPE == CALLBACK_VALUE_TYPE_UINT8
typedef uint8_t callback_value_t;
#define CALLBACK_VALUE_MIN UINT8_MIN
#define CALLBACK_VALUE_MAX UINT8_MAX
#elif CALLBACK_VALUE_TYPE == CALLBACK_VALUE_TYPE_INT16
typedef int16_t callback_value_t;
#define CALLBACK_VALUE_MIN INT16_MIN
#define CALLBACK_VALUE_MAX INT16_MAX
#elif CALLBACK_VALUE_TYPE == CALLBACK_VALUE_TYPE_UINT16
typedef uint16_t callback_value_t;
#define CALLBACK_VALUE_MIN UINT16_MIN
#define CALLBACK_VALUE_MAX UINT16_MAX
#elif CALLBACK_VALUE_TYPE == CALLBACK_VALUE_TYPE_INT32
typedef int32_t callback_value_t;
#define CALLBACK_VALUE_MIN INT32_MIN
#define CALLBACK_VALUE_MAX INT32_MAX
#elif CALLBACK_VALUE_TYPE == CALLBACK_VALUE_TYPE_UINT32
typedef uint32_t callback_value_t;
#define CALLBACK_VALUE_MIN UINT32_MIN
#define CALLBACK_VALUE_MAX UINT32_MAX
#endif

typedef callback_value_t (*CallbackValueGetter)(void);

typedef struct {
	CallbackValueGetter get_callback_value;
	callback_value_t value_last;

	uint32_t period;
	uint32_t last_time;
	bool value_has_to_change;

	callback_value_t threshold_min;
	callback_value_t threshold_max;
	char threshold_option;

	callback_value_t threshold_min_user;
	callback_value_t threshold_max_user;
	char threshold_option_user;
} CallbackValue;


typedef struct {
	TFPMessageHeader header;
	uint32_t debounce;
} __attribute__((__packed__)) SetDebouncePeriod;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetDebouncePeriod;

typedef struct {
	TFPMessageHeader header;
	uint32_t debounce;
} __attribute__((__packed__)) GetDebouncePeriod_Response;


typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetCallbackValue;

typedef struct {
	TFPMessageHeader header;
	int32_t value;
} __attribute__((__packed__)) GetCallbackValue_Response;

typedef struct {
	TFPMessageHeader header;
	uint32_t period;
	bool value_has_to_change;
	char option;
	uint16_t min;
	uint16_t max;
} __attribute__((__packed__)) SetCallbackValueCallbackConfiguration;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetCallbackValueCallbackConfiguration;

typedef struct {
	TFPMessageHeader header;
	uint32_t period;
	bool value_has_to_change;
	char option;
	uint16_t min;
	uint16_t max;
} __attribute__((__packed__)) GetCallbackValueCallbackConfiguration_Response;

typedef struct {
	TFPMessageHeader header;
	callback_value_t value;
} __attribute__((__packed__)) CallbackValue_Callback;


void callback_value_init(CallbackValue *callback_value, CallbackValueGetter callback_value_getter);

BootloaderHandleMessageResponse set_debounce_period(const SetDebouncePeriod *data);
BootloaderHandleMessageResponse get_debounce_period(const GetDebouncePeriod *data, GetDebouncePeriod_Response *response);

BootloaderHandleMessageResponse get_callback_value(const GetCallbackValue *data, GetCallbackValue_Response *response, CallbackValue *callback_value);
BootloaderHandleMessageResponse set_callback_value_callback_configuration(const SetCallbackValueCallbackConfiguration *data, CallbackValue *callback_value);
BootloaderHandleMessageResponse get_callback_value_callback_configuration(const GetCallbackValueCallbackConfiguration *data, GetCallbackValueCallbackConfiguration_Response *response, CallbackValue *callback_value);

bool handle_callback_value_callback(CallbackValue *callback_value, const uint8_t fid);

#endif
