/* brickletboot
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tinydma.h: sam0 DMA driver with small memory/flash footprint
 *            loosely based on atmel sam0/dma.h
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

#ifndef TINYDMA_H
#define TINYDMA_H



#include <compiler.h>

#include "configs/config_tinydma.h"

#define TINYDMA_SPITFP_RX_INDEX 0
#define TINYDMA_SPITFP_TX_INDEX 1

#if (SAML21) || (SAML22) || (SAMC20) || (SAMC21) || (SAMR30) || defined(__DOXYGEN__)
#define FEATURE_DMA_CHANNEL_STANDBY
#endif

#define TINYDMA_CURRENT_BUFFER_COUNT_FOR_CHANNEL(i) ((*(uint32_t*)(DMAC->WRBADDR.bit.WRBADDR + i*0x10)) >> 16)

// DMA invalid channel number.
#define DMA_INVALID_CHANNEL        0xff

/** DMA priority level. */
enum dma_priority_level {
	/** Priority level 0. */
	DMA_PRIORITY_LEVEL_0,
	/** Priority level 1. */
	DMA_PRIORITY_LEVEL_1,
	/** Priority level 2. */
	DMA_PRIORITY_LEVEL_2,
	/** Priority level 3. */
	DMA_PRIORITY_LEVEL_3,
};

/** DMA input actions. */
enum dma_event_input_action {
	/** No action. */
	DMA_EVENT_INPUT_NOACT,
	/** Normal transfer and periodic transfer trigger. */
	DMA_EVENT_INPUT_TRIG,
	/** Conditional transfer trigger. */
	DMA_EVENT_INPUT_CTRIG,
	/** Conditional block transfer. */
	DMA_EVENT_INPUT_CBLOCK,
	/** Channel suspend operation. */
	DMA_EVENT_INPUT_SUSPEND,
	/** Channel resume operation. */
	DMA_EVENT_INPUT_RESUME,
	/** Skip next block suspend action. */
	DMA_EVENT_INPUT_SSKIP,
};

/**
 * Address increment step size. These bits select the address increment step
 * size. The setting apply to source or destination address, depending on
 * STEPSEL setting.
 */
