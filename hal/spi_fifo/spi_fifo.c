/* bricklib2
 * Copyright (C) 2018 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spi_fifo.c: SPI with fifo implementation for XMC
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
 * 
 *
 * 
 * The SPI FIFO implements simple standard i2c read/write function
 * with the use of the fifo. This implementation assumes that
 * the fifo is big enough to hold one complete read or write.
 *
 * The actual SPI transfer is done completely in hardware, there
 * is no interrupt or similar in use.
 * 
**/

#include "spi_fifo.h"

#include "bricklib2/hal/system_timer/system_timer.h"

#ifndef SPI_FIFO_TIMEOUT
#define SPI_FIFO_TIMEOUT 10 // in ms
#endif

static bool spi_fifo_ready_or_idle(SPIFifo *spi_fifo) {
	return (spi_fifo->state == SPI_FIFO_STATE_IDLE) || (spi_fifo->state & SPI_FIFO_STATE_READY);
}

void spi_fifo_transceive(SPIFifo *spi_fifo,  const uint16_t length, const uint8_t *data) {
	if(!spi_fifo_ready_or_idle(spi_fifo)) {
		spi_fifo->state = SPI_FIFO_STATE_TRANSCEIVE_ERROR;
		return;
	}

	spi_fifo->last_activity = system_timer_get_ms();

	XMC_SPI_CH_EnableSlaveSelect(spi_fifo->channel, spi_fifo->slave);

	// We assume that the data fits in the FIFO
	for(uint16_t i = 0; i < length; i++) {
		spi_fifo->channel->IN[0] = data[i];
	}
	spi_fifo->expected_fifo_level = length;

	spi_fifo->state = SPI_FIFO_STATE_TRANSCEIVE;
}

uint8_t spi_fifo_read_fifo(SPIFifo *spi_fifo, uint8_t *buffer, const uint8_t buffer_length) {
	uint8_t length = 0;

	while((!XMC_USIC_CH_RXFIFO_IsEmpty(spi_fifo->channel)) && (length < buffer_length)) {
        buffer[length] = spi_fifo->channel->OUTR;
		length++;
	}

	return length;
}

