/* TNG
 * Copyright (C) 2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tng_firmware.c: TNG firmware copy/flash functions
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

#include "tng_firmware.h"

#include "bricklib2/utility/crc32.h"

#include "configs/config.h"
#include "config_stm32f0_128kb.h"


uint32_t tng_firmware_get_crc(void) {
	const uint32_t *crc = (uint32_t *)STM32F0_NEW_CRC_POSITION;

	return *crc;
}

uint32_t tng_firmware_get_length(void) {
	const uint32_t *length = (uint32_t *)STM32F0_NEW_LENGTH_POSITION;

	return *length;
}

bool tng_firmware_check_device_identifier(void) {
	const uint32_t *new_did    = (uint32_t*)STM32F0_NEW_DEVICE_IDENTIFIER_POSITION;
	const uint32_t *active_did = (uint32_t*)STM32F0_ACTIVE_DEVICE_IDENTIFIER_POSITION;

	return *new_did == *active_did;
}

bool tng_firmware_check_magic_number(void) {
	const uint32_t *magic_number = (uint32_t*)STM32F0_NEW_MAGIC_NUMBER_POSITION;

	return *magic_number == STM32F0_MAGIC_NUMBER;
}

bool tng_firmware_check_length(void) {
	// We leave at least one page untouched by the 
	// firmware for UID and non-volatile settings
	return tng_firmware_get_length() < (STM32F0_FIRMWARE_SIZE - FLASH_PAGE_SIZE);
}

bool tng_firmware_check_crc(void) {
	const uint32_t length = tng_firmware_get_length();
	const uint32_t crc    = tng_firmware_get_crc();

	const uint32_t crc_calc = crc32_ieee_802_3((const void*)STM32F0_FIRMWARE_NEW_POS_START + 8, length - 8);

	return crc == crc_calc;
}

uint8_t tng_firmware_check_all(void) {
	if(!tng_firmware_check_device_identifier()) {
		return TNG_FIRMWARE_COPY_STATUS_DEVICE_IDENTIFIER_INCORRECT;
	}

	if(!tng_firmware_check_magic_number()) {
		return TNG_FIRMWARE_COPY_STATUS_MAGIC_NUMBER_INCORRECT;
	}

	if(!tng_firmware_check_length()) {
		return TNG_FIRMWARE_COPY_STATUS_LENGTH_MALFORMED;
	}

	if(!tng_firmware_check_crc()) {
		return TNG_FIRMWARE_COPY_STATUS_CRC_MISMATCH;
	}

	return TNG_FIRMWARE_COPY_STATUS_OK;
}

uint32_t tng_firmware_get_boot_info(void) {
	const uint32_t *boot_info = (uint32_t*)STM32F0_BOOT_INFO_POSITION;

	return *boot_info;
}

void tng_firmware_set_boot_info(uint32_t boot_info) {
	HAL_FLASH_Unlock();

	FLASH_EraseInitTypeDef erase_init = {
		.TypeErase = FLASH_TYPEERASE_PAGES,
		.PageAddress = STM32F0_BOOT_INFO_POSITION,
		.NbPages = 1
	};
	uint32_t page_error = 0;
	if(HAL_FLASHEx_Erase(&erase_init, &page_error) != HAL_OK) {
		HAL_FLASH_Lock();
		return;
	}

	if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, STM32F0_BOOT_INFO_POSITION, boot_info) != HAL_OK) {
		HAL_FLASH_Lock();
		return;
	}

	HAL_FLASH_Lock();
}