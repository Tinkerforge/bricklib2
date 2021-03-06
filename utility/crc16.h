/* bricklib2
 * Copyright (C) 2017 Ishraq Ibne Ashraf <ishraq@tinkerforge.com>
 * Copyright (C) 2017 Olaf Lüke <olaf@tinkerforge.com>
 *
 * crc16.h: Implementation of CRC16 checksum calculation
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

#ifndef CRC16_H
#define CRC16_H

#include "configs/config.h"

#include <stdint.h>

#ifdef CRC16_USE_MODBUS
uint16_t crc16_modbus(uint8_t *buffer, uint32_t length);
#endif

#ifdef CRC16_USE_CCITT
uint16_t crc16_ccitt_8in(uint8_t *buffer, uint32_t length);
uint16_t crc16_ccitt_16in(uint16_t *buffer, uint32_t length);
#endif

#endif