enum dma_address_increment_stepsize {
	/** The address is incremented by (beat size * 1). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_1 = 0,
	/** The address is incremented by (beat size * 2). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_2,
	/** The address is incremented by (beat size * 4). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_4,
	/** The address is incremented by (beat size * 8). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_8,
	/** The address is incremented by (beat size * 16). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_16,
	/** The address is incremented by (beat size * 32). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_32,
	/** The address is incremented by (beat size * 64). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_64,
	/** The address is incremented by (beat size * 128). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_128,
};

/**
 * DMA step selection. This bit determines whether the step size setting
 * is applied to source or destination address.
 */
enum dma_step_selection {
	/** Step size settings apply to the destination address. */
	DMA_STEPSEL_DST = 0,
	/** Step size settings apply to the source address. */
	DMA_STEPSEL_SRC,
};

/** The basic transfer unit in DMAC is a beat, which is defined as a
 *  single bus access. Its size is configurable and applies to both read
 *  and write. */
enum dma_beat_size {
	/** 8-bit access. */
	DMA_BEAT_SIZE_BYTE = 0,
	/** 16-bit access. */
	DMA_BEAT_SIZE_HWORD,
	/** 32-bit access. */
	DMA_BEAT_SIZE_WORD,
};

/**
 * Block action definitions.
 */
enum dma_block_action {
	/** No action. */
	DMA_BLOCK_ACTION_NOACT = 0,
	/** Channel in normal operation and sets transfer complete interrupt flag
	 *  after block transfer. */
	DMA_BLOCK_ACTION_INT,
	/** Trigger channel suspend after block transfer and sets channel
	 *  suspend interrupt flag once the channel is suspended. */
	DMA_BLOCK_ACTION_SUSPEND,
	/** Sets transfer complete interrupt flag after a block transfer and
	 *  trigger channel suspend. The channel suspend interrupt flag will be set
	 *  once the channel is suspended. */
	DMA_BLOCK_ACTION_BOTH,
};

/** Event output selection. */
enum dma_event_output_selection {
	/** Event generation disable. */
	DMA_EVENT_OUTPUT_DISABLE = 0,
	/** Event strobe when block transfer complete. */
	DMA_EVENT_OUTPUT_BLOCK,
	/** Event output reserved. */
	DMA_EVENT_OUTPUT_RESERVED,
	/** Event strobe when beat transfer complete. */
	DMA_EVENT_OUTPUT_BEAT,
};

/** DMA trigger action type. */
enum dma_transfer_trigger_action{
	/** Perform a block transfer when triggered. */
	DMA_TRIGGER_ACTION_BLOCK = DMAC_CHCTRLB_TRIGACT_BLOCK_Val,
	/** Perform a beat transfer when triggered. */
	DMA_TRIGGER_ACTION_BEAT = DMAC_CHCTRLB_TRIGACT_BEAT_Val,
	/** Perform a transaction when triggered. */
	DMA_TRIGGER_ACTION_TRANSACTION = DMAC_CHCTRLB_TRIGACT_TRANSACTION_Val,
};

/**
 * DMA transfer descriptor configuration. When the source or destination address
 * increment is enabled, the addresses stored into the configuration structure
 * must correspond to the end of the transfer.
 *
 */
typedef struct {
	/** Descriptor valid flag used to identify whether a descriptor is
	    valid or not */
	bool descriptor_valid;
	/** This is used to generate an event on specific transfer action in
	    a channel. Supported only in four lower channels. */
	enum dma_event_output_selection event_output_selection;
	/** Action taken when a block transfer is completed */
	enum dma_block_action block_action;
	/** Beat size is configurable as 8-bit, 16-bit, or 32-bit */
	enum dma_beat_size beat_size;
	/** Used for enabling the source address increment */
	bool src_increment_enable;
	/** Used for enabling the destination address increment */
	bool dst_increment_enable;
	/** This bit selects whether the source or destination address is
	    using the step size settings */
	enum dma_step_selection step_selection;
	/** The step size for source/destination address increment.
	    The next address is calculated
	    as next_addr = addr + (2^step_size * beat size). */
	enum dma_address_increment_stepsize step_size;
	/** It is the number of beats in a block. This count value is
	 * decremented by one after each beat data transfer. */
	uint16_t block_transfer_count;
	/** Transfer source address */
	uint32_t source_address;
	/** Transfer destination address */
	uint32_t destination_address;
	/** Set to zero for static descriptors. This must have a valid memory
	    address for linked descriptors. */
	uint32_t next_descriptor_address;
} TinyDmaDescriptorConfig;

/** Configurations for DMA events. */
typedef struct {
	/** Event input actions */
	enum dma_event_input_action input_action;
	/** Enable DMA event output */
	bool event_output_enable;
}TinyDmaEventsConfig;

/** DMA configurations for transfer. */
typedef struct {
	/** DMA transfer priority */
	enum dma_priority_level priority;
	/**DMA peripheral trigger index */
	uint8_t peripheral_trigger;
	/** DMA trigger action */
	enum dma_transfer_trigger_action trigger_action;
#ifdef FEATURE_DMA_CHANNEL_STANDBY
	/** Keep DMA channel enabled in standby sleep mode if true */
	bool run_in_standby;
#endif
	/** DMA events configurations */
	TinyDmaEventsConfig event_config;
} TinyDmaChannelConfig;

void tinydma_get_channel_config_defaults(TinyDmaChannelConfig *config);
void tinydma_init(DmacDescriptor *descriptor_section, DmacDescriptor *write_back_section);
void tinydma_start_transfer(const uint8_t channel_id);
void tinydma_trigger_transfer(const uint8_t channel_id);
void tinydma_descriptor_get_config_defaults(TinyDmaDescriptorConfig *config);
void tinydma_resume_job(const uint8_t channel_id);
void tinydma_abort_job(const uint8_t channel_id);
void tinydma_suspend_job(const uint8_t channel_id);
void tinydma_descriptor_init(DmacDescriptor* descriptor, TinyDmaDescriptorConfig *config);
void tinydma_channel_init(const uint8_t channel_id, TinyDmaChannelConfig *config);

#ifdef TINYDMA_USE_INTERNAL_DESCRIPTORS
DmacDescriptor* tinydma_get_descriptor_section(void);
DmacDescriptor* tinydma_get_write_back_section(void);
#endif

#endif
