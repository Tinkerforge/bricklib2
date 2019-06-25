/* TNG
 * Copyright (C) 2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tng_firmware.h: TNG firmware copy/flash functions
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

#ifndef TNG_FIRMWARE_H
#define TNG_FIRMWARE_H

#include <stdbool.h>
#include <stdint.h>

#define TNG_FIRMWARE_COPY_STATUS_OK                          0
#define TNG_FIRMWARE_COPY_STATUS_DEVICE_IDENTIFIER_INCORRECT 1
#define TNG_FIRMWARE_COPY_STATUS_MAGIC_NUMBER_INCORRECT      2
#define TNG_FIRMWARE_COPY_STATUS_LENGTH_MALFORMED            3
#define TNG_FIRMWARE_COPY_STATUS_CRC_MISMATCH                4

uint32_t tng_firmware_get_length(void);
bool tng_firmware_check_device_identifier(void);
bool tng_firmware_check_magic_number(void);
bool tng_firmware_check_length(void);
bool tng_firmware_check_crc(void);
uint8_t tng_firmware_check_all(void);
uint32_t tng_firmware_get_boot_info(void);
void tng_firmware_set_boot_info(uint32_t boot_info);

#endif