/* TNG
 * Copyright (C) 2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * usb_tfp.h: TNG system STM32 USB descriptors for Vendor 
 *            Specifc USB class (for TFP)
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

#ifndef USBD_TFP_H
#define USBD_TFP_H

#include  "usbd_ioreq.h"

#define USBD_TFP_IN_EP     0x81
#define USBD_TFP_OUT_EP    0x01
#define USBD_TFP_IN_SIZE   64
#define USBD_TFP_OUT_SIZE  64

extern USBD_ClassTypeDef usbd_tfp;

#endif
