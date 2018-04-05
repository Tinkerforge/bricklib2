/* bricklib2
 * Copyright (C) 2017 Olaf Lüke <olaf@tinkerforge.com>
 *
 * i2c_fifo.c: with fifo implementation for XMC
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

#include "i2c_fifo.h"

#include "xmc_i2c.h"
#include "xmc_gpio.h"
#include "xmc_usic.h"

typedef enum XMC_I2C_CH_TDF {
	XMC_I2C_CH_TDF_MASTER_SEND         = 0,
	XMC_I2C_CH_TDF_SLAVE_SEND          = 1 << 8,
	XMC_I2C_CH_TDF_MASTER_RECEIVE_ACK  = 2 << 8,
	XMC_I2C_CH_TDF_MASTER_RECEIVE_NACK = 3 << 8,
	XMC_I2C_CH_TDF_MASTER_START        = 4 << 8,
	XMC_I2C_CH_TDF_MASTER_RESTART      = 5 << 8,
	XMC_I2C_CH_TDF_MASTER_STOP         = 6 << 8
} XMC_I2C_CH_TDF_t;

static bool i2c_fifo_ready_or_idle(I2CFifo *i2c_fifo) {
	return (i2c_fifo->state == I2C_FIFO_STATE_IDLE) || (i2c_fifo->state & I2C_FIFO_STATE_READY);
}

// The i2c read/write functions here assume that the FIFO size is never exceeded.
void i2c_fifo_write_register(I2CFifo *i2c_fifo, const uint8_t reg, const uint16_t length, const uint8_t *data, const bool send_stop) {
	if(!i2c_fifo_ready_or_idle(i2c_fifo)) {
		i2c_fifo->state = I2C_FIFO_STATE_WRITE_REGISTER_ERROR;
		return;
	}

	// If we don't send stop we will read directly afterwards, so the state will change to read
	if(send_stop) {
		i2c_fifo->state = I2C_FIFO_STATE_WRITE_REGISTER;
	}

	// I2C Master Start
	i2c_fifo->i2c->IN[0] = (i2c_fifo->address << 1) | XMC_I2C_CH_TDF_MASTER_START;
	// I2C Send 8 bit register
	i2c_fifo->i2c->IN[0] = XMC_I2C_CH_TDF_MASTER_SEND | reg;

	// I2C Master send bytes to write
	for(uint16_t i = 0; i < length; i++) {
		i2c_fifo->i2c->IN[0] = XMC_I2C_CH_TDF_MASTER_SEND | data[i];
	}

	if(send_stop) {
		// I2C Master stop
		i2c_fifo->i2c->IN[0] = (uint32_t)XMC_I2C_CH_TDF_MASTER_STOP;
	}
}

void i2c_fifo_write_direct(I2CFifo *i2c_fifo, const uint16_t length, const uint8_t *data, const bool send_stop) {
	if(!i2c_fifo_ready_or_idle(i2c_fifo)) {
		i2c_fifo->state = I2C_FIFO_STATE_WRITE_DIRECT_ERROR;
		return;
	}

	// If we don't send stop we will read directly afterwards, so the state will change to read
	if(send_stop) {
		i2c_fifo->state = I2C_FIFO_STATE_WRITE_DIRECT;
	}

	// I2C Master Start
	i2c_fifo->i2c->IN[0] = (i2c_fifo->address << 1) | XMC_I2C_CH_TDF_MASTER_START;

	// I2C Master send bytes to write
	for(uint16_t i = 0; i < length; i++) {
		i2c_fifo->i2c->IN[0] = XMC_I2C_CH_TDF_MASTER_SEND | data[i];
	}

	if(send_stop) {
		// I2C Master stop
		i2c_fifo->i2c->IN[0] = (uint32_t)XMC_I2C_CH_TDF_MASTER_STOP;
	}
}

void i2c_fifo_read_register(I2CFifo *i2c_fifo, const uint8_t reg, const uint32_t length) {
	i2c_fifo_write_register(i2c_fifo, reg, 0, NULL, false);

	if(!i2c_fifo_ready_or_idle(i2c_fifo)) {
		i2c_fifo->state = I2C_FIFO_STATE_READ_REGISTER_ERROR;
		return;
	}

	i2c_fifo->state = I2C_FIFO_STATE_READ_REGISTER;
	i2c_fifo->expected_fifo_level = length;

	// I2C Master restart
	i2c_fifo->i2c->IN[0] = XMC_I2C_CH_TDF_MASTER_RESTART | XMC_I2C_CH_CMD_READ | (i2c_fifo->address << 1);

	// I2C Master send ACK/NACK
	if(length != 0) {
		for(uint16_t i = 0; i < length-1; i++) {
			i2c_fifo->i2c->IN[0] = XMC_I2C_CH_TDF_MASTER_RECEIVE_ACK;
		}

		i2c_fifo->i2c->IN[0] = XMC_I2C_CH_TDF_MASTER_RECEIVE_NACK;
	}

	// I2C Master stop
	i2c_fifo->i2c->IN[0] = XMC_I2C_CH_TDF_MASTER_STOP;
}

void i2c_fifo_read_direct(I2CFifo *i2c_fifo, const uint32_t length, const bool restart) {
	if(!i2c_fifo_ready_or_idle(i2c_fifo)) {
		i2c_fifo->state = I2C_FIFO_STATE_READ_DIRECT_ERROR;
		return;
	}

	i2c_fifo->state = I2C_FIFO_STATE_READ_DIRECT;
	i2c_fifo->expected_fifo_level = length;

	// I2C Master start
	i2c_fifo->i2c->IN[0] = (restart ? XMC_I2C_CH_TDF_MASTER_RESTART : XMC_I2C_CH_TDF_MASTER_START) | XMC_I2C_CH_CMD_READ | (i2c_fifo->address << 1) ;

	// I2C Master send ACK/NACK
	if(length != 0) {
		for(uint16_t i = 0; i < length-1; i++) {
			i2c_fifo->i2c->IN[0] = XMC_I2C_CH_TDF_MASTER_RECEIVE_ACK;
		}

		i2c_fifo->i2c->IN[0] = XMC_I2C_CH_TDF_MASTER_RECEIVE_NACK;
	}

	// I2C Master stop
	i2c_fifo->i2c->IN[0] = XMC_I2C_CH_TDF_MASTER_STOP;
}

uint8_t i2c_fifo_read_fifo(I2CFifo *i2c_fifo, uint8_t *buffer, const uint8_t buffer_length) {
	uint8_t length = 0;

	while((!XMC_USIC_CH_RXFIFO_IsEmpty(i2c_fifo->i2c)) && (length < buffer_length)) {
		buffer[length] = XMC_I2C_CH_GetReceivedData(i2c_fifo->i2c);
		length++;
	}

	return length;
}

void i2c_fifo_init(I2CFifo *i2c_fifo) {
	i2c_fifo->state = I2C_FIFO_STATE_IDLE;

	// First de-configure everything, so we can also use
	// this function as i2c reset

	XMC_I2C_CH_Stop(i2c_fifo->i2c);

	const XMC_GPIO_CONFIG_t config_reset =  {
		.mode             = XMC_GPIO_MODE_INPUT_PULL_UP,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_LARGE,
	};

	XMC_GPIO_Init(i2c_fifo->sda_port, i2c_fifo->sda_pin, &config_reset);
	XMC_GPIO_Init(i2c_fifo->scl_port, i2c_fifo->scl_pin, &config_reset);

	XMC_USIC_CH_TXFIFO_Flush(i2c_fifo->i2c);
	XMC_USIC_CH_RXFIFO_Flush(i2c_fifo->i2c);

	WR_REG(i2c_fifo->i2c->FMR, USIC_CH_FMR_MTDV_Msk, USIC_CH_FMR_MTDV_Pos, 2);

	const XMC_I2C_CH_CONFIG_t master_channel_config = {
		.baudrate = i2c_fifo->baudrate,
		.address  = 0
	};

	const XMC_GPIO_CONFIG_t sda_pin_config =  {
		.mode         = i2c_fifo->sda_mode,
		.output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH,
	};

	const XMC_GPIO_CONFIG_t scl_pin_config = {
		.mode         = i2c_fifo->scl_mode,
		.output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH,
	};

	XMC_I2C_CH_Init(i2c_fifo->i2c, &master_channel_config);

	XMC_USIC_CH_SetInputSource(i2c_fifo->i2c, i2c_fifo->sda_input, i2c_fifo->sda_source);
	XMC_USIC_CH_SetInputSource(i2c_fifo->i2c, i2c_fifo->scl_input, i2c_fifo->scl_source);

	XMC_USIC_CH_TXFIFO_Configure(i2c_fifo->i2c, i2c_fifo->sda_fifo_pointer, i2c_fifo->sda_fifo_size, 0);
	XMC_USIC_CH_RXFIFO_Configure(i2c_fifo->i2c, i2c_fifo->scl_fifo_pointer, i2c_fifo->scl_fifo_size, 0);

	XMC_I2C_CH_Start(i2c_fifo->i2c);

	XMC_GPIO_Init(i2c_fifo->sda_port, i2c_fifo->sda_pin, &sda_pin_config);
	XMC_GPIO_Init(i2c_fifo->scl_port, i2c_fifo->scl_pin, &scl_pin_config);
}

I2CFifoState i2c_fifo_next_state(I2CFifo *i2c_fifo) {
	switch(i2c_fifo->state) {
		case I2C_FIFO_STATE_WRITE_REGISTER:
		case I2C_FIFO_STATE_WRITE_DIRECT: {
			i2c_fifo->i2c_status = XMC_I2C_CH_GetStatusFlag(i2c_fifo->i2c);
			if(i2c_fifo->i2c_status & (XMC_I2C_CH_STATUS_FLAG_NACK_RECEIVED |
			                           XMC_I2C_CH_STATUS_FLAG_ARBITRATION_LOST |
			                           XMC_I2C_CH_STATUS_FLAG_ERROR |
			                           XMC_I2C_CH_STATUS_FLAG_WRONG_TDF_CODE_FOUND)) {
				XMC_I2C_CH_ClearStatusFlag(i2c_fifo->i2c, 0xFFFFFFFF);
				i2c_fifo->state |= I2C_FIFO_STATE_ERROR;
			} else if(XMC_USIC_CH_TXFIFO_IsEmpty(i2c_fifo->i2c)) {
				i2c_fifo->state |= I2C_FIFO_STATE_READY;
			}

			break;
		}

		case I2C_FIFO_STATE_READ_REGISTER:
		case I2C_FIFO_STATE_READ_DIRECT: {
			i2c_fifo->i2c_status = XMC_I2C_CH_GetStatusFlag(i2c_fifo->i2c);
			if(i2c_fifo->i2c_status & (XMC_I2C_CH_STATUS_FLAG_NACK_RECEIVED |
			                           XMC_I2C_CH_STATUS_FLAG_ARBITRATION_LOST |
			                           XMC_I2C_CH_STATUS_FLAG_ERROR |
			                           XMC_I2C_CH_STATUS_FLAG_WRONG_TDF_CODE_FOUND)) {
				XMC_I2C_CH_ClearStatusFlag(i2c_fifo->i2c, 0xFFFFFFFF);
				i2c_fifo->expected_fifo_level = 0;
				i2c_fifo->state |= I2C_FIFO_STATE_ERROR;
			} else if(XMC_USIC_CH_RXFIFO_GetLevel(i2c_fifo->i2c) >= i2c_fifo->expected_fifo_level) {
				i2c_fifo->expected_fifo_level = 0;
				i2c_fifo->state |= I2C_FIFO_STATE_READY;
			}

			break;
		}

		default: {
			break;
		}
	}

	return i2c_fifo->state;
}
