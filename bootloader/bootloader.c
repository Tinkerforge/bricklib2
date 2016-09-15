/* bricklib2
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * bootloader.c: Configuration for bootloader
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

#include "bootloader.h"

#include "bricklib2/hal/system_timer/system_timer.h"
#include "communication.h"

const uint32_t device_identifier __attribute__ ((section(".device_identifier"))) = BOOTLOADER_DEVICE_IDENTIFIER;
const uint32_t firmware_version __attribute__ ((section(".firmware_version"))) = (FIRMWARE_VERSION_MAJOR << 16) | (FIRMWARE_VERSION_MINOR << 8) | (FIRMWARE_VERSION_REVISION << 0);

const bootloader_firmware_entry_func_t bootloader_firmware_entry =  BOOTLOADER_FIRMWARE_ENTRY_FUNC;

BootloaderStatus bootloader_status;
BootloaderFunctions bootloader_functions;

void bootloader_spitfp_tick(BootloaderStatus *bootloader_status) {
	bootloader_functions.spitfp_tick(bootloader_status);
}

void bootloader_spitfp_send_ack_and_message(SPITFP *st, uint8_t *data, const uint8_t length) {
	bootloader_functions.spitfp_send_ack_and_message(st, data, length);
}

bool bootloader_spitfp_is_send_possible(SPITFP *st) {
	return bootloader_functions.spitfp_is_send_possible(st);
}

enum status_code bootloader_dsu_crc32_cal(const uint32_t addr, const uint32_t len, uint32_t *pcrc32) {
	return bootloader_functions.dsu_crc32_cal(addr, len, pcrc32);
}

void bootloader_init(void) {
	bootloader_status.boot_mode = BOOT_MODE_FIRMWARE;
	bootloader_status.status_led_config = 1;
	bootloader_status.firmware_handle_message_func = handle_message;
	bootloader_status.st.descriptor_section = tinydma_get_descriptor_section();
	bootloader_status.st.write_back_section = tinydma_get_write_back_section();
	bootloader_firmware_entry(&bootloader_functions, &bootloader_status);
}

void bootloader_tick(void) {
	bootloader_status.system_timer_tick = system_timer_get_ms();
	bootloader_spitfp_tick(&bootloader_status);
}
