/* bricklib2
 * Copyright (C) 2017 Ishraq Ibne Ashraf <ishraq@tinkerforge.com>
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

#include <string.h>

#include "crc16.h"

void crc16(uint8_t *buffer, uint32_t length, uint8_t *checksum) {
  uint16_t crc = 0xFFFF;

  for(uint32_t position = 0; position < length; position++) {
    crc ^= buffer[position];

    for(uint8_t i = 8; i != 0; i--) {
      if((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      }
      else  {
        crc >>= 1;
      }
    }
  }

  memcpy(checksum, &crc, 2);
}
