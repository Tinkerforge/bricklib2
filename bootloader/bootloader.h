/* bricklib2
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * bootloader.h: Configuration for bootloader
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

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "bricklib2/protocols/spitfp/spitfp.h"
#include "dsu_crc32.h"

typedef enum {
	HANDLE_MESSAGE_RETURN_NEW_MESSAGE = 0,
	HANDLE_MESSAGE_RETURN_EMPTY = 1,
	HANDLE_MESSAGE_RETURN_NOT_SUPPORTED = 2,
	HANDLE_MESSAGE_RETURN_INVALID_PARAMETER = 3,
} BootloaderHandleMessageReturn;

typedef BootloaderHandleMessageReturn (* bootloader_firmware_handle_message_func_t)(const void *, const uint8_t );

typedef enum {
	BOOT_MODE_BOOTLOADER = 0,
	BOOT_MODE_FIRMWARE = 1,
	BOOT_MODE_BOOTLOADER_WAIT_FOR_REBOOT = 2,
	BOOT_MODE_FIRMWARE_WAIT_FOR_REBOOT = 3,
	BOOT_MODE_FIRMWARE_WAIT_FOR_ERASE_AND_REBOOT = 4,
} BootloaderBootMode;

typedef struct {
	bootloader_firmware_handle_message_func_t firmware_handle_message_func;
	BootloaderBootMode boot_mode;
	uint8_t status_led_config;
	uint32_t system_timer_tick;
	uint32_t reboot_started_at;

	SPITFP st;
} BootloaderStatus;

// Firmware stuff
typedef struct {
	uint32_t firmware_version;
	uint32_t device_identifier;
	uint32_t firmware_crc;
} __attribute__((__packed__)) BootloaderFirmwareConfiguration;

typedef struct {
	void (*spitfp_tick)(BootloaderStatus *bootloader_status);
	void (*spitfp_send_ack_and_message)(SPITFP *st, uint8_t *data, const uint8_t length);
	bool (*spitfp_is_send_possible)(SPITFP *st);
	enum status_code (*dsu_crc32_cal)(const uint32_t addr, const uint32_t len, uint32_t *pcrc32);
} BootloaderFunctions;

typedef void (*firmware_entry_func_t)(BootloaderFunctions *bf, BootloaderStatus *bs);

#define BOOTLOADER_FLASH_SIZE (16*1024)

#define BOOTLOADER_BOOTLOADER_SIZE (8*1024)
#define BOOTLOADER_BOOTLOADER_START_POS 0
#define BOOTLOADER_BOOTLOADER_BOOT_INFO_POS (BOOTLOADER_BOOTLOADER_START_POS + BOOTLOADER_BOOTLOADER_SIZE - sizeof(uint32_t))

#define BOOTLOADER_FIRMWARE_SIZE (BOOTLOADER_FLASH_SIZE - BOOTLOADER_BOOTLOADER_SIZE)
#define BOOTLOADER_FIRMWARE_START_POS (BOOTLOADER_BOOTLOADER_START_POS + BOOTLOADER_BOOTLOADER_SIZE)
#define BOOTLOADER_FIRMWARE_CRC_SIZE (sizeof(uint32_t))

#define BOOTLOADER_FIRMWARE_CONFIGURATION_POINTER ((BootloaderFirmwareConfiguration*)(BOOTLOADER_FIRMWARE_START_POS + BOOTLOADER_FIRMWARE_SIZE - sizeof(BootloaderFirmwareConfiguration)))
#define BOOTLOADER_FIRMWARE_FIRST_BYTES (*((uint32_t*)(BOOTLOADER_FIRMWARE_START_POS)))
#define BOOTLOADER_FIRMWARE_FIRST_BYTES_DEFAULT 0xFFFFFFFF
#define BOOTLOADER_FIRMWARE_MAGIC_NUMBER 0xDEADBEEF

#define BOOTLOADER_FIRMWARE_ENTRY_FUNC_SIZE 64
#define BOOTLOADER_FIRMWARE_ENTRY_FUNC ((firmware_entry_func_t)(BOOTLOADER_BOOTLOADER_START_POS + BOOTLOADER_BOOTLOADER_SIZE - BOOTLOADER_FIRMWARE_ENTRY_FUNC_SIZE + 1))

#endif
