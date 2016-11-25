/* brickletboot
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spitfp.h: Tinkerforge Protocol (TFP) over SPI functions
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

#ifndef SPITFP_H
#define SPITFP_H

#ifdef __SAM0__
#include "spi.h"
#include "bricklib2/bootloader/tinydma.h"
#endif

#include "bricklib2/protocols/tfp/tfp.h"
#include "bricklib2/utility/ringbuffer.h"

#include "configs/config.h"

#define SPITFP_PROTOCOL_OVERHEAD 3 // 3 byte overhead for Brick <-> Bricklet SPI protocol

typedef enum {
	SPITFP_STATE_START,
	SPITFP_STATE_ACK_SEQUENCE_NUMBER,
	SPITFP_STATE_ACK_CHECKSUM,
	SPITFP_STATE_MESSAGE_SEQUENCE_NUMBER,
	SPITFP_STATE_MESSAGE_DATA,
	SPITFP_STATE_MESSAGE_CHECKSUM
} SPITFPState;


typedef struct {
#if defined(__SAM0__)
	DmacDescriptor *descriptor_section;
	DmacDescriptor *write_back_section;
	DmacDescriptor descriptor_tx;
	struct spi_module spi_module;
#elif defined(__XMC1__)
	uint8_t buffer_send_index;
#endif

	uint8_t buffer_send_length;
	uint16_t buffer_send_ack_timeout; // uint16 for testing, can be uint8 later on
	uint8_t current_sequence_number;
	uint8_t last_sequence_number_seen;
	uint32_t last_send_started;

	uint8_t buffer_recv[SPITFP_RECEIVE_BUFFER_SIZE];
	uint8_t buffer_send[TFP_MESSAGE_MAX_LENGTH + SPITFP_PROTOCOL_OVERHEAD*2]; // *2 for send message overhead and additional ACK
	Ringbuffer ringbuffer_recv;
} SPITFP;

#endif
