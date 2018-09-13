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
 *
 *
 *
 * The I2C FIFO implements simple standard i2c read/write function
 * with the use of the fifo. This implementation assumes that
 * the fifo is big enough to hold one complete read or write.
 *
 * The actual i2c transfer is done completely in hardware, there
 * is no interrupt or similar in use.
 *
**/

#ifndef I2C_FIFO_H
#define I2C_FIFO_H

#include "xmc_usic.h"
#include "xmc_gpio.h"

#include "configs/config.h"

#ifdef I2C_FIFO_COOP_ENABLE
#include "bricklib2/os/coop_task.h"
#endif

#define I2C_FIFO_STATUS_TIMEOUT 0xFFFFFFFF
#define I2C_FIFO_STATUS_MUTEX   0xFFFFFFFE

typedef enum {
	I2C_FIFO_STATE_IDLE                 = 0,
	I2C_FIFO_STATE_READY                = 1 << 6,
	I2C_FIFO_STATE_ERROR                = 1 << 7,

	I2C_FIFO_STATE_WRITE_REGISTER       = 1 << 0,
	I2C_FIFO_STATE_WRITE_DIRECT         = 1 << 1,
	I2C_FIFO_STATE_READ_REGISTER        = 1 << 2,
	I2C_FIFO_STATE_READ_DIRECT          = 1 << 3,

	I2C_FIFO_STATE_WRITE_REGISTER_READY = I2C_FIFO_STATE_WRITE_REGISTER | I2C_FIFO_STATE_READY,
	I2C_FIFO_STATE_WRITE_REGISTER_ERROR = I2C_FIFO_STATE_WRITE_REGISTER | I2C_FIFO_STATE_ERROR,

	I2C_FIFO_STATE_WRITE_DIRECT_READY   = I2C_FIFO_STATE_WRITE_DIRECT   | I2C_FIFO_STATE_READY,
	I2C_FIFO_STATE_WRITE_DIRECT_ERROR   = I2C_FIFO_STATE_WRITE_DIRECT   | I2C_FIFO_STATE_ERROR,

	I2C_FIFO_STATE_READ_REGISTER_READY  = I2C_FIFO_STATE_READ_REGISTER  | I2C_FIFO_STATE_READY,
	I2C_FIFO_STATE_READ_REGISTER_ERROR  = I2C_FIFO_STATE_READ_REGISTER  | I2C_FIFO_STATE_ERROR,

	I2C_FIFO_STATE_READ_DIRECT_READY    = I2C_FIFO_STATE_READ_DIRECT    | I2C_FIFO_STATE_READY,
	I2C_FIFO_STATE_READ_DIRECT_ERROR    = I2C_FIFO_STATE_READ_DIRECT    | I2C_FIFO_STATE_ERROR,

	I2C_FIFO_STATE_MASK                 = 0b00111111
} I2CFifoState;

typedef struct {
	uint32_t baudrate;
	uint8_t address;
	XMC_USIC_CH_t *i2c;

	XMC_GPIO_PORT_t *scl_port;
	uint8_t scl_pin;
	XMC_GPIO_MODE_t scl_mode;
	XMC_USIC_CH_INPUT_t scl_input;
	uint8_t scl_source;
	XMC_USIC_CH_FIFO_SIZE_t scl_fifo_size;
	uint8_t scl_fifo_pointer;

	XMC_GPIO_PORT_t *sda_port;
	uint8_t sda_pin;
	XMC_GPIO_MODE_t sda_mode;
	XMC_USIC_CH_INPUT_t sda_input;
	uint8_t sda_source;
	XMC_USIC_CH_FIFO_SIZE_t sda_fifo_size;
	uint8_t sda_fifo_pointer;

	uint8_t expected_fifo_level;
	I2CFifoState state;
	uint32_t i2c_status;

	uint32_t last_activity;
} I2CFifo;


void i2c_fifo_write_register(I2CFifo *i2c_fifo, const uint8_t reg, const uint16_t length, const uint8_t *data, const bool send_stop);
void i2c_fifo_write_direct(I2CFifo *i2c_fifo, const uint16_t length, const uint8_t *data, const bool send_stop);
void i2c_fifo_read_register(I2CFifo *i2c_fifo, const uint8_t reg, const uint32_t length);
void i2c_fifo_read_direct(I2CFifo *i2c_fifo, const uint32_t length, const bool restart);
uint8_t i2c_fifo_read_fifo(I2CFifo *i2c_fifo, uint8_t *buffer, const uint8_t buffer_length);
void i2c_fifo_init(I2CFifo *i2c_fifo);
I2CFifoState i2c_fifo_next_state(I2CFifo *i2c_fifo);

#ifdef I2C_FIFO_COOP_ENABLE
uint32_t i2c_fifo_coop_read_register(I2CFifo *i2c_fifo, const uint8_t reg, const uint32_t length, uint8_t *data);
uint32_t i2c_fifo_coop_write_register(I2CFifo *i2c_fifo, const uint8_t reg, const uint32_t length, const uint8_t *data, const bool send_stop);
#endif

#endif
