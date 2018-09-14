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

#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/logging/logging.h"

#ifndef I2C_FIFO_TIMEOUT
#define I2C_FIFO_TIMEOUT 10 // in ms
#endif

#ifndef I2C_FIFO_CLEAR_SLEEP
#define I2C_FIFO_CLEAR_SLEEP 10 // in us
#endif

typedef enum XMC_I2C_CH_TDF {
	XMC_I2C_CH_TDF_MASTER_SEND         = 0,
	XMC_I2C_CH_TDF_SLAVE_SEND          = 1 << 8,
	XMC_I2C_CH_TDF_MASTER_RECEIVE_ACK  = 2 << 8,
	XMC_I2C_CH_TDF_MASTER_RECEIVE_NACK = 3 << 8,
	XMC_I2C_CH_TDF_MASTER_START        = 4 << 8,
	XMC_I2C_CH_TDF_MASTER_RESTART      = 5 << 8,
	XMC_I2C_CH_TDF_MASTER_STOP         = 6 << 8
} XMC_I2C_CH_TDF_t;

static void i2c_fifo_clear_bus_low(XMC_GPIO_PORT_t *port, uint8_t pin) {
	XMC_GPIO_SetOutputLow(port, pin);
	system_timer_sleep_us(I2C_FIFO_CLEAR_SLEEP);
}

static void i2c_fifo_clear_bus_high(XMC_GPIO_PORT_t *port, uint8_t pin) {
	XMC_GPIO_SetOutputHigh(port, pin);
	system_timer_sleep_us(I2C_FIFO_CLEAR_SLEEP);
}

static void __attribute__((optimize("-Os"))) i2c_fifo_clear_bus(XMC_GPIO_PORT_t *sda_port, uint8_t sda_pin, XMC_GPIO_PORT_t *scl_port, uint8_t scl_pin) {
	const XMC_GPIO_CONFIG_t config_open_drain =  {
		.mode         = XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN,
		.output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH,
	};
	XMC_GPIO_Init(sda_port, sda_pin, &config_open_drain);
	system_timer_sleep_us(I2C_FIFO_CLEAR_SLEEP);
	XMC_GPIO_Init(scl_port, scl_pin, &config_open_drain);
	system_timer_sleep_us(I2C_FIFO_CLEAR_SLEEP);

	// Generate clock for 9 bits (maximum number of stuck bits)
	for(uint8_t i = 0; i < 9; i++) {
		i2c_fifo_clear_bus_low(scl_port, scl_pin);
		i2c_fifo_clear_bus_high(scl_port, scl_pin);
	}

	// After all data is clocked trough, we generate a stop condition
	i2c_fifo_clear_bus_high(sda_port, sda_pin);
	i2c_fifo_clear_bus_high(scl_port, scl_pin);
	i2c_fifo_clear_bus_low(sda_port, sda_pin);
	i2c_fifo_clear_bus_low(scl_port, scl_pin);
	i2c_fifo_clear_bus_high(scl_port, scl_pin);
	i2c_fifo_clear_bus_high(sda_port, sda_pin);
}

static bool i2c_fifo_ready_or_idle(I2CFifo *i2c_fifo) {
	return (i2c_fifo->state == I2C_FIFO_STATE_IDLE) || (i2c_fifo->state & I2C_FIFO_STATE_READY);
}

