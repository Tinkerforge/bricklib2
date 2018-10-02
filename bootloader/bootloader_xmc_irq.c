/* bricklib2
 * Copyright (C) 2016-2018 Olaf Lüke <olaf@tinkerforge.com>
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

// Save pointers for start and overhead end so we can use them in the interrupts
// without the struct access overhead
uint8_t * const buffer_send_pointer_start = bootloader_status.st.buffer_send;

// Only use this define with Bricklets released after 2018 October 1st.
// All bricklets before this date have an incompatible bootloader
#ifdef BOOTLOADER_FIX_POINTER_END
uint8_t * const buffer_send_pointer_protocol_overhead_end = bootloader_status.st.buffer_send + SPITFP_PROTOCOL_OVERHEAD;
#else
uint8_t * const buffer_send_pointer_protocol_overhead_end = bootloader_status.st.buffer_send + SPITFP_PROTOCOL_OVERHEAD-1;
#endif

// Save pointers for recv buffer and ringbuffer so we can use them in the interrupts
// without the struct access overhead
Ringbuffer *ringbuffer_recv = &bootloader_status.st.ringbuffer_recv;
uint8_t *ringbuffer_recv_buffer = bootloader_status.st.buffer_recv;

void __attribute__((optimize("-O3"))) __attribute__((section (".ram_code"))) spitfp_tx_irq_handler(void) {
	// Use local pointer to save the time for accessing the structs
	uint8_t *buffer_send_pointer     = bootloader_status.st.buffer_send_pointer;
	uint8_t *buffer_send_pointer_end = bootloader_status.st.buffer_send_pointer_end;

	while(!XMC_USIC_CH_TXFIFO_IsFull(SPITFP_USIC)) {
		SPITFP_USIC->IN[0] = *buffer_send_pointer;
		buffer_send_pointer++;
		if(buffer_send_pointer == buffer_send_pointer_end) {
#ifndef BOOTLOADER_FIX_POINTER_END
			// In the bootloader we check for buffer_send_pointer == buffer_send_pointer_end as a condition
			// to check if the message was completely send. Because of this we have to make sure that the
			// last byte is definitely send, so we may need to busy wait for the last byte here.

			// Since we can't update the bootloader for existing Bricklets, this is the best solution
			// we were able to come up with.

			// This check will rarely be true, so the time penalty is acceptable.
			while(XMC_USIC_CH_TXFIFO_IsFull(SPITFP_USIC)) {
				__NOP();
			}
			SPITFP_USIC->IN[0] = *buffer_send_pointer;
#endif

			// If message is ACK we don't re-send it automatically
			if(buffer_send_pointer_end == buffer_send_pointer_protocol_overhead_end) {
				buffer_send_pointer_end = buffer_send_pointer_start;
			}

			XMC_USIC_CH_TXFIFO_DisableEvent(SPITFP_USIC, XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
			XMC_USIC_CH_TXFIFO_ClearEvent(SPITFP_USIC, USIC_CH_TRBSCR_CSTBI_Msk);
			NVIC_ClearPendingIRQ(SPITFP_IRQ_TX);

			break;
		}
	}

	// Save local pointer again
	bootloader_status.st.buffer_send_pointer     = buffer_send_pointer;
	bootloader_status.st.buffer_send_pointer_end = buffer_send_pointer_end;
}

void __attribute__((optimize("-O3"))) __attribute__((section (".ram_code"))) spitfp_rx_irq_handler(void) {
	while(!XMC_USIC_CH_RXFIFO_IsEmpty(SPITFP_USIC)) {
		ringbuffer_recv_buffer[ringbuffer_recv->end] = SPITFP_USIC->OUTR;
		ringbuffer_recv->end = (ringbuffer_recv->end + 1) & SPITFP_RECEIVE_BUFFER_MASK;

		// Without this "if" the interrupt takes 1.39us, including the "if" it takes 1.75us
		// Without the "if" it still works, but we loose the overflow counter,
		// we will get frame/checksum errors instead
#ifndef BOOTLOADER_XMC_RX_IRQ_NO_OVERFLOW
		// Tell GCC that this branch is unlikely to occur => __builtin_expect(value, 0)
		if(__builtin_expect((ringbuffer_recv->end == ringbuffer_recv->start), 0)) {
			bootloader_status.error_count.error_count_overflow++;
			ringbuffer_recv->end = (ringbuffer_recv->end - 1) & SPITFP_RECEIVE_BUFFER_MASK;
		}
#endif

	}
}
