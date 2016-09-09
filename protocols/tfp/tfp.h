/* brickletboot
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tfp.h: Tinkerforge Protocol (TFP) functions
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

#ifndef TFP_H
#define TFP_H

#include <stdint.h>
#include <stdbool.h>

#define TFP_MESSAGE_MIN_LENGTH 8
#define TFP_MESSAGE_MAX_LENGTH 80

#define TFP_MESSAGE_ERROR_CODE_OK 0
#define TFP_MESSAGE_ERROR_CODE_INVALID_PARAMETER 1
#define TFP_MESSAGE_ERROR_CODE_NOT_SUPPORTED 2


typedef struct {
	uint32_t uid;
	uint8_t length;
	uint8_t fid;
	uint8_t other_options:2,
	        authentication:1,
	        return_expected:1,
			sequence_num:4;
	uint8_t future_use:6,
	        error:2;
} __attribute__((__packed__)) TFPMessageHeader;

bool tfp_is_return_expected(const void *message);
uint8_t tfp_get_fid_from_message(const void *message);
uint8_t tfp_get_length_from_message(const void *message);
uint32_t tfp_get_uid_from_message(const void *message);
void tfp_uid_uint32_to_base58(uint32_t value, char *str);

#endif
