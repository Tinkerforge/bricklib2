/* tng-stm32
 * Copyright (C) 2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * config.h: Configuration for stm32f0 with 128kb flash
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

#ifndef CONFIG_STM32F0_128KB_H
#define CONFIG_STM32F0_128KB_H

#include "stm32f0xx_hal.h"

#define STM32F0_MAGIC_NUMBER 0xDEADBEEF

#define STM32F0_BOOTLOADER_POS_START  0x8000000
#define STM32F0_RAM_VECTOR_TABLE_POSITION 0x20000000


#define STM32F0_MCU_FLASH_SIZE (128*1024)
#define STM32F0_FIRMWARE_PREAMBLE_SIZE (4*4)

#define STM32F0_FIRMWARE_BOOTLOADER_SIZE (8*1024)
#define STM32F0_FIRMWARE_SIZE ((STM32F0_MCU_FLASH_SIZE - STM32F0_FIRMWARE_BOOTLOADER_SIZE)/2)

#define STM32F0_FIRMWARE_ACTIVE_POS_START (STM32F0_BOOTLOADER_POS_START + STM32F0_FIRMWARE_BOOTLOADER_SIZE)
#define STM32F0_FIRMWARE_ACTIVE_POS_STACK_POINTER (STM32F0_FIRMWARE_ACTIVE_POS_START + STM32F0_FIRMWARE_PREAMBLE_SIZE)
#define STM32F0_FIRMWARE_NEW_POS_START (STM32F0_FIRMWARE_ACTIVE_POS_START + STM32F0_FIRMWARE_SIZE)

#define STM32F0_UID_POSITION (STM32F0_FIRMWARE_NEW_POS_START - 4)
#define STM32F0_CONFIG_FLASH_PAGE ((STM32F0_UID_POSITION - STM32F0_BOOTLOADER_POS_START) / FLASH_PAGE_SIZE)

#endif