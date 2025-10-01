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
#include "bricklib2/utility/util_definitions.h"

// Redefine enum here, so we can compare with CPP
#define XMC_USIC_CH_FIFO_DISABLED     0
#define XMC_USIC_CH_FIFO_SIZE_2WORDS  1
#define XMC_USIC_CH_FIFO_SIZE_4WORDS  2
#define XMC_USIC_CH_FIFO_SIZE_8WORDS  3
#define XMC_USIC_CH_FIFO_SIZE_16WORDS 4
#define XMC_USIC_CH_FIFO_SIZE_32WORDS 5
#define XMC_USIC_CH_FIFO_SIZE_64WORDS 6
#include "bootloader.h"

// The irqs are compiled for bootloader as well as firmware
#ifndef SPITFP_IRQ_RX_HANDLER
#define SPITFP_IRQ_RX_HANDLER IRQ_Hdlr_9
#endif

#ifndef SPITFP_IRQ_TX_HANDLER
#define SPITFP_IRQ_TX_HANDLER IRQ_Hdlr_10
#endif

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

#ifdef BOOTLOADER_USE_MEMORY_OPTIMIZED_IRQ_HANDLER
void __attribute__((optimize("-O3"))) __attribute__((section (".ram_code"))) SPITFP_IRQ_TX_HANDLER(void) {
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

#else

// Sending 16 bytes per interrupt with this version takes 3us-4us (measured with logic analyzer).
// Naive version with while-loop takes 17us.
void __attribute__((optimize("-O3"))) __attribute__((section (".ram_code"))) SPITFP_IRQ_TX_HANDLER(void) {
	// Use local pointer to save the time for accessing the structs
	uint8_t *buffer_send_pointer     = bootloader_status.st.buffer_send_pointer;
	uint8_t *buffer_send_pointer_end = bootloader_status.st.buffer_send_pointer_end;

	const uint8_t to_send = buffer_send_pointer_end - buffer_send_pointer;

#if   SPITFP_TX_SIZE == XMC_USIC_CH_FIFO_SIZE_2WORDS
	const uint8_t fifo_level = 2  - XMC_USIC_CH_TXFIFO_GetLevel(SPITFP_USIC);
#elif SPITFP_TX_SIZE == XMC_USIC_CH_FIFO_SIZE_4WORDS
	const uint8_t fifo_level = 4  - XMC_USIC_CH_TXFIFO_GetLevel(SPITFP_USIC);
#elif SPITFP_TX_SIZE == XMC_USIC_CH_FIFO_SIZE_8WORDS
	const uint8_t fifo_level = 8  - XMC_USIC_CH_TXFIFO_GetLevel(SPITFP_USIC);
#elif SPITFP_TX_SIZE == XMC_USIC_CH_FIFO_SIZE_16WORDS
	const uint8_t fifo_level = 16 - XMC_USIC_CH_TXFIFO_GetLevel(SPITFP_USIC);
#elif SPITFP_TX_SIZE == XMC_USIC_CH_FIFO_SIZE_32WORDS
	const uint8_t fifo_level = 32 - XMC_USIC_CH_TXFIFO_GetLevel(SPITFP_USIC);
#else
	#error "Invalid spitfp tx size"
#endif

	const uint8_t length = MIN(to_send, fifo_level);

	// Use local pointer to save the time for accessing the struct
	volatile uint32_t *SPITFP_USIC_IN_PTR = SPITFP_USIC->IN;

#ifndef SPITFP_NOT_ALLOWED_TO_DISABLE_IRQ
	// Disable IRQ while we write the data. If we don't disable the IRQs here, there
	// is a minor race condition:
	// If a IRQ with higher priority is triggered exactly after the first byte is
	// written to the FIFO and this IRQ takes longer then it takes to transfer one
	// byte we will send erronous data (instead of A B C we send A 0 B C).
	// The CRC of this message does not fit, so this will be resolved through the
	// Brick by requesting the message again.
	//
	// However, for Bricklets that do not have any real-time requirement for
	// the high priority IRQs (or no IRQs), we can disable the irqs here.
	//
	// Bricklets that have real-time requirement for IRQs have to set the
	// "SPITFP_NOT_ALLOWED_TO_DISABLE_IRQ" define.
	__disable_irq();
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
	switch(length) {
#if (SPITFP_TX_SIZE == XMC_USIC_CH_FIFO_SIZE_32WORDS)
		case 32: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 31: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 30: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 29: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 28: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 27: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 26: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 25: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 24: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 23: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 22: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 21: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 20: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 19: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 18: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 17: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
#endif

#if (SPITFP_TX_SIZE == XMC_USIC_CH_FIFO_SIZE_16WORDS) || (SPITFP_TX_SIZE == XMC_USIC_CH_FIFO_SIZE_32WORDS)
		case 16: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 15: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 14: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 13: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 12: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 11: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 10: SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 9:  SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
#endif

#if (SPITFP_TX_SIZE == XMC_USIC_CH_FIFO_SIZE_8WORDS) || (SPITFP_TX_SIZE == XMC_USIC_CH_FIFO_SIZE_16WORDS) || (SPITFP_TX_SIZE == XMC_USIC_CH_FIFO_SIZE_32WORDS)
		case 8:  SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 7:  SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 6:  SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 5:  SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
#endif

#if (SPITFP_TX_SIZE == XMC_USIC_CH_FIFO_SIZE_4WORDS) || (SPITFP_TX_SIZE == XMC_USIC_CH_FIFO_SIZE_8WORDS) || (SPITFP_TX_SIZE == XMC_USIC_CH_FIFO_SIZE_16WORDS) || (SPITFP_TX_SIZE == XMC_USIC_CH_FIFO_SIZE_32WORDS)
		case 4:  SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 3:  SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
#endif

		case 2:  SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;
		case 1:  SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer; buffer_send_pointer++;

		default: case 0: break;
	}
#pragma GCC diagnostic pop

#ifndef SPITFP_NOT_ALLOWED_TO_DISABLE_IRQ
	__enable_irq();
#endif

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
		SPITFP_USIC_IN_PTR[0] = *buffer_send_pointer;
#endif

		// If message is ACK we don't re-send it automatically
		if(buffer_send_pointer_end == buffer_send_pointer_protocol_overhead_end) {
			buffer_send_pointer_end = buffer_send_pointer_start;
		}

		XMC_USIC_CH_TXFIFO_DisableEvent(SPITFP_USIC, XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
		XMC_USIC_CH_TXFIFO_ClearEvent(SPITFP_USIC, USIC_CH_TRBSCR_CSTBI_Msk);
		NVIC_ClearPendingIRQ(SPITFP_IRQ_TX);
	}

	// Save local pointer again
	bootloader_status.st.buffer_send_pointer     = buffer_send_pointer;
	bootloader_status.st.buffer_send_pointer_end = buffer_send_pointer_end;
}
#endif


void __attribute__((optimize("-O3"))) __attribute__((section (".ram_code"))) SPITFP_IRQ_RX_HANDLER(void) {
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
