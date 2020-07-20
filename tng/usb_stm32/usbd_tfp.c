/* TNG
 * Copyright (C) 2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * usb_tfp.c: TNG system STM32 USB descriptors for Vendor 
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

#include "usbd_tfp.h"

#include "configs/config.h"
#include "bricklib2/logging/logging.h"
#include "bricklib2/tng/tng_communication.h"
#include "usb.h"

// USB Standard Device Descriptor
__attribute__ ((aligned (4))) static uint8_t usbd_tfp_device_qualifier_descriptor[USB_LEN_DEV_QUALIFIER_DESC] = {
	USB_LEN_DEV_QUALIFIER_DESC,
	USB_DESC_TYPE_DEVICE_QUALIFIER,
	0x00,
	0x02,
	0x00,
	0x00,
	0x00,
	0x40,
	0x01,
	0x00,
};


// TFP device Configuration Descriptor
__attribute__ ((aligned (4))) static uint8_t usbd_tfp_configuration_descriptor[] = {
	// Configuration Descriptor
	0x09,   // bLength: Configuration Descriptor size
	USB_DESC_TYPE_CONFIGURATION,      // bDescriptorType: Configuration
	9+9+7+7,                          // wTotalLength: no of returned bytes
	0x00,
	0x01,   // bNumInterfaces: 1 interface
	0x01,   // bConfigurationValue: Configuration value 
	0x00,   // iConfiguration: Index of string descriptor describing the configuration
	0xC0,   // bmAttributes: self powered <-- TODO
	0x32,   // MaxPower XXX mA  <-- TODO
	
	// Interface Descriptor
	0x09,   // bLength: Interface Descriptor size
	USB_DESC_TYPE_INTERFACE,  // bDescriptorType: Interface
	0x00,   // bInterfaceNumber: Number of Interface
	0x00,   // bAlternateSetting: Alternate setting
	0x02,   // bNumEndpoints: Two endpoints used
	0xFF,   // bInterfaceClass: Vendor Specific
	0x00,   // bInterfaceSubClass: None
	0x00,   // bInterfaceProtocol: None
	0x00,   // iInterface: No string descriptor

	// Endpoint IN Descriptor
	0x07,                    // bLength: Endpoint Descriptor size
	USB_DESC_TYPE_ENDPOINT,  // bDescriptorType: Endpoint
	USBD_TFP_IN_EP,          // bEndpointAddress 
	0x02,                    // bmAttributes: Bulk
	LOBYTE(USBD_TFP_IN_SIZE),   // wMaxPacketSize 
	HIBYTE(USBD_TFP_IN_SIZE),
	0x00,                    // bInterval: ignore for Bulk transfer

	// Endpoint OUT Descriptor
	0x07,                    // bLength: Endpoint Descriptor size
	USB_DESC_TYPE_ENDPOINT,  // bDescriptorType: Endpoint
	USBD_TFP_OUT_EP,         // bEndpointAddress 
	0x02,                    // bmAttributes: Bulk
	LOBYTE(USBD_TFP_OUT_SIZE),  // wMaxPacketSize 
	HIBYTE(USBD_TFP_OUT_SIZE),
	0x00                     // bInterval: ignore for Bulk transfer
};

static uint8_t usbd_tfp_init(USBD_HandleTypeDef *dev, uint8_t cfgidx) {
	// Open IN EP 
	USBD_LL_OpenEP(dev,	USBD_TFP_IN_EP,  USBD_EP_TYPE_BULK, USBD_TFP_IN_SIZE);

	// Open OUT EP 
	USBD_LL_OpenEP(dev,	USBD_TFP_OUT_EP, USBD_EP_TYPE_BULK, USBD_TFP_OUT_SIZE);
	
	// Prepare Out endpoint to receive next packet
	USBD_LL_PrepareReceive(dev, USBD_TFP_OUT_EP, tfusb.out_buffer, USBD_TFP_OUT_SIZE);

	// TODO: We don't check if there was enough space in the usb buffer here.
	//       I think this is always OK for the initial enumerate, since there is never any messages in the buffer?
	//       If this doesn't work out we can add a tng_communication_tick with a bool for the initial enumeration.
	tng_send_initial_enumerate();

	return USBD_OK;
}

static uint8_t usbd_tfp_deinit(USBD_HandleTypeDef *dev, uint8_t cfgidx) {
	// Close IN EP
	USBD_LL_CloseEP(dev, USBD_TFP_IN_EP);

	// Close OUT EP
	USBD_LL_CloseEP(dev, USBD_TFP_OUT_EP);
	
	return USBD_OK;
}

static uint8_t usbd_tfp_data_in(USBD_HandleTypeDef *dev, uint8_t epnum) {
	tfusb.transfer_in_progress = false;
	usb_send_storage();

	return USBD_OK;
}

static uint8_t usbd_tfp_data_out(USBD_HandleTypeDef *dev, uint8_t epnum) {      
	tfusb.out_buffer_length = USBD_LL_GetRxDataSize(dev, epnum);

	return USBD_OK;
}

static uint8_t  *usbd_tfp_get_configuration_descriptor(uint16_t *length) {
	*length = sizeof(usbd_tfp_configuration_descriptor);
	return usbd_tfp_configuration_descriptor;
}

uint8_t *usbd_tfp_get_device_qualifier_descriptor(uint16_t *length) {
	*length = sizeof(usbd_tfp_device_qualifier_descriptor);
	return usbd_tfp_device_qualifier_descriptor;
}

// TFP interface class callbacks structure
USBD_ClassTypeDef usbd_tfp =  {
	usbd_tfp_init,
	usbd_tfp_deinit,
	NULL, // usbd_tfp_setup
	NULL, // usbd_tfp_ep0_tx_ready
	NULL, // usbd_tfp_ep0_rx_ready
	usbd_tfp_data_in,
	usbd_tfp_data_out,
	NULL, // usbd_tfp_sof
	NULL, // usbd_tfp_iso_in_complete
	NULL, // usbd_tfp_iso_out_complete
	usbd_tfp_get_configuration_descriptor,
	usbd_tfp_get_configuration_descriptor,
	usbd_tfp_get_configuration_descriptor,
	usbd_tfp_get_device_qualifier_descriptor,
};