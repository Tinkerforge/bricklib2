/* bricklib2
 * Copyright (C) 2018 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spi_fifo.h: SPI with fifo implementation for XMC
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
**/

#ifndef SPI_FIFO_H
#define SPI_FIFO_H

#include "xmc_usic.h"
#include "xmc_spi.h"
#include "xmc_gpio.h"

#define SPI_FIFO_STATUS_TIMEOUT 0xFFFFFFFF

typedef enum {
	SPI_FIFO_STATE_IDLE                 = 0,
	SPI_FIFO_STATE_READY                = 1 << 6,
	SPI_FIFO_STATE_ERROR                = 1 << 7,

	SPI_FIFO_STATE_TRANSCEIVE           = 1 << 0,

	SPI_FIFO_STATE_TRANSCEIVE_READY     = SPI_FIFO_STATE_TRANSCEIVE | SPI_FIFO_STATE_READY,
	SPI_FIFO_STATE_TRANSCEIVE_ERROR     = SPI_FIFO_STATE_TRANSCEIVE | SPI_FIFO_STATE_ERROR,

	SPI_FIFO_STATE_MASK                 = 0b00000001
} SPIFifoState;

typedef struct {
	uint32_t baudrate;
	XMC_USIC_CH_t *channel;

	XMC_USIC_CH_FIFO_SIZE_t rx_fifo_size;
	uint8_t rx_fifo_pointer;

	XMC_USIC_CH_FIFO_SIZE_t tx_fifo_size;
	uint8_t tx_fifo_pointer;

	XMC_SPI_CH_BRG_SHIFT_CLOCK_PASSIVE_LEVEL_t clock_passive_level;
	XMC_SPI_CH_BRG_SHIFT_CLOCK_OUTPUT_t clock_output;
	XMC_SPI_CH_SLAVE_SELECT_t slave;

	XMC_GPIO_PORT_t *sclk_port;
	uint8_t sclk_pin;
	XMC_GPIO_MODE_t sclk_pin_mode;

	XMC_GPIO_PORT_t *select_port;
	uint8_t select_pin;
	XMC_GPIO_MODE_t select_pin_mode;

	XMC_GPIO_PORT_t *mosi_port;
	uint8_t mosi_pin;
	XMC_GPIO_MODE_t mosi_pin_mode;

	XMC_GPIO_PORT_t *miso_port;
	uint8_t miso_pin;
	XMC_USIC_CH_INPUT_t miso_input;
	uint8_t miso_source;

	uint8_t *buffer_miso;
	uint8_t expected_fifo_level;
	SPIFifoState state;
	uint32_t spi_status;

	uint32_t last_activity;
} SPIFifo;

#ifdef SPI_FIFO_COOP_ENABLE
bool spi_fifo_coop_transceive(SPIFifo *spi_fifo,  const uint16_t length, const uint8_t *mosi, uint8_t *miso);
#else
void spi_fifo_transceive(SPIFifo *spi_fifo,  const uint16_t length, const uint8_t *data);
uint8_t spi_fifo_read_fifo(SPIFifo *spi_fifo, uint8_t *buffer, const uint8_t buffer_length);
SPIFifoState spi_fifo_next_state(SPIFifo *spi_fifo);
#endif 

void spi_fifo_init(SPIFifo *i2c_fifo);

#endif