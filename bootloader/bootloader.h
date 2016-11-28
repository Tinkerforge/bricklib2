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
#include "bricklib2/utility/led_flicker.h"
#include "configs/config.h"

#ifdef BOOTLOADER_FUNCTION_DSU_CRC32_CAL
#include "dsu_crc32.h"
#endif

typedef enum {
	HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE = 0,
	HANDLE_MESSAGE_RESPONSE_EMPTY = 1,
	HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED = 2,
	HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER = 3,
} BootloaderHandleMessageResponse;

typedef BootloaderHandleMessageResponse (* bootloader_firmware_handle_message_func_t)(const void *, void *);

typedef enum {
	BOOT_MODE_BOOTLOADER = 0,
	BOOT_MODE_FIRMWARE = 1,
	BOOT_MODE_BOOTLOADER_WAIT_FOR_REBOOT = 2,
	BOOT_MODE_FIRMWARE_WAIT_FOR_REBOOT = 3,
	BOOT_MODE_FIRMWARE_WAIT_FOR_ERASE_AND_REBOOT = 4,
} BootloaderBootMode;

typedef struct {
	uint32_t error_count_ack_checksum;
	uint32_t error_count_message_checksum;
	uint32_t error_count_frame;
	uint32_t error_count_overflow;
} BootloaderErrorCount;

typedef struct {
	bootloader_firmware_handle_message_func_t firmware_handle_message_func;
	BootloaderBootMode boot_mode;
	uint32_t system_timer_tick;
	uint32_t reboot_started_at;

	LEDFlickerState led_flicker_state;
	BootloaderErrorCount error_count;

	SPITFP st;
} BootloaderStatus;

// Firmware stuff
typedef struct {
	uint32_t firmware_version;
	uint32_t device_identifier;
	uint32_t firmware_crc;
} __attribute__((__packed__)) BootloaderFirmwareConfiguration;

typedef struct {
#ifdef BOOTLOADER_FUNCTION_SPITFP_TICK
	void (*spitfp_tick)(BootloaderStatus *bootloader_status);
#endif

#ifdef BOOTLOADER_FUNCTION_SEND_ACK_AND_MESSAGE
	void (*spitfp_send_ack_and_message)(BootloaderStatus *bootloader_status, uint8_t *data, const uint8_t length);
#endif

#ifdef BOOTLOADER_FUNCTION_SPITFP_IS_SEND_POSSIBLE
	bool (*spitfp_is_send_possible)(SPITFP *st);
#endif

#ifdef BOOTLOADER_FUNCTION_GET_UID
	uint32_t (*get_uid)(void);
#endif

#ifdef BOOTLOADER_FUNCTION_DSU_CRC32_CAL
	enum status_code (*dsu_crc32_cal)(const uint32_t addr, const uint32_t len, uint32_t *pcrc32);
#endif

#ifdef BOOTLOADER_FUNCTION_SPI_INIT
	enum status_code (*spi_init)(struct spi_module *const module, Sercom *const hw, const struct spi_config *const config);
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_GET_CHANNEL_CONFIG_DEFAULTS
	void (*tinydma_get_channel_config_defaults)(TinyDmaChannelConfig *config);
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_INIT
	void (*tinydma_init)(DmacDescriptor *descriptor_section, DmacDescriptor *write_back_section);
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_START_TRANSFER
	void (*tinydma_start_transfer)(const uint8_t channel_id);
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_DESCRIPTOR_GET_CONFIG_DEFAULTS
	void (*tinydma_descriptor_get_config_defaults)(TinyDmaDescriptorConfig *config);
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_DESCRIPTOR_INIT
	void (*tinydma_descriptor_init)(DmacDescriptor* descriptor, TinyDmaDescriptorConfig *config);
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_CHANNEL_INIT
	void (*tinydma_channel_init)(const uint8_t channel_id, TinyDmaChannelConfig *config);
#endif

#ifdef BOOTLOADER_FUNCTION_AEABI_IDIV
	int (*__aeabi_idiv)(int a, int b);
#endif

#ifdef BOOTLOADER_FUNCTION_AEABI_UIDIV
	unsigned int (*__aeabi_uidiv)(unsigned int a, unsigned int b);
#endif

#ifdef BOOTLOADER_FUNCTION_AEABI_IDIVMOD
	uint64_t (*__aeabi_idivmod)(int a, int b);
#endif

#ifdef BOOTLOADER_FUNCTION_AEABI_UIDIVMOD
	uint64_t (*__aeabi_uidivmod)(unsigned int a, unsigned int b);
#endif
} BootloaderFunctions;

