/* TNG
 * Copyright (C) 2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * usb_desc.c: TNG system STM32 USB descriptors
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

#include "usbd_desc.h"

#include "usbd_core.h"
#include "usbd_conf.h"

#include "configs/config.h"


#define USBD_VID                     0x16D0
#define USBD_PID                     0x063D
#define USBD_LANGID_STRING           0x0409 // English US
#define USBD_MANUFACTURER_STRING     "Tinkerforge GmbH"
#define USBD_PRODUCT_STRING          "TNG DI8"
#define USBD_CONFIGURATION_STRING    "TFP Configuration"
#define USBD_INTERFACE_STRING        "TFP Interface"
#define USBD_SERIAL_STRING           "A"
#define USBD_MAX_STRING_SIZE         512

// USB standard device descriptor for TNG modules 
__attribute__ ((aligned (4))) uint8_t usbd_device_descriptor[USB_LEN_DEV_DESC] = {
	0x12,                       // bLength
	USB_DESC_TYPE_DEVICE,       // bDescriptorType
	0x00,                       // bcdUSB
	0x02,
	0xff,                       // bDeviceClass
	0x02,                       // bDeviceSubClass
	0x00,                       // bDeviceProtocol
	USB_MAX_EP0_SIZE,           // bMaxPacketSize
	LOBYTE(USBD_VID),           // idVendor
	HIBYTE(USBD_VID),           // idVendor
	LOBYTE(USBD_PID),           // idProduct
	HIBYTE(USBD_PID),           // idProduct
	0x00,                       // bcdDevice rel. 2.00
	0x02,
	USBD_IDX_MFC_STR,           // Index of manufacturer  string
	USBD_IDX_PRODUCT_STR,       // Index of product string
	USBD_IDX_SERIAL_STR,        // Index of serial number string
	USBD_MAX_NUM_CONFIGURATION  // bNumConfigurations
};


// USB language indentifier descriptor
__attribute__ ((aligned (4))) uint8_t usbd_language_id_descriptor[] = {
	USB_LEN_LANGID_STR_DESC,
	USB_DESC_TYPE_STRING,
	LOBYTE(USBD_LANGID_STRING),
	HIBYTE(USBD_LANGID_STRING)
};

// Buffer for general string descriptor
__attribute__ ((aligned (4))) uint8_t usbd_string_descriptor[USBD_MAX_STRING_SIZE];


uint8_t *usbd_get_device_descriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
	UNUSED(speed);
	*length = sizeof(usbd_device_descriptor);
	return usbd_device_descriptor;
}

uint8_t *usbd_get_language_id_descriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
	UNUSED(speed);
	*length = sizeof(usbd_language_id_descriptor);
	return usbd_language_id_descriptor;
}

uint8_t *usbd_get_manufacturer_string_descriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
	UNUSED(speed);
	USBD_GetString((uint8_t *)USBD_MANUFACTURER_STRING, usbd_string_descriptor, length);
	return usbd_string_descriptor;
}

uint8_t *usbd_get_product_string_descriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
	UNUSED(speed);
	USBD_GetString((uint8_t *)USBD_PRODUCT_STRING, usbd_string_descriptor, length);
	return usbd_string_descriptor;
}

uint8_t *usbd_get_serial_string_descriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
	UNUSED(speed);
	USBD_GetString((uint8_t *)USBD_SERIAL_STRING, usbd_string_descriptor, length);
	return usbd_string_descriptor;
}

uint8_t *usbd_get_config_string_descriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
	UNUSED(speed);
	USBD_GetString((uint8_t *)USBD_CONFIGURATION_STRING, usbd_string_descriptor, length);
	return usbd_string_descriptor;
}

uint8_t *usbd_get_interface_string_descriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
	UNUSED(speed);
	USBD_GetString((uint8_t *)USBD_INTERFACE_STRING, usbd_string_descriptor, length);
	return usbd_string_descriptor;
}


USBD_DescriptorsTypeDef usbd_descriptors = {
	usbd_get_device_descriptor,
	usbd_get_language_id_descriptor,
	usbd_get_manufacturer_string_descriptor,
	usbd_get_product_string_descriptor,
	usbd_get_serial_string_descriptor,
	usbd_get_config_string_descriptor,
	usbd_get_interface_string_descriptor,
};