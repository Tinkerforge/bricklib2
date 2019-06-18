/* TNG
 * Copyright (C) 2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * usb.c: TNG system USB API
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

#include "usb.h"

#include "bricklib2/logging/logging.h"
#include "bricklib2/os/coop_task.h"

#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_tfp.h"
#include "usbd_def.h"

TFUSB tfusb;

USBD_HandleTypeDef usbd_device;

void usb_init(void) {
	USBD_StatusTypeDef status = USBD_Init(&usbd_device, &usbd_descriptors, DEVICE_FS);
	if(status != USBD_OK) {
		loge("USBD_Init error: %d\n\r", status);
	}

	status = USBD_RegisterClass(&usbd_device, &usbd_tfp);
	if(status != USBD_OK) {
		loge("USBD_RegisterClass error: %d\n\r", status);
	}

	status = USBD_Start(&usbd_device);
	if(status != USBD_OK) {
		loge("USBD_Start error: %d\n\r", status);
	}
}

void usb_task_send(uint8_t *data, uint16_t length) {
	while(tfusb.transfer_in_progress) {
		coop_task_yield();
	}

	memcpy(tfusb.in_buffer, data, length);
	tfusb.in_buffer_length = length;
	tfusb.transfer_in_progress = true;
	while(USBD_LL_Transmit(&usbd_device, USBD_TFP_IN_EP, tfusb.in_buffer, tfusb.in_buffer_length) != USBD_OK) {
		coop_task_yield();
	}

}

bool usb_send(uint8_t *data, uint16_t length) {
	if(tfusb.transfer_in_progress) {
		return false;
	}

	memcpy(tfusb.in_buffer, data, length);
	tfusb.in_buffer_length = length;
	tfusb.transfer_in_progress = true;
	if(USBD_LL_Transmit(&usbd_device, USBD_TFP_IN_EP, tfusb.in_buffer, tfusb.in_buffer_length) == USBD_OK) {
		return true;
	}

	tfusb.transfer_in_progress = false;
	return false;
}

uint16_t usb_recv(uint8_t *data, uint16_t max_length) {
	if(tfusb.out_buffer_length == 0) {
		return 0;
	}

	const uint16_t length = tfusb.out_buffer_length;
	memcpy(data, tfusb.out_buffer, length);
	if(USBD_LL_PrepareReceive(&usbd_device, USBD_TFP_OUT_EP, tfusb.out_buffer, USBD_TFP_OUT_SIZE) == USBD_OK) {
		tfusb.out_buffer_length = 0;
		return length;
	}

	return 0;
}
