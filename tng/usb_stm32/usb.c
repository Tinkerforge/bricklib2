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

// USB interrupt enable/disable functions can be used to prevent race condition
// with USB code and USB_IRQHandler. Currently, because of the structure of
// the USB code, these are not necessary!
void usb_interrupt_enable(bool mask) {
	if(mask) {
		NVIC_EnableIRQ(USB_IRQn);
	}
}

bool usb_interrupt_disable(void) {
	// Is USB interrupt enabled
	const bool mask = ((uint32_t)(((NVIC->ISER[0U] & (1UL << (((uint32_t)(int32_t)USB_IRQn) & 0x1FUL))) != 0UL) ? 1UL : 0UL));

	if(mask) {
		NVIC_DisableIRQ(USB_IRQn);

		// Data and Instruction Synchronization Barrier.
		__DSB();
		__ISB();
	}
	return mask;
}

void usb_init(void) {
	memset(&tfusb, 0, sizeof(TFUSB));

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

bool usb_send_storage(void) {
	// If there is data in storage and there is no transfer in progress,
	// try to send the storage data.
	if((tfusb.in_buffer_storage_length != 0) && (!tfusb.transfer_in_progress)) {
		// Copy storage to USB in buffer and try to send
		memcpy(tfusb.in_buffer, tfusb.in_buffer_storage, tfusb.in_buffer_storage_length);
		tfusb.in_buffer_length = tfusb.in_buffer_storage_length;
		tfusb.transfer_in_progress = true;
		if(USBD_LL_Transmit(&usbd_device, USBD_TFP_IN_EP, tfusb.in_buffer, tfusb.in_buffer_length) == USBD_OK) {
			// If the transmit was successful, we can set the storage length back to 0.
			tfusb.in_buffer_storage_length = 0;
			return true;
		}
		tfusb.transfer_in_progress = false;
	}

	return false;
}

bool usb_append_to_storage(uint8_t *data, uint16_t length) {
	if(tfusb.in_buffer_storage_length + length < USB_BUFFER_SIZE) {
		// Append to storage if there is enough space
		memcpy(&tfusb.in_buffer_storage[tfusb.in_buffer_storage_length], data, length);
		tfusb.in_buffer_storage_length += length;
		return true;
	}

	// Not enough space
	return false;
}

bool usb_send(uint8_t *data, uint16_t length) {
	// first check if the storage is empty.
	// If it is not we append the data to the storage, otherwise we would re-order responses.
	if(tfusb.in_buffer_storage_length > 0) {
		if(usb_append_to_storage(data, length)) {
			// Try to send the storage
			usb_send_storage();

			// The data was copied in any way at this point, we can always return true!
			return true;
		}
			
		// If the storage is full we can't do anything
		return false;
	}

	if(tfusb.transfer_in_progress) {
		// If there is a transfer in progress, append the data to storage
		return usb_append_to_storage(data, length);
	}

	// Copy data to USB in buffer and try to send
	memcpy(tfusb.in_buffer, data, length);
	tfusb.in_buffer_length = length;
	tfusb.transfer_in_progress = true;
	if(USBD_LL_Transmit(&usbd_device, USBD_TFP_IN_EP, tfusb.in_buffer, tfusb.in_buffer_length) == USBD_OK) {
		return true;
	}

	// If we can't send we try to append the data to the storage.
	tfusb.transfer_in_progress = false;

	return usb_append_to_storage(data, length);
}

uint16_t usb_recv(uint8_t *data, uint16_t max_length) {
	// If we didn't receive any data return 0.
	if(tfusb.out_buffer_length == 0) {
		return 0;
	}

	// Copy received data
	const uint16_t length = tfusb.out_buffer_length;
	memcpy(data, tfusb.out_buffer, length);

	// Set out USB buffer again for next receive.
	tfusb.out_buffer_length = 0;
	if(USBD_LL_PrepareReceive(&usbd_device, USBD_TFP_OUT_EP, tfusb.out_buffer, USB_BUFFER_SIZE) == USBD_OK) {
		return length;
	}

	tfusb.out_buffer_length = length;
	return 0;
}

inline bool usb_can_recv(void) {
	return tfusb.out_buffer_length != 0;
}