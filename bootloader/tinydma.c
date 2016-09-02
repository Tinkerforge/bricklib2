/* brickletboot
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tinydma.c: sam0 DMA driver with small memory/flash footprint
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

#include "tinydma.h"

#include <string.h>

#include "clock.h"
#include "system_interrupt.h"


// Maximum retry counter for resuming a job transfer
#define TINYDMA_MAX_JOB_RESUME_COUNT    10000

#ifdef TINYDMA_USE_INTERNAL_DESCRIPTORS
COMPILER_ALIGNED(16)
static DmacDescriptor tinydma_descriptor_section[TINYDMA_MAX_USED_CHANNEL] SECTION_DMAC_DESCRIPTOR;

COMPILER_ALIGNED(16)
static DmacDescriptor tinydma_write_back_section[TINYDMA_MAX_USED_CHANNEL] SECTION_DMAC_DESCRIPTOR;

DmacDescriptor* tinydma_get_descriptor_section(void) {
	return tinydma_descriptor_section;
}

DmacDescriptor* tinydma_get_write_back_section(void) {
	return tinydma_write_back_section;
}
#endif

#ifdef TINYDNA_MINIMAL_INTERRUPT_HANDLER
void DMAC_Handler(void) {
	system_interrupt_enter_critical_section();

	// Get Pending channel
	const uint8_t active_channel =  DMAC->INTPEND.reg & DMAC_INTPEND_ID_Msk;

	// Select the active channel
	DMAC->CHID.reg = DMAC_CHID_ID(active_channel);

	// Calculate block transfer size of the DMA transfer
	// total_size = descriptor_section[resource->channel_id].BTCNT.reg;
	// write_size = write_back_section[resource->channel_id].BTCNT.reg;

	// DMA channel interrupt handler
	const uint8_t isr = DMAC->CHINTFLAG.reg;
	if(isr & DMAC_CHINTENCLR_TERR) {
		// Clear transfer error flag
		DMAC->CHINTFLAG.reg = DMAC_CHINTENCLR_TERR;
	} else if(isr & DMAC_CHINTENCLR_TCMPL) {
		// Clear the transfer complete flag
		DMAC->CHINTFLAG.reg = DMAC_CHINTENCLR_TCMPL;

		if(active_channel == TINYDMA_SPITFP_TX_INDEX) {
			if(tinydma_descriptor_section[TINYDMA_SPITFP_TX_INDEX].DESCADDR.reg != (uint32_t)&tinydma_descriptor_section[TINYDMA_SPITFP_TX_INDEX]) {
				tinydma_descriptor_section[TINYDMA_SPITFP_TX_INDEX].DESCADDR.reg = (uint32_t)&tinydma_descriptor_section[TINYDMA_SPITFP_TX_INDEX];
				DMAC->CHINTENCLR.reg = DMAC_CHINTENCLR_TCMPL;
			}
		}
	} else if(isr & DMAC_CHINTENCLR_SUSP) {
		// Clear channel suspend flag
		DMAC->CHINTFLAG.reg = DMAC_CHINTENCLR_SUSP;
	}

	system_interrupt_leave_critical_section();
}
#endif


void tinydma_get_channel_config_defaults(TinyDmaChannelConfig *config) {
	// Set as priority 0
	config->priority = DMA_PRIORITY_LEVEL_0;
	// Only software/event trigger
	config->peripheral_trigger = 0;
	// Transaction trigger
	config->trigger_action = DMA_TRIGGER_ACTION_TRANSACTION;

	// Event configurations, no event input/output
	config->event_config.input_action = DMA_EVENT_INPUT_NOACT;
	config->event_config.event_output_enable = false;
#ifdef FEATURE_DMA_CHANNEL_STANDBY
	config->run_in_standby = false;
#endif
}

void tinydma_channel_init(const uint8_t channel_id, TinyDmaChannelConfig *config) {
	system_interrupt_enter_critical_section();

	// Perform a reset for the allocated channel
	DMAC->CHID.reg = DMAC_CHID_ID(channel_id);
	DMAC->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
	DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;

#ifdef FEATURE_DMA_CHANNEL_STANDBY
	if(config->run_in_standby){
		DMAC->CHCTRLA.reg |= DMAC_CHCTRLA_RUNSTDBY;
	}
#endif

	// Select the DMA channel and clear software trigger
	DMAC->CHID.reg = DMAC_CHID_ID(channel_id);
	DMAC->SWTRIGCTRL.reg &= (uint32_t)(~(1 << channel_id));

	uint32_t CHCTRLB_reg = DMAC_CHCTRLB_LVL(config->priority) | DMAC_CHCTRLB_TRIGSRC(config->peripheral_trigger) | DMAC_CHCTRLB_TRIGACT(config->trigger_action);


	if(config->event_config.input_action) {
		CHCTRLB_reg |= DMAC_CHCTRLB_EVIE | DMAC_CHCTRLB_EVACT(config->event_config.input_action);
	}

	// Enable event output, the event output selection is configured in
	// each transfer descriptor
	if(config->event_config.event_output_enable) {
		CHCTRLB_reg |= DMAC_CHCTRLB_EVOE;
	}

	// Write config to CTRLB register
	DMAC->CHCTRLB.reg = CHCTRLB_reg;

	system_interrupt_leave_critical_section();
}

void tinydma_abort_job(const uint8_t channel_id) {
	system_interrupt_enter_critical_section();

	DMAC->CHID.reg = DMAC_CHID_ID(channel_id);
	DMAC->CHCTRLA.reg = 0;

	system_interrupt_leave_critical_section();
}

void tinydma_suspend_job(const uint8_t channel_id) {
	system_interrupt_enter_critical_section();

	// Select the channel
	DMAC->CHID.reg = DMAC_CHID_ID(channel_id);

	//Send the suspend request
	DMAC->CHCTRLB.reg |= DMAC_CHCTRLB_CMD_SUSPEND;

	system_interrupt_leave_critical_section();
}


void tinydma_resume_job(const uint8_t channel_id) {
	uint32_t bitmap_channel;
	uint32_t count = 0;

	// Get bitmap of the allocated DMA channel
	bitmap_channel = (1 << channel_id);
	system_interrupt_enter_critical_section();

	// Send resume request
	DMAC->CHID.reg = DMAC_CHID_ID(channel_id);
	DMAC->CHCTRLB.reg |= DMAC_CHCTRLB_CMD_RESUME;

	system_interrupt_leave_critical_section();

	// Check if transfer job resumed
	for (count = 0; count < TINYDMA_MAX_JOB_RESUME_COUNT; count++) {
		if ((DMAC->BUSYCH.reg & bitmap_channel) == bitmap_channel) {
			break;
		}
	}
}

void tinydma_descriptor_init(DmacDescriptor* descriptor, TinyDmaDescriptorConfig *config) {
	// Set block transfer control
	descriptor->BTCTRL.bit.VALID = config->descriptor_valid;
	descriptor->BTCTRL.bit.EVOSEL = config->event_output_selection;
	descriptor->BTCTRL.bit.BLOCKACT = config->block_action;
	descriptor->BTCTRL.bit.BEATSIZE = config->beat_size;
	descriptor->BTCTRL.bit.SRCINC = config->src_increment_enable;
	descriptor->BTCTRL.bit.DSTINC = config->dst_increment_enable;
	descriptor->BTCTRL.bit.STEPSEL = config->step_selection;
	descriptor->BTCTRL.bit.STEPSIZE = config->step_size;

	// Set transfer size, source address and destination address
	descriptor->BTCNT.reg = config->block_transfer_count;
	descriptor->SRCADDR.reg = config->source_address;
	descriptor->DSTADDR.reg = config->destination_address;

	// Set next transfer descriptor address
	descriptor->DESCADDR.reg = config->next_descriptor_address;
}

void tinydma_init(DmacDescriptor *descriptor_section, DmacDescriptor *write_back_section) {
	// Initialize clocks for DMA
#if (SAML21) || (SAML22) || (SAMC20) || (SAMC21) || (SAMR30)
	system_ahb_clock_set_mask(MCLK_AHBMASK_DMAC);
#else
	system_ahb_clock_set_mask(PM_AHBMASK_DMAC);
	system_apb_clock_set_mask(SYSTEM_CLOCK_APB_APBB, PM_APBBMASK_DMAC);
#endif

	// Perform a software reset before enable DMA controller
	DMAC->CTRL.reg &= ~DMAC_CTRL_DMAENABLE;
	DMAC->CTRL.reg = DMAC_CTRL_SWRST;

	// Setup descriptor base address and write back section base address
	DMAC->BASEADDR.reg = (uint32_t)descriptor_section;
	DMAC->WRBADDR.reg = (uint32_t)write_back_section;

	// Enable all priority level at the same time
	DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xf);

	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_DMA);
}

void tinydma_start_transfer(const uint8_t channel_id) {
	system_interrupt_enter_critical_section();

	// Enable transfer
	DMAC->CHID.reg = DMAC_CHID_ID(channel_id);
	DMAC->CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;

	system_interrupt_leave_critical_section();
}

void tinydma_trigger_transfer(const uint8_t channel_id) {
	DMAC->SWTRIGCTRL.reg |= (1 << channel_id);
}

void tinydma_descriptor_get_config_defaults(TinyDmaDescriptorConfig *config) {
	/* Set descriptor as valid */
	config->descriptor_valid = true;
	/* Disable event output */
	config->event_output_selection = DMA_EVENT_OUTPUT_DISABLE;
	/* No block action */
	config->block_action = DMA_BLOCK_ACTION_NOACT;
	/* Set beat size to one byte */
	config->beat_size = DMA_BEAT_SIZE_BYTE;
	/* Enable source increment */
	config->src_increment_enable = true;
	/* Enable destination increment */
	config->dst_increment_enable = true;
	/* Step size is applied to the destination address */
	config->step_selection = DMA_STEPSEL_DST;
	/* Address increment is beat size multiplied by 1*/
	config->step_size = DMA_ADDRESS_INCREMENT_STEP_SIZE_1;
	/* Default transfer size is set to 0 */
	config->block_transfer_count = 0;
	/* Default source address is set to NULL */
	config->source_address = (uint32_t)NULL;
	/* Default destination address is set to NULL */
	config->destination_address = (uint32_t)NULL;
	/** Next descriptor address set to 0 */
	config->next_descriptor_address = 0;
}
