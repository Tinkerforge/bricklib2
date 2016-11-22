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

#include "configs/config.h"

#include "bricklib2/hal/system_timer/system_timer.h"
#include "communication.h"

const uint32_t device_identifier __attribute__ ((section(".device_identifier"))) = BOOTLOADER_DEVICE_IDENTIFIER;
const uint32_t firmware_version __attribute__ ((section(".firmware_version"))) = (FIRMWARE_VERSION_MAJOR << 16) | (FIRMWARE_VERSION_MINOR << 8) | (FIRMWARE_VERSION_REVISION << 0);

const bootloader_firmware_entry_func_t bootloader_firmware_entry =  BOOTLOADER_FIRMWARE_ENTRY_FUNC;

BootloaderStatus bootloader_status;
BootloaderFunctions bootloader_functions;

#ifdef BOOTLOADER_FUNCTION_SPITFP_TICK
void bootloader_spitfp_tick(BootloaderStatus *bootloader_status) {
	bootloader_functions.spitfp_tick(bootloader_status);
}
#endif

#ifdef BOOTLOADER_FUNCTION_SEND_ACK_AND_MESSAGE
void bootloader_spitfp_send_ack_and_message(BootloaderStatus *bootloader_status, uint8_t *data, const uint8_t length) {
	bootloader_functions.spitfp_send_ack_and_message(bootloader_status, data, length);
}
#endif

#ifdef BOOTLOADER_FUNCTION_SPITFP_IS_SEND_POSSIBLE
bool bootloader_spitfp_is_send_possible(SPITFP *st) {
	return bootloader_functions.spitfp_is_send_possible(st);
}
#endif

#ifdef BOOTLOADER_FUNCTION_GET_UID
uint32_t bootloader_get_uid(void) {
	return bootloader_functions.get_uid();
}
#endif

#ifdef BOOTLOADER_FUNCTION_DSU_CRC32_CAL
enum status_code bootloader_dsu_crc32_cal(const uint32_t addr, const uint32_t len, uint32_t *pcrc32) {
	return bootloader_functions.dsu_crc32_cal(addr, len, pcrc32);
}
#endif

#ifdef BOOTLOADER_FUNCTION_SPI_INIT
enum status_code bootloader_spi_init(struct spi_module *const module, Sercom *const hw, const struct spi_config *const config) {
	return bootloader_functions.spi_init(module, hw, config);
}
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_GET_CHANNEL_CONFIG_DEFAULTS
void bootloader_tinydma_get_channel_config_defaults(TinyDmaChannelConfig *config) {
	bootloader_functions.tinydma_get_channel_config_defaults(config);
}
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_INIT
void bootloader_tinydma_init(DmacDescriptor *descriptor_section, DmacDescriptor *write_back_section) {
	bootloader_functions.tinydma_init(descriptor_section, write_back_section);
}
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_START_TRANSFER
void bootloader_tinydma_start_transfer(const uint8_t channel_id) {
	bootloader_functions.tinydma_start_transfer(channel_id);
}
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_DESCRIPTOR_GET_CONFIG_DEFAULTS
void bootloader_tinydma_descriptor_get_config_defaults(TinyDmaDescriptorConfig *config) {
	bootloader_functions.tinydma_descriptor_get_config_defaults(config);
}
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_DESCRIPTOR_INIT
void bootloader_tinydma_descriptor_init(DmacDescriptor* descriptor, TinyDmaDescriptorConfig *config) {
	bootloader_functions.tinydma_descriptor_init(descriptor, config);
}
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_CHANNEL_INIT
void bootloader_tinydma_channel_init(const uint8_t channel_id, TinyDmaChannelConfig *config) {
	bootloader_functions.tinydma_channel_init(channel_id, config);
}
#endif


void bootloader_init(void) {
#ifdef __SAM0__
	bootloader_status.st.descriptor_section = tinydma_get_descriptor_section();
	bootloader_status.st.write_back_section = tinydma_get_write_back_section();
#endif

	bootloader_status.boot_mode = BOOT_MODE_FIRMWARE;
	bootloader_status.reboot_started_at = 0;
	bootloader_status.status_led_config = 1;
	bootloader_status.firmware_handle_message_func = handle_message;

	bootloader_firmware_entry(&bootloader_functions, &bootloader_status);
}

void bootloader_tick(void) {
	bootloader_status.system_timer_tick = system_timer_get_ms();
#ifdef BOOTLOADER_FUNCTION_SPITFP_TICK
	bootloader_spitfp_tick(&bootloader_status);
#endif
}
