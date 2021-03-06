/* tng
 * Copyright (C) 2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * communication.c: TFP protocol message handling
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

#include "tng_communication.h"

#include "configs/config.h"

#include "bricklib2/utility/communication_callback.h"
#include "bricklib2/protocols/tfp/tfp.h"
#include "bricklib2/logging/logging.h"
#include "bricklib2/hal/system_timer/system_timer.h"

#include "bricklib2/tng/tng.h"
#include "bricklib2/tng/tng_firmware.h"

#include "bricklib2/tng/usb_stm32/usb.h"

extern const uint32_t device_identifier;

static uint32_t tng_firmware_pointer = 0;

TNGHandleMessageResponse tng_handle_message(const void *message, void *response) {
	switch(tfp_get_fid_from_message(message)) {
		case TNG_FID_GET_TIMESTAMP: return tng_get_timestamp(message, response);
		case TNG_FID_COPY_FIRMWARE: return tng_copy_firmware(message, response);
		case TNG_FID_SET_WRITE_FIRMWARE_POINTER: return tng_set_write_firmware_pointer(message);
		case TNG_FID_WRITE_FIRMWARE: return tng_write_firmware(message, response);
		case TNG_FID_RESET: return tng_reset(message);
		case TNG_FID_READ_UID: return tng_read_uid(message, response);
		case TNG_FID_WRITE_UID: return tng_write_uid(message);
		case TNG_FID_ENUMERATE: return tng_enumerate(message, response);
		case TNG_FID_GET_IDENTITY: return tng_get_identity(message, response);
		default: return HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED;
	}
}

TNGHandleMessageResponse tng_get_timestamp(const TNGGetTimestamp *data, TNGGetTimestamp_Response *response) {
	response->header.length = sizeof(TNGGetTimestamp_Response);
	response->timestamp     = system_timer_get_us();

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

TNGHandleMessageResponse tng_copy_firmware(const TNGCopyFirmware *data, TNGCopyFirmware_Response *response) {
	response->header.length = sizeof(TNGCopyFirmware_Response);
	response->status        = tng_firmware_check_all();

	if(response->status == TNG_FIRMWARE_COPY_STATUS_OK) {
		tng_firmware_set_boot_info(0xFFFFFFFF);
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

TNGHandleMessageResponse tng_set_write_firmware_pointer(const TNGSetWriteFirmwarePointer *data) {
	tng_firmware_pointer = data->pointer;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

TNGHandleMessageResponse tng_write_firmware(const TNGWriteFirmware *data, TNGWriteFirmware_Response *response) {
	response->header.length = sizeof(TNGWriteFirmware_Response);
	HAL_FLASH_Unlock();
	if(tng_firmware_pointer == 0) {
		FLASH_EraseInitTypeDef erase_init = {
			.TypeErase = FLASH_TYPEERASE_PAGES,
			.PageAddress = STM32F0_FIRMWARE_NEW_POS_START,
			.NbPages = STM32F0_FIRMWARE_SIZE/FLASH_PAGE_SIZE
		};
		uint32_t page_error = 0;
		if(HAL_FLASHEx_Erase(&erase_init, &page_error) != HAL_OK) {
			HAL_FLASH_Lock();
			response->status = TNG_WRITE_FIRMWARE_STATUS_INVALID_POINTER;
			return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
		}
	}

	for(uint8_t i = 0; i < TNG_WRITE_FIRMWARE_CHUNK_SIZE; i += 4) {
		uint32_t *data32 = (uint32_t *)&data->data[i];
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, STM32F0_FIRMWARE_NEW_POS_START + tng_firmware_pointer + i, *data32) != HAL_OK) {
			HAL_FLASH_Lock();
			response->status = TNG_WRITE_FIRMWARE_STATUS_INVALID_POINTER;
			return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
		}
	}
	HAL_FLASH_Lock();

	response->status = TNG_WRITE_FIRMWARE_STATUS_OK;
	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

TNGHandleMessageResponse tng_reset(const TNGReset *data) {
	NVIC_SystemReset();

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

TNGHandleMessageResponse tng_read_uid(const TNGWriteUID *data, TNGReadUID_Response *response) {
	response->header.length = sizeof(TNGReadUID_Response);
	response->uid = tng_get_uid();

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

TNGHandleMessageResponse tng_write_uid(const TNGWriteUID *data) {
	if((data->uid == 0) || (data->uid == 1) || (data->uid == UINT32_MAX)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	HAL_FLASH_Unlock();

	FLASH_EraseInitTypeDef erase_init = {
		.TypeErase = FLASH_TYPEERASE_PAGES,
		.PageAddress = STM32F0_UID_POSITION,
		.NbPages = 1
	};
	uint32_t page_error = 0;
	if(HAL_FLASHEx_Erase(&erase_init, &page_error) != HAL_OK) {
		HAL_FLASH_Lock();
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, STM32F0_UID_POSITION, data->uid) != HAL_OK) {
		HAL_FLASH_Lock();
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	HAL_FLASH_Lock();
	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

TNGHandleMessageResponse tng_get_identity(const TNGGetIdentity *data, TNGGetIdentity_Response *response) {
	response->header.length = sizeof(TNGGetIdentity_Response);

	tfp_uid_uint32_to_base58(tng_get_uid(), response->uid);
	memset(response->connected_uid, 0, TFP_UID_STR_MAX_LENGTH);

	response->version_hw[0] = HARDWARE_VERSION_MAJOR;
	response->version_hw[1] = HARDWARE_VERSION_MINOR;
	response->version_hw[2] = HARDWARE_VERSION_REVISION;

	response->version_fw[0] = FIRMWARE_VERSION_MAJOR;
	response->version_fw[1] = FIRMWARE_VERSION_MINOR;
	response->version_fw[2] = FIRMWARE_VERSION_REVISION;

	response->device_identifier = device_identifier;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

TNGHandleMessageResponse tng_enumerate(const TNGEnumerate *data, TNGEnumerate_Callback *response) {
	// The function itself does not return anything, but we return the callback here instead.
	// We use get_identity for uids, fw version and hw version.
	// The layout of the struct it the same.
	tng_get_identity((void*)data, (void*)response);
	tfp_make_default_header(&response->header, tng_get_uid(), sizeof(TNGEnumerate_Callback), TNG_FID_ENUMERATE_CALLBACK);

	response->enumeration_type = TNG_ENUMERATE_TYPE_AVAILABLE;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

bool tng_send_initial_enumerate(void) {
	TNGEnumerate_Callback response;

	// Use the normal tng_enumerate function and replace the type from enumerate-available to -added.
	tng_enumerate(NULL, &response);
	response.enumeration_type = TNG_ENUMERATE_TYPE_ADDED;

	return usb_send((uint8_t *)&response, response.header.length);
}