/* bricklib2
 * Copyright (C) 2017 Olaf Lüke <olaf@tinkerforge.com>
 *
 * moving_average.c: Simple moving average implementation
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

#include "moving_average.h"

#include "bricklib2/utility/util_definitions.h"

moving_average_t moving_average_get(const MovingAverage *const moving_average) {
	// We do proper rounding instead of normal division truncate

#if MOVING_AVERAGE_SUM_TYPE == MOVING_AVERAGE_TYPE_INT8 || MOVING_AVERAGE_SUM_TYPE == MOVING_AVERAGE_TYPE_INT16 || MOVING_AVERAGE_SUM_TYPE == MOVING_AVERAGE_TYPE_INT32
	// In the case that the sum can be negative we have to handle the negative case separately
	if(moving_average->sum < 0) {
		return (moving_average->sum - moving_average->length/2) / moving_average->length;
	}
#endif

	return (moving_average->sum + moving_average->length/2) / moving_average->length;
}

void moving_average_new_length(MovingAverage *const moving_average, const uint32_t length) {
	const uint32_t new_length = BETWEEN(1, length, MOVING_AVERAGE_MAX_LENGTH);
	if(new_length == moving_average->length) {
		return;
	}

	moving_average_init(moving_average, moving_average_get(moving_average), new_length);
}

void moving_average_init(MovingAverage *const moving_average, const moving_average_t start_value, const uint32_t length) {
	const uint32_t new_length = BETWEEN(1, length, MOVING_AVERAGE_MAX_LENGTH);

	moving_average->index = 0;
	moving_average->length = new_length;

	moving_average->sum = start_value*new_length;

	for(uint32_t i = 0; i < new_length; i++) {
		moving_average->values[i] = start_value;
	}
}

moving_average_t moving_average_handle_value(MovingAverage *const moving_average, const moving_average_t new_value) {
	moving_average->sum -= moving_average->values[moving_average->index];
	moving_average->values[moving_average->index] = new_value;
	moving_average->sum += new_value;

	moving_average->index = moving_average->index + 1;
	if(moving_average->index >= moving_average->length) {
		moving_average->index = 0;
	}

	return moving_average_get(moving_average);
}
