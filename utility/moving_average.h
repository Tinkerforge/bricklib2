/* bricklib2
 * Copyright (C) 2017 Olaf Lüke <olaf@tinkerforge.com>
 *
 * moving_average.h: Simple moving average implementation
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

#ifndef MOVING_AVERAGE
#define MOVING_AVERAGE

#include "configs/config.h"

#include <stdint.h>


#ifndef MOVING_AVERAGE_MAX_LENGTH
#define MOVING_AVERAGE_MAX_LENGTH 100
#endif

#ifndef MOVING_AVERAGE_DEFAULT_LENGTH
#define MOVING_AVERAGE_DEFAULT_LENGTH 10
#endif

#define MOVING_AVERAGE_TYPE_INT8   0
#define MOVING_AVERAGE_TYPE_UINT8  1
#define MOVING_AVERAGE_TYPE_INT16  2
#define MOVING_AVERAGE_TYPE_UINT16 3
#define MOVING_AVERAGE_TYPE_INT32  4
#define MOVING_AVERAGE_TYPE_UINT32 5

#ifndef MOVING_AVERAGE_TYPE
#define MOVING_AVERAGE_TYPE MOVING_AVERAGE_TYPE_INT16
#endif

#if MOVING_AVERAGE_TYPE == MOVING_AVERAGE_TYPE_INT8
typedef int8_t moving_average_t;
#elif MOVING_AVERAGE_TYPE == MOVING_AVERAGE_TYPE_UINT8
typedef uint8_t moving_average_t;
#elif MOVING_AVERAGE_TYPE == MOVING_AVERAGE_TYPE_INT16
typedef int16_t moving_average_t;
#elif MOVING_AVERAGE_TYPE == MOVING_AVERAGE_TYPE_UINT16
typedef uint16_t moving_average_t;
#elif MOVING_AVERAGE_TYPE == MOVING_AVERAGE_TYPE_INT32
typedef int32_t moving_average_t;
#elif MOVING_AVERAGE_TYPE == MOVING_AVERAGE_TYPE_UINT32
typedef uint32_t moving_average_t;
#endif


#ifndef MOVING_AVERAGE_SUM_TYPE
#define MOVING_AVERAGE_SUM_TYPE MOVING_AVERAGE_TYPE_INT32
#endif

#if MOVING_AVERAGE_SUM_TYPE == MOVING_AVERAGE_TYPE_INT8
typedef int8_t moving_average_sum_t;
#elif MOVING_AVERAGE_SUM_TYPE == MOVING_AVERAGE_TYPE_UINT8
typedef uint8_t moving_average_sum_t;
#elif MOVING_AVERAGE_SUM_TYPE == MOVING_AVERAGE_TYPE_INT16
typedef int16_t moving_average_sum_t;
#elif MOVING_AVERAGE_SUM_TYPE == MOVING_AVERAGE_TYPE_UINT16
typedef uint16_t moving_average_sum_t;
#elif MOVING_AVERAGE_SUM_TYPE == MOVING_AVERAGE_TYPE_INT32
typedef int32_t moving_average_sum_t;
#elif MOVING_AVERAGE_SUM_TYPE == MOVING_AVERAGE_TYPE_UINT32
typedef uint32_t moving_average_sum_t;
#endif

typedef struct {
	uint32_t length;
	uint32_t index;

	moving_average_sum_t sum;
	moving_average_t values[MOVING_AVERAGE_MAX_LENGTH];
} MovingAverage;

moving_average_t moving_average_get(const MovingAverage *const moving_average);
void moving_average_new_length(MovingAverage *const moving_average, const uint32_t length);
void moving_average_init(MovingAverage *const moving_average, const moving_average_t start_value, const uint32_t length);
moving_average_t moving_average_handle_value(MovingAverage *const moving_average, const moving_average_t new_value);

#endif
