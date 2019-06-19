/* tng
 * Copyright (C) 2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * communication.h: TFP protocol message handling
 *                  for common TNG messages
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

#ifndef TNG_COMMUNICATION_H
#define TNG_COMMUNICATION_H

#include <stdint.h>
#include <stdbool.h>

#include "bricklib2/protocols/tfp/tfp.h"
#include "bricklib2/tng/tng.h"

TNGHandleMessageResponse tng_handle_message(const void *data, void *response);

#define TNG_ENUMERATE_CALLBACK_UID_LENGTH     8
#define TNG_ENUMERATE_CALLBACK_VERSION_LENGTH 3

#define TNG_ENUMERATE_TYPE_AVAILABLE 0
#define TNG_ENUMERATE_TYPE_ADDED     1
#define TNG_ENUMERATE_TYPE_REMOVED   2

#define TNG_WRITE_FIRMWARE_CHUNK_SIZE 64

#define TNG_WRITE_FIRMWARE_STATUS_OK              0
#define TNG_WRITE_FIRMWARE_STATUS_INVALID_POINTER 1

// Function and callback IDs and structs
#define TNG_FID_SET_WRITE_FIRMWARE_POINTER 237
#define TNG_FID_WRITE_FIRMWARE 238
#define TNG_FID_RESET 243
#define TNG_FID_WRITE_UID 248
#define TNG_FID_READ_UID 249
#define TNG_FID_ENUMERATE_CALLBACK 253
#define TNG_FID_ENUMERATE 254
#define TNG_FID_GET_IDENTITY 255

typedef struct {
	TFPMessageHeader header;
	uint32_t pointer;
} __attribute__((__packed__)) TNGSetWriteFirmwarePointer;

typedef struct {
	TFPMessageHeader header;
	uint8_t data[TNG_WRITE_FIRMWARE_CHUNK_SIZE];
} __attribute__((__packed__)) TNGWriteFirmware;

typedef struct {
	TFPMessageHeader header;
	uint8_t status;
} __attribute__((__packed__)) TNGWriteFirmware_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) TNGReset;

typedef struct {
	TFPMessageHeader header;
	uint32_t uid;
} __attribute__((__packed__)) TNGWriteUID;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) TNGReadUID;

typedef struct {
	TFPMessageHeader header;
	uint32_t uid;
} __attribute__((__packed__)) TNGReadUID_Response;

typedef struct {
	TFPMessageHeader header;
	char uid[TNG_ENUMERATE_CALLBACK_UID_LENGTH];
	char connected_uid[TNG_ENUMERATE_CALLBACK_UID_LENGTH];
	char position;
	uint8_t version_hw[TNG_ENUMERATE_CALLBACK_VERSION_LENGTH];
	uint8_t version_fw[TNG_ENUMERATE_CALLBACK_VERSION_LENGTH];
	uint16_t device_identifier;
	uint8_t enumeration_type;
} __attribute__((__packed__)) TNGEnumerate_Callback;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) TNGEnumerate;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) TNGGetIdentity;

typedef struct {
	TFPMessageHeader header;
	char uid[TNG_ENUMERATE_CALLBACK_UID_LENGTH];
	char connected_uid[TNG_ENUMERATE_CALLBACK_UID_LENGTH];
	char position;
	uint8_t version_hw[TNG_ENUMERATE_CALLBACK_VERSION_LENGTH];
	uint8_t version_fw[TNG_ENUMERATE_CALLBACK_VERSION_LENGTH];
	uint16_t device_identifier;
} __attribute__((__packed__)) TNGGetIdentity_Response;


// Function prototypes
TNGHandleMessageResponse tng_set_write_firmware_pointer(const TNGSetWriteFirmwarePointer *data);
TNGHandleMessageResponse tng_write_firmware(const TNGWriteFirmware *data, TNGWriteFirmware_Response *response);
TNGHandleMessageResponse tng_reset(const TNGReset *data);
TNGHandleMessageResponse tng_read_uid(const TNGWriteUID *data, TNGReadUID_Response *response);
TNGHandleMessageResponse tng_write_uid(const TNGWriteUID *data);
TNGHandleMessageResponse tng_get_identity(const TNGGetIdentity *data, TNGGetIdentity_Response *response);
TNGHandleMessageResponse tng_enumerate(const TNGEnumerate *data, TNGEnumerate_Callback *response);

#endif