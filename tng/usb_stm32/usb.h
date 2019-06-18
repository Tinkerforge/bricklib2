/* TNG
 * Copyright (C) 2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * usb.h: TNG system USB API
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

#ifndef USB_H
#define USB_H

#include <stdbool.h>
#include <stdint.h>

#include "usbd_tfp.h"

typedef struct {
    __attribute__ ((aligned (4))) uint8_t in_buffer[USBD_TFP_IN_SIZE];
    __attribute__ ((aligned (4))) uint8_t out_buffer[USBD_TFP_OUT_SIZE];
    uint16_t in_buffer_length;
    uint16_t out_buffer_length;
    bool transfer_in_progress;
} TFUSB;

extern TFUSB tfusb;

void usb_init(void);
bool usb_send(uint8_t *data, uint16_t length);
uint16_t usb_recv(uint8_t *data, uint16_t max_length);

#endif