void spi_fifo_init(SPIFifo *spi_fifo) {
	// USIC channel configuration
	const XMC_SPI_CH_CONFIG_t channel_config = {
		.baudrate       = spi_fifo->baudrate,
		.bus_mode       = XMC_SPI_CH_BUS_MODE_MASTER,
		.selo_inversion = XMC_SPI_CH_SLAVE_SEL_INV_TO_MSLS,
		.parity_mode    = XMC_USIC_CH_PARITY_MODE_NONE
	};

	// MOSI pin configuration
	const XMC_GPIO_CONFIG_t mosi_pin_config = {
		.mode             = spi_fifo->mosi_pin_mode,
		.output_level     = XMC_GPIO_OUTPUT_LEVEL_HIGH
	};

	// MISO pin configuration
	const XMC_GPIO_CONFIG_t miso_pin_config = {
		.mode             = XMC_GPIO_MODE_INPUT_TRISTATE,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	// SCLK pin configuration
	const XMC_GPIO_CONFIG_t sclk_pin_config = {
		.mode             = spi_fifo->sclk_pin_mode,
		.output_level     = XMC_GPIO_OUTPUT_LEVEL_HIGH
	};

	// SELECT pin configuration
	const XMC_GPIO_CONFIG_t select_pin_config = {
		.mode             = spi_fifo->select_pin_mode,
		.output_level     = XMC_GPIO_OUTPUT_LEVEL_HIGH
	};

	// Configure MISO pin
	XMC_GPIO_Init(spi_fifo->miso_port, spi_fifo->miso_pin, &miso_pin_config);

	// Initialize USIC channel in SPI master mode
	XMC_SPI_CH_Init(spi_fifo->channel, &channel_config);
	spi_fifo->channel->SCTR &= ~USIC_CH_SCTR_PDL_Msk; // Set passive data level to 0
//	UC1701_USIC->PCR_SSCMode &= ~USIC_CH_PCR_SSCMode_TIWEN_Msk; // Disable time between bytes

	XMC_SPI_CH_SetSlaveSelectPolarity(spi_fifo->channel, XMC_SPI_CH_SLAVE_SEL_INV_TO_MSLS);
	XMC_SPI_CH_SetBitOrderMsbFirst(spi_fifo->channel);

	XMC_SPI_CH_SetWordLength(spi_fifo->channel, 8U);
	XMC_SPI_CH_SetFrameLength(spi_fifo->channel, 64U);

	XMC_SPI_CH_SetTransmitMode(spi_fifo->channel, XMC_SPI_CH_MODE_STANDARD);

	// Configure the clock polarity and clock delay
	XMC_SPI_CH_ConfigureShiftClockOutput(spi_fifo->channel,
	                                     spi_fifo->clock_passive_level,
	                                     spi_fifo->clock_output);
	// Configure Leading/Trailing delay
	XMC_SPI_CH_SetSlaveSelectDelay(spi_fifo->channel, 2);

	// Set input source path
	XMC_SPI_CH_SetInputSource(spi_fifo->channel, spi_fifo->miso_input, spi_fifo->miso_source);

	// SPI Mode: CPOL=1 and CPHA=1
	((USIC_CH_TypeDef *)spi_fifo->channel)->DX1CR |= USIC_CH_DX1CR_DPOL_Msk;

	// Configure transmit FIFO
	XMC_USIC_CH_TXFIFO_Configure(spi_fifo->channel, spi_fifo->tx_fifo_pointer, spi_fifo->tx_fifo_size, 8);

	// Configure receive FIFO
	XMC_USIC_CH_RXFIFO_Configure(spi_fifo->channel, spi_fifo->rx_fifo_pointer, spi_fifo->rx_fifo_size, 8);

	// Start SPI
	XMC_SPI_CH_Start(spi_fifo->channel);

	// Configure SCLK pin
	XMC_GPIO_Init(spi_fifo->sclk_port, spi_fifo->sclk_pin, &sclk_pin_config);

	// Configure MOSI pin
	XMC_GPIO_Init(spi_fifo->mosi_port, spi_fifo->mosi_pin, &mosi_pin_config);

	// Configure SELECT pin
	XMC_GPIO_Init(spi_fifo->select_port, spi_fifo->select_pin, &select_pin_config);

	XMC_USIC_CH_RXFIFO_Flush(spi_fifo->channel);

	spi_fifo->state = SPI_FIFO_STATE_IDLE;
}

SPIFifoState spi_fifo_next_state(SPIFifo *spi_fifo) {
	if(spi_fifo->state == SPI_FIFO_STATE_TRANSCEIVE) {
		spi_fifo->spi_status = XMC_SPI_CH_GetStatusFlag(spi_fifo->channel);
		if(spi_fifo->spi_status & (XMC_SPI_CH_STATUS_FLAG_PARITY_ERROR_EVENT_DETECTED | XMC_SPI_CH_STATUS_FLAG_DATA_LOST_INDICATION)) {
			XMC_SPI_CH_DisableSlaveSelect(spi_fifo->channel);
			spi_fifo->state = SPI_FIFO_STATE_TRANSCEIVE_ERROR;
		} else if(XMC_USIC_CH_RXFIFO_GetLevel(spi_fifo->channel) >= spi_fifo->expected_fifo_level) {
			XMC_SPI_CH_DisableSlaveSelect(spi_fifo->channel);
			spi_fifo->state = SPI_FIFO_STATE_TRANSCEIVE_READY;
		} else if(system_timer_is_time_elapsed_ms(spi_fifo->last_activity, SPI_FIFO_TIMEOUT)) {
			spi_fifo->state = SPI_FIFO_STATE_TRANSCEIVE_ERROR;
			spi_fifo->spi_status = SPI_FIFO_STATUS_TIMEOUT;
		}
	}

	return spi_fifo->state;
}