// The i2c read/write functions here assume that the FIFO size is never exceeded.
void i2c_fifo_write_register(I2CFifo *i2c_fifo, const uint8_t reg, const uint16_t length, const uint8_t *data, const bool send_stop) {
	if(!i2c_fifo_ready_or_idle(i2c_fifo)) {
		i2c_fifo->state = I2C_FIFO_STATE_WRITE_REGISTER_ERROR;
		return;
	}

	i2c_fifo->last_activity = system_timer_get_ms();

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

	i2c_fifo->last_activity = system_timer_get_ms();

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

	i2c_fifo->last_activity = system_timer_get_ms();

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

	i2c_fifo->last_activity = system_timer_get_ms();

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
#ifdef I2C_FIFO_COOP_USE_MUTEX
	if(i2c_fifo->mutex) {
		return; 
	}
	i2c_fifo->mutex = true;
#endif

	i2c_fifo->state = I2C_FIFO_STATE_IDLE;

	// First de-configure everything, so we can also use
	// this function as i2c reset

	XMC_I2C_CH_Stop(i2c_fifo->i2c);

	if(i2c_fifo->i2c_status == I2C_FIFO_STATUS_TIMEOUT) {
		i2c_fifo_clear_bus(i2c_fifo->sda_port, i2c_fifo->sda_pin, i2c_fifo->scl_port, i2c_fifo->scl_pin);
	}

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

	i2c_fifo->mutex = false;
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
			} else if(system_timer_is_time_elapsed_ms(i2c_fifo->last_activity, I2C_FIFO_TIMEOUT)) {
				i2c_fifo->state |= I2C_FIFO_STATE_ERROR;
				i2c_fifo->i2c_status = I2C_FIFO_STATUS_TIMEOUT;
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
			} else if(system_timer_is_time_elapsed_ms(i2c_fifo->last_activity, I2C_FIFO_TIMEOUT)) {
				i2c_fifo->state |= I2C_FIFO_STATE_ERROR;
				i2c_fifo->i2c_status = I2C_FIFO_STATUS_TIMEOUT;
			}

			break;
		}

		default: {
			break;
		}
	}

	return i2c_fifo->state;
}



#ifdef I2C_FIFO_COOP_ENABLE
uint32_t i2c_fifo_coop_read_register(I2CFifo *i2c_fifo, const uint8_t reg, const uint32_t length, uint8_t *data) {
#ifdef I2C_FIFO_COOP_USE_MUTEX
	if(i2c_fifo->mutex) {
		return I2C_FIFO_STATUS_MUTEX;
	}
	i2c_fifo->mutex = true;
#endif

	i2c_fifo_read_register(i2c_fifo, reg, length);

	while(true) {
		I2CFifoState state = i2c_fifo_next_state(i2c_fifo);
		if(state & I2C_FIFO_STATE_ERROR) {
			loge("I2C FIFO COOP I2C error %d (state %d)\n\r", i2c_fifo->i2c_status, state);
#ifdef I2C_FIFO_COOP_USE_MUTEX
			i2c_fifo->mutex = false;
#endif
			return i2c_fifo->i2c_status;
		}
		if(state != I2C_FIFO_STATE_READ_REGISTER_READY) {
			coop_task_yield();
			continue;
		}

		uint8_t length_read = i2c_fifo_read_fifo(i2c_fifo, data, length);
		if(length_read != length) {
			loge("I2C FIFO COOP unexpected I2C read length: %d vs %d\n\r", length_read, length);
#ifdef I2C_FIFO_COOP_USE_MUTEX
			i2c_fifo->mutex = false;
#endif
			return XMC_I2C_CH_STATUS_FLAG_DATA_LOST_INDICATION;
		}

#ifdef I2C_FIFO_COOP_USE_MUTEX
		i2c_fifo->mutex = false;
#endif
		return 0;
	}
}

uint32_t i2c_fifo_coop_write_register(I2CFifo *i2c_fifo, const uint8_t reg, const uint32_t length, const uint8_t *data, const bool send_stop) {
#ifdef I2C_FIFO_COOP_USE_MUTEX
	if(i2c_fifo->mutex) {
		return I2C_FIFO_STATUS_MUTEX;
	}
	i2c_fifo->mutex = true;
#endif

	i2c_fifo_write_register(i2c_fifo, reg, length, data, send_stop);

	while(true) {
		I2CFifoState state = i2c_fifo_next_state(i2c_fifo);
		if(state & I2C_FIFO_STATE_ERROR) {
			loge("I2C FIFO COOP I2C error %d (state %d)\n\r", i2c_fifo->i2c_status, state);
#ifdef I2C_FIFO_COOP_USE_MUTEX
			i2c_fifo->mutex = false;
#endif
			return i2c_fifo->i2c_status;
		}
		if(state != I2C_FIFO_STATE_WRITE_REGISTER_READY) {
			coop_task_yield();
			continue;
		}

#ifdef I2C_FIFO_COOP_USE_MUTEX
		i2c_fifo->mutex = false;
#endif
		return 0;
	}
}
#endif