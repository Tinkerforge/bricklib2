/* brickletboot
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tfp.c: Tinkerforge Protocol (TFP) functions
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

#include "tfp.h"

#define TFP_BASE58_STR_SIZE 8

const char TFP_BASE58_STR[] = "123456789abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ";

bool tfp_is_return_expected(const void *message) {
	return ((TFPMessageHeader*)message)->return_expected;
}

uint8_t tfp_get_length_from_message(const void *message) {
	return ((TFPMessageHeader*)message)->length;
}

uint8_t tfp_get_fid_from_message(const void *message) {
	return ((TFPMessageHeader*)message)->fid;
}

uint32_t tfp_get_uid_from_message(const void *message) {
	return ((TFPMessageHeader*)message)->uid;
}

void tfp_uid_uint32_to_base58(uint32_t value, char *str) {
	char reverse_str[TFP_BASE58_STR_SIZE] = {'\0'};
	uint8_t i = 0;
	while(value >= 58) {
		uint32_t mod = value % 58;
		reverse_str[i] = TFP_BASE58_STR[mod];
		value = value/58;
		i++;
	}

	reverse_str[i] = TFP_BASE58_STR[value];

	uint8_t j = 0;
	for(j = 0; j <= i; j++) {
		str[j] = reverse_str[i-j];
	}
	for(; j < TFP_BASE58_STR_SIZE; j++) {
		str[j] = '\0';
	}
}

void tfp_make_default_header(TFPMessageHeader *header, const uint32_t uid, const uint8_t length, const uint8_t fid) {
	header->uid              = uid;
	header->length           = length;
	header->fid              = fid;
	header->sequence_num     = 0; // Sequence number for callback is 0
	header->return_expected  = 1;
	header->authentication   = 0; // TODO
	header->other_options    = 0;
	header->error            = 0;
	header->future_use       = 0;
}
