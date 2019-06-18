/* TNG
 * Copyright (C) 2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * usb_conf.h: TNG system STM32 USB configuration (for CubeMX API)
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

#ifndef USBD_CONF_H
#define USBD_CONF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "configs/config.h"
#include "bricklib2/logging/logging.h"

#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"

#define USBD_MAX_NUM_INTERFACES           1
#define USBD_MAX_NUM_CONFIGURATION        1
#define USBD_DEBUG_LEVEL                  0

#define DEVICE_FS                         0


#if (USBD_DEBUG_LEVEL > 0)
#define USBD_UsrLog(...)    logi(__VA_ARGS__); uartbb_putnl();
#else
#define USBD_UsrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 1)

#define USBD_ErrLog(...)    loge(__VA_ARGS__); uartbb_punl();
#else
#define USBD_ErrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 2)
#define USBD_DbgLog(...)    logd(__VA_ARGS__); uartbb_punl();
#else
#define USBD_DbgLog(...)
#endif

#endif 