typedef void (*bootloader_firmware_entry_func_t)(BootloaderFunctions *bf, BootloaderStatus *bs);

#ifndef BOOTLOADER_FLASH_SIZE
#define BOOTLOADER_FLASH_SIZE (16*1024)
#endif

#define BOOTLOADER_BOOTLOADER_SIZE (8*1024)
#if defined(__SAM0__)
	#define BOOTLOADER_BOOTLOADER_START_POS 0
#elif defined(__XMC1__)
	#define BOOTLOADER_BOOTLOADER_START_POS 0x10001000
#endif
#define BOOTLOADER_BOOTLOADER_BOOT_INFO_POS (BOOTLOADER_BOOTLOADER_START_POS + BOOTLOADER_BOOTLOADER_SIZE - sizeof(uint32_t))

#define BOOTLOADER_FIRMWARE_SIZE (BOOTLOADER_FLASH_SIZE - BOOTLOADER_BOOTLOADER_SIZE)
#define BOOTLOADER_FIRMWARE_START_POS (BOOTLOADER_BOOTLOADER_START_POS + BOOTLOADER_BOOTLOADER_SIZE)
#define BOOTLOADER_FIRMWARE_CRC_SIZE (sizeof(uint32_t))

#define BOOTLOADER_FIRMWARE_CONFIGURATION_POINTER ((BootloaderFirmwareConfiguration*)(BOOTLOADER_FIRMWARE_START_POS + BOOTLOADER_FIRMWARE_SIZE - sizeof(BootloaderFirmwareConfiguration)))
#define BOOTLOADER_FIRMWARE_FIRST_BYTES (*((uint32_t*)(BOOTLOADER_FIRMWARE_START_POS)))
#define BOOTLOADER_FIRMWARE_FIRST_BYTES_DEFAULT 0xFFFFFFFF

#define BOOTLOADER_FIRMWARE_ENTRY_FUNC_SIZE 32
#define BOOTLOADER_FIRMWARE_ENTRY_FUNC ((bootloader_firmware_entry_func_t)(BOOTLOADER_BOOTLOADER_START_POS + BOOTLOADER_BOOTLOADER_SIZE - BOOTLOADER_FIRMWARE_ENTRY_FUNC_SIZE + 1))

// If we are not in bootloader
#ifdef STARTUP_SYSTEM_INIT_ALREADY_DONE
extern BootloaderStatus bootloader_status;

void bootloader_init(void);
void bootloader_tick(void);

#ifdef BOOTLOADER_FUNCTION_SPITFP_TICK
void bootloader_spitfp_tick(BootloaderStatus *bootloader_status);
#endif

#ifdef BOOTLOADER_FUNCTION_SEND_ACK_AND_MESSAGE
void bootloader_spitfp_send_ack_and_message(BootloaderStatus * bootloader_status, uint8_t *data, const uint8_t length);
#endif

#ifdef BOOTLOADER_FUNCTION_SPITFP_IS_SEND_POSSIBLE
bool bootloader_spitfp_is_send_possible(SPITFP *st);
#endif

#ifdef BOOTLOADER_FUNCTION_GET_UID
uint32_t bootloader_get_uid(void);
#endif

#ifdef BOOTLOADER_FUNCTION_DSU_CRC32_CAL
enum status_code bootloader_dsu_crc32_cal(const uint32_t addr, const uint32_t len, uint32_t *pcrc32);
#endif

#ifdef BOOTLOADER_FUNCTION_SPI_INIT
enum status_code bootloader_spi_init(struct spi_module *const module, Sercom *const hw, const struct spi_config *const config);
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_GET_CHANNEL_CONFIG_DEFAULTS
void bootloader_tinydma_get_channel_config_defaults(TinyDmaChannelConfig *config);
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_INIT
void bootloader_tinydma_init(DmacDescriptor *descriptor_section, DmacDescriptor *write_back_section);
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_START_TRANSFER
void bootloader_tinydma_start_transfer(const uint8_t channel_id);
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_DESCRIPTOR_GET_CONFIG_DEFAULTS
void bootloader_tinydma_descriptor_get_config_defaults(TinyDmaDescriptorConfig *config);
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_DESCRIPTOR_INIT
void bootloader_tinydma_descriptor_init(DmacDescriptor* descriptor, TinyDmaDescriptorConfig *config);
#endif

#ifdef BOOTLOADER_FUNCTION_TINYDMA_CHANNEL_INIT
void bootloader_tinydma_channel_init(const uint8_t channel_id, TinyDmaChannelConfig *config);
#endif

#endif

#endif
