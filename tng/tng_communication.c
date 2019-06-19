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

#include "bricklib2/tng/tng.h"

extern const uint32_t device_identifier;

static uint32_t tng_firmware_pointer = 0;

TNGHandleMessageResponse tng_handle_message(const void *message, void *response) {
	switch(tfp_get_fid_from_message(message)) {
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

TNGHandleMessageResponse tng_set_write_firmware_pointer(const TNGSetWriteFirmwarePointer *data) {
	tng_firmware_pointer = data->pointer;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

TNGHandleMessageResponse tng_write_firmware(const TNGWriteFirmware *data, TNGWriteFirmware_Response *response) {
    HAL_FLASH_Unlock();
    if(tng_firmware_pointer == 0) {
        FLASH_EraseInitTypeDef erase_init = {
            .TypeErase = FLASH_TYPEERASE_PAGES,
            .PageAddress = STM32F0_FIRMWARE_NEW_POS_START,
            .NbPages = STM32F0_FIRMWARE_SIZE/FLASH_PAGE_SIZE
        };
        uint32_t page_error = 0;
        if(HAL_FLASHEx_Erase(&erase_init, &page_error) != HAL_OK) {
            loge("HAL_FLASHEx_Erase error %d\n\r", page_error);
            response->status = TNG_WRITE_FIRMWARE_STATUS_INVALID_POINTER;
	        return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
        }
    }

    uint32_t *data32 = (uint32_t *)data->data;
    for(uint8_t i = 0; i < TNG_WRITE_FIRMWARE_CHUNK_SIZE/4; i++) {
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, STM32F0_FIRMWARE_NEW_POS_START + tng_firmware_pointer*TNG_WRITE_FIRMWARE_CHUNK_SIZE + i*4, *data32) != HAL_OK) {
            loge("HAL_FLASH_Program error\n\r");
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
        loge("HAL_FLASHEx_Erase error %d\n\r", page_error);
    }

    if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, STM32F0_UID_POSITION, data->uid) != HAL_OK) {
        loge("HAL_FLASH_Program error\n\r");
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
	logd("tng_enumerate\n\r");

	// The function itself does not return anything, but we return the callback here instead.
	// We use get_identity for uids, fw version and hw version.
	// The layout of the struct it the same.
	tng_get_identity((void*)data, (void*)response);

	response->header.length           = sizeof(TNGEnumerate_Callback);
	response->header.uid              = tng_get_uid();
	response->header.fid              = TNG_FID_ENUMERATE_CALLBACK;
	response->header.sequence_num     = 0; // Sequence number for callback is 0
	response->header.return_expected  = 1;
	response->header.authentication   = 0;
	response->header.other_options    = 0;
	response->header.error            = 0;
	response->header.future_use       = 0;

	response->enumeration_type = TNG_ENUMERATE_TYPE_AVAILABLE;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}