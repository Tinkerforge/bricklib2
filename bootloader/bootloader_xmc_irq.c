/* bricklib2
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * bootloader_xmc_irq.c: FIFO IRQ handling for TFP SPI
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

#include <stdint.h>

#include "xmc_spi.h"
#include "xmc_gpio.h"
#include "bootloader.h"

// The irqs are compiled for bootloader as well as firmware

#define spitfp_rx_irq_handler IRQ_Hdlr_9
#define spitfp_tx_irq_handler IRQ_Hdlr_10

extern BootloaderStatus bootloader_status;
void __attribute__((optimize("-O3"))) spitfp_tx_irq_handler(void) {
	if(bootloader_status.st.buffer_send_index >= bootloader_status.st.buffer_send_length) {
		XMC_USIC_CH_TXFIFO_DisableEvent(SPITFP_USIC, XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
		return;
	}

	while(!XMC_USIC_CH_TXFIFO_IsFull(SPITFP_USIC)) {
		SPITFP_USIC->IN[0] = bootloader_status.st.buffer_send[bootloader_status.st.buffer_send_index];
		bootloader_status.st.buffer_send_index++;
		if(bootloader_status.st.buffer_send_index == bootloader_status.st.buffer_send_length) {

			// If message is ACK we don't re-send it automatically
			if(bootloader_status.st.buffer_send_length == SPITFP_PROTOCOL_OVERHEAD) {
				bootloader_status.st.buffer_send_length = 0;
			}
			XMC_USIC_CH_TXFIFO_DisableEvent(SPITFP_USIC, XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
			return;
		}
	}
}

void __attribute__((optimize("-O3"))) spitfp_rx_irq_handler(void) {
	while(!XMC_USIC_CH_RXFIFO_IsEmpty(SPITFP_USIC)) {
		bootloader_status.st.ringbuffer_recv.buffer[bootloader_status.st.ringbuffer_recv.end] = SPITFP_USIC->OUTR;
		bootloader_status.st.ringbuffer_recv.end = (bootloader_status.st.ringbuffer_recv.end + 1) & SPITFP_RECEIVE_BUFFER_MASK;

		// The above is equivalent to
		//   ringbuffer_add(&bootloader_status.st.ringbuffer_recv, data);
		// without the check for buffer fullness if buffer size is power of two
	}
}
