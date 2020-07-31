/* TNG
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tng_energy_monitor.c: TNG energy monitor driver (PAC193X)
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

#include "tng_energy_monitor.h"
#include "configs/config_energy_monitor.h"

#include "bricklib2/logging/logging.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/utility/util_definitions.h"

TNGEnergyMonitor tng_energy_monitor;

#if 0
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *I2cHandle) {
	logd("HAL_I2C_MasterTxCpltCallback\n\r");
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *I2cHandle) {
	logd("HAL_I2C_SlaveTxCpltCallback\n\r");
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *I2cHandle) {
	logd("HAL_I2C_MasterRxCpltCallback\n\r");
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *I2cHandle) {
	logd("HAL_I2C_SlaveRxCpltCallback\n\r");
}
#endif

void I2C2_IRQHandler(void) {
	HAL_I2C_EV_IRQHandler(&tng_energy_monitor.pac193x_i2c);
	HAL_I2C_ER_IRQHandler(&tng_energy_monitor.pac193x_i2c);  
}

void DMA1_Channel4_5_IRQHandler(void) {
	HAL_DMA_IRQHandler(tng_energy_monitor.pac193x_i2c.hdmarx);
	HAL_DMA_IRQHandler(tng_energy_monitor.pac193x_i2c.hdmatx);
}

// Blocking write register
static void pac193x_write_register(uint8_t reg, uint8_t length, uint8_t *data) {
	if(length > 128) {
		return;
	}

	uint8_t buffer[129] = {0};
	buffer[0] = reg;
	memcpy(&buffer[1], data, length);

	HAL_StatusTypeDef status = HAL_I2C_Master_Transmit_DMA(&tng_energy_monitor.pac193x_i2c, PAC193X_ADDRESS, buffer, length+1);
	if(status != HAL_OK) {
		loge("Error during pac193x_write_register transmit: %d\n\r", status);
	}
	while(HAL_I2C_GetState(&tng_energy_monitor.pac193x_i2c) != HAL_I2C_STATE_READY);
}

// Blocking read register
static void pac193x_read_register(uint8_t reg, uint8_t length, uint8_t *data) {
	HAL_StatusTypeDef status = HAL_I2C_Master_Transmit_DMA(&tng_energy_monitor.pac193x_i2c, PAC193X_ADDRESS, &reg, 1);
	if(status != HAL_OK) {
		loge("Error during pac193x_read_register transmit: %d\n\r", status);
	}
	while(HAL_I2C_GetState(&tng_energy_monitor.pac193x_i2c) != HAL_I2C_STATE_READY);

	status = HAL_I2C_Master_Receive_DMA(&tng_energy_monitor.pac193x_i2c, PAC193X_ADDRESS, data, length);
	if(status != HAL_OK) {
		loge("Error during pac193x_read_register receive: %d\n\r", status);
	}
	while(HAL_I2C_GetState(&tng_energy_monitor.pac193x_i2c) != HAL_I2C_STATE_READY);	
}

static void pac193x_init_i2c(void) {
	// Enable clocks
	__HAL_RCC_I2C2_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();

	// Configure the DMA TX
	static DMA_HandleTypeDef hdma_tx;
	hdma_tx.Instance                 = DMA1_Channel4; // I2Cx_DMA_INSTANCE_TX
	hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
	hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
	hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
	hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
	hdma_tx.Init.Mode                = DMA_NORMAL;
	hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;
	HAL_DMA_Init(&hdma_tx);   
	__HAL_LINKDMA(&tng_energy_monitor.pac193x_i2c, hdmatx, hdma_tx);
    
	// Configure the DMA TX
	static DMA_HandleTypeDef hdma_rx;
	hdma_rx.Instance                 = DMA1_Channel5; //I2Cx_DMA_INSTANCE_RX
	hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
	hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
	hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
	hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
	hdma_rx.Init.Mode                = DMA_NORMAL;
	hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
	HAL_DMA_Init(&hdma_rx);
	__HAL_LINKDMA(&tng_energy_monitor.pac193x_i2c, hdmarx, hdma_rx);
    
	// Configure the NVIC for DMA and I2C
	HAL_NVIC_SetPriority(DMA1_Channel4_5_IRQn, 0, 1); // I2Cx_DMA_IRQn
	HAL_NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);
	HAL_NVIC_SetPriority(I2C2_IRQn, 0, 1);
	HAL_NVIC_EnableIRQ(I2C2_IRQn);

	// Configure SDA
	GPIO_InitTypeDef gpio_sda = {
		.Pin       = PAC193X_SDA_PIN,
		.Mode      = GPIO_MODE_AF_OD,
		.Pull      = GPIO_PULLUP,
		.Speed     = GPIO_SPEED_FREQ_HIGH,
		.Alternate = PAC193X_SDA_AF
	};
	HAL_GPIO_Init(PAC193X_SDA_PORT, &gpio_sda);

	// Configure SCL
	GPIO_InitTypeDef gpio_scl = {
		.Pin       = PAC193X_SCL_PIN,
		.Mode      = GPIO_MODE_AF_OD,
		.Pull      = GPIO_PULLUP,
		.Speed     = GPIO_SPEED_FREQ_HIGH,
		.Alternate = PAC193X_SCL_AF
	};
	HAL_GPIO_Init(PAC193X_SCL_PORT, &gpio_scl);

	tng_energy_monitor.pac193x_i2c.Instance                  = PAC193X_INSTANCE;
	tng_energy_monitor.pac193x_i2c.Init.Timing               = PAC193X_TIMING; // 0x00E0D3FF?
	tng_energy_monitor.pac193x_i2c.Init.DualAddressMode      = I2C_DUALADDRESS_DISABLE;
	tng_energy_monitor.pac193x_i2c.Init.OwnAddress1          = 0x00;
	tng_energy_monitor.pac193x_i2c.Init.AddressingMode       = I2C_ADDRESSINGMODE_7BIT;
	tng_energy_monitor.pac193x_i2c.Init.OwnAddress2          = 0x00;
	tng_energy_monitor.pac193x_i2c.Init.GeneralCallMode      = I2C_GENERALCALL_DISABLE;
	tng_energy_monitor.pac193x_i2c.Init.NoStretchMode        = I2C_NOSTRETCH_DISABLE;  

	HAL_StatusTypeDef status;
	if((status = HAL_I2C_Init(&tng_energy_monitor.pac193x_i2c)) != HAL_OK) {
		loge("HAL_I2C_Init Error %d\n\r", status);
	}

	HAL_I2CEx_ConfigAnalogFilter(&tng_energy_monitor.pac193x_i2c, I2C_ANALOGFILTER_ENABLE);
}

static void pac193x_init(void) {
	pac193x_init_i2c();

#if LOGGING_LEVEL == LOGGING_DEBUG
	uint8_t ids[3] = {0, 0, 0};
	pac193x_read_register(0xFD, 3, ids);
	logd("PAC193X: Product ID %x, Manufacturer ID %x, Revision ID %x\n\r", ids[0], ids[1], ids[2]);
#endif

	// Enable negative current measurements.
	// We always use the absolute value.
	// Because of routing-optimizations on PCB may get negative values.
	uint8_t neg_pwr = 0b11110000;
	pac193x_write_register(0x1D, 1, &neg_pwr);
}

static void pac193x_debug_print_reg_line(char *reg, char *name, uint8_t *data, uint8_t length) {
	uartbb_puts("* ");
	uartbb_puts(reg);
	uartbb_puts(" ");
	uartbb_puts(name);
	for(uint8_t i = 0; i < length; i++) {
		uartbb_printf(" %d", data[i]);
	}
	uartbb_puts(" -> ");

	uint32_t value = 0;
	for(uint32_t i = 0; i < length; i++) {
		value |= (data[i] << ((length - 1 - i)*8));
	}

	int32_t value_signed = INTN_TO_INT32(value, length*8);
	uartbb_printf("%d", value_signed);

	uartbb_puts("\n\r");
}

static void pac193x_debug_print(void) {
	uartbb_printf("Register 0x01 - 0x1A:\n\r");
	pac193x_debug_print_reg_line("0x01", "CTRL:        ", &tng_energy_monitor.pac193x_read_register.ctrl,          1);
	pac193x_debug_print_reg_line("0x02", "ACC_COUNT:   ",  tng_energy_monitor.pac193x_read_register.acc_count,     3);
	pac193x_debug_print_reg_line("0x03", "VPOWER1_ACC: ",  tng_energy_monitor.pac193x_read_register.vpower_acc[0], 6);
	pac193x_debug_print_reg_line("0x04", "VPOWER2_ACC: ",  tng_energy_monitor.pac193x_read_register.vpower_acc[1], 6);
	pac193x_debug_print_reg_line("0x05", "VPOWER3_ACC: ",  tng_energy_monitor.pac193x_read_register.vpower_acc[2], 6);
	pac193x_debug_print_reg_line("0x07", "VBUS1:       ",  tng_energy_monitor.pac193x_read_register.vbus[0],       2);
	pac193x_debug_print_reg_line("0x08", "VBUS2:       ",  tng_energy_monitor.pac193x_read_register.vbus[1],       2);
	pac193x_debug_print_reg_line("0x09", "VBUS3:       ",  tng_energy_monitor.pac193x_read_register.vbus[2],       2);
	pac193x_debug_print_reg_line("0x0B", "VSENSE1:     ",  tng_energy_monitor.pac193x_read_register.vsense[0],     2);
	pac193x_debug_print_reg_line("0x0C", "VSENSE2:     ",  tng_energy_monitor.pac193x_read_register.vsense[1],     2);
	pac193x_debug_print_reg_line("0x0D", "VSENSE3:     ",  tng_energy_monitor.pac193x_read_register.vsense[2],     2);
	pac193x_debug_print_reg_line("0x0F", "VBUS1_AVG:   ",  tng_energy_monitor.pac193x_read_register.vbus_avg[0],   2);
	pac193x_debug_print_reg_line("0x10", "VBUS2_AVG:   ",  tng_energy_monitor.pac193x_read_register.vbus_avg[1],   2);
	pac193x_debug_print_reg_line("0x11", "VBUS3_AVG:   ",  tng_energy_monitor.pac193x_read_register.vbus_avg[2],   2);
	pac193x_debug_print_reg_line("0x13", "VSENSE1_AVG: ",  tng_energy_monitor.pac193x_read_register.vsense_avg[0], 2);
	pac193x_debug_print_reg_line("0x14", "VSENSE2_AVG: ",  tng_energy_monitor.pac193x_read_register.vsense_avg[1], 2);
	pac193x_debug_print_reg_line("0x15", "VSENSE3_AVG: ",  tng_energy_monitor.pac193x_read_register.vsense_avg[2], 2);
	pac193x_debug_print_reg_line("0x17", "VPOWER1:     ",  tng_energy_monitor.pac193x_read_register.vpower[0],     4);
	pac193x_debug_print_reg_line("0x18", "VPOWER2:     ",  tng_energy_monitor.pac193x_read_register.vpower[1],     4);
	pac193x_debug_print_reg_line("0x19", "VPOWER3:     ",  tng_energy_monitor.pac193x_read_register.vpower[2],     4);
	uartbb_puts("\n\r");


	for(uint8_t i = 0; i < PAC193X_CHANNEL_NUM; i++) {
		uartbb_printf("ch%d: %dmV, %dmA, %dmW, %dmWs\n\r", i, tng_energy_monitor.voltage[i], tng_energy_monitor.current[i], tng_energy_monitor.power[i], (int32_t)tng_energy_monitor.energy[i]);
	}
	uartbb_puts("\n\r");
}

static void pac193x_update_values(PAC193XReadRegister *read_reg_tmp) {
	memcpy(&tng_energy_monitor.pac193x_read_register, read_reg_tmp, sizeof(PAC193XReadRegister));

	for(uint8_t i = 0; i < PAC193X_CHANNEL_NUM; i++) {
		uint8_t *x;

		// 32000/65536 = 125/256
		x = tng_energy_monitor.pac193x_read_register.vbus_avg[i];
		tng_energy_monitor.voltage[i] = ((uint32_t)((x[0] << 8) | (x[1] << 0)))*125/256;

		// 10000/65536 = 625/4096
		x = tng_energy_monitor.pac193x_read_register.vsense_avg[i];
		tng_energy_monitor.current[i] = ABS(INTN_TO_INT32((x[0] << 8) | (x[1] << 0), 16)*625/4096);

		// 160*1000/(134217728*count) = 625/(524288*count)
		x = tng_energy_monitor.pac193x_read_register.acc_count;
		const uint32_t count = (uint32_t)((x[0] << 16) | (x[1] << 8) | (x[2] << 0));
		x = tng_energy_monitor.pac193x_read_register.vpower_acc[i];
		const uint64_t x64[6] = {x[0], x[1], x[2], x[3], x[4], x[5]};
		tng_energy_monitor.power[i] = ABS(INTN_TO_INT64((x64[0] << 40) | (x64[1] << 32) | (x64[2] << 24) | (x64[3] << 16) | (x64[4] << 8) | (x64[5] << 0), 48))*625/(524288*count);
		tng_energy_monitor.energy[i] += tng_energy_monitor.power[i]/5; // (milliwatt-seconds)
	}


	pac193x_debug_print();
}

static void pac193x_tick(void) {
	static uint32_t wait_start_time = 0;
	static PAC193XReadRegister read_reg_tmp;

	// Non-blocking refresh/read state-machine
	// Do not use blocking pac193x_read/write_register in tick!
	do {
	switch(tng_energy_monitor.pac193x_state) {
		case PAC193X_STATE_REFRESH: {
			uint8_t refresh[2] = {PAC193X_REG_REFRESH, 0};
			HAL_StatusTypeDef status = HAL_I2C_Master_Transmit_DMA(&tng_energy_monitor.pac193x_i2c, PAC193X_ADDRESS, refresh, 2);
			if(status != HAL_OK) {
				loge("Error during PAC193X_STATE_REFRESH transmit: %d\n\r", status);
			} else {
				tng_energy_monitor.pac193x_state = PAC193X_STATE_REFRESH_WAIT;
			}
			break;
		}

		case PAC193X_STATE_REFRESH_WAIT: {
			HAL_I2C_StateTypeDef state = HAL_I2C_GetState(&tng_energy_monitor.pac193x_i2c);
			if(state == HAL_I2C_STATE_READY) {
				tng_energy_monitor.pac193x_state = PAC193X_STATE_READ_TX;
				wait_start_time = system_timer_get_ms();
			}
			break;
		}

		case PAC193X_STATE_READ_TX: {
			if((wait_start_time == 0) || system_timer_is_time_elapsed_ms(wait_start_time, 2)) {
				wait_start_time = 0;
				uint8_t reg = PAC193X_REG_CTRL; // Start reading from CTRL reg
				HAL_StatusTypeDef status = HAL_I2C_Master_Transmit_DMA(&tng_energy_monitor.pac193x_i2c, PAC193X_ADDRESS, &reg, 1);
				if(status != HAL_OK) {
					loge("Error during PAC193X_STATE_READ_TX transmit: %d\n\r", status);
				} else {
					tng_energy_monitor.pac193x_state = PAC193X_STATE_READ_TX_WAIT;
				}
			}
			break;
		}

		case PAC193X_STATE_READ_TX_WAIT: {
			HAL_I2C_StateTypeDef state = HAL_I2C_GetState(&tng_energy_monitor.pac193x_i2c);
			if(state == HAL_I2C_STATE_READY) {
				tng_energy_monitor.pac193x_state = PAC193X_STATE_READ_RX;
			}
			break;
		}

		case PAC193X_STATE_READ_RX: {
			memset(&read_reg_tmp, 0, sizeof(PAC193XReadRegister));
			HAL_StatusTypeDef status = HAL_I2C_Master_Receive_DMA(&tng_energy_monitor.pac193x_i2c, PAC193X_ADDRESS, (uint8_t *)&read_reg_tmp, sizeof(PAC193XReadRegister));
			if(status != HAL_OK) {
				loge("Error during PAC193X_STATE_READ_RX receive: %d\n\r", status);
			} else {
				tng_energy_monitor.pac193x_state = PAC193X_STATE_READ_RX_WAIT;
			}
			break;
		}

		case PAC193X_STATE_READ_RX_WAIT: {
			HAL_I2C_StateTypeDef state = HAL_I2C_GetState(&tng_energy_monitor.pac193x_i2c);
			if(state == HAL_I2C_STATE_READY) {
				pac193x_update_values(&read_reg_tmp);
				tng_energy_monitor.pac193x_state = PAC193X_STATE_SLEEP;
				wait_start_time = system_timer_get_ms();
			}
			break;
		}

		case PAC193X_STATE_SLEEP: {
			// We configure to 64 samples/s, 
			// which would be 15.625ms sleep time between measurements, we round that up to 16.
			if((wait_start_time == 0) || system_timer_is_time_elapsed_ms(wait_start_time, 200))  { 
				wait_start_time = 0;
				tng_energy_monitor.pac193x_state = PAC193X_STATE_REFRESH;
			}
			break;
		}

		default: {
			tng_energy_monitor.pac193x_state = PAC193X_STATE_REFRESH;
			loge("Unknown state: %d\n\r", tng_energy_monitor.pac193x_state);
			break;
		}
	}
	}while(tng_energy_monitor.pac193x_state != PAC193X_STATE_SLEEP);

#if 0
	uint8_t refresh = 0;
	pac193x_write_register(PAC193X_REG_REFRESH, 1, &refresh);

	pac193x_read_register(0x01, sizeof(PAC193XReadRegister), (uint8_t *)&tng_energy_monitor.pac193x_read_register);

	uint32_t voltage[3] = {0, 0, 0}; // register 0x7-0x9
	for(uint8_t ch = 0; ch < 3; ch++) {
		voltage[ch] = (tng_energy_monitor.pac193x_read_register.vbus[ch][0] << 8) | tng_energy_monitor.pac193x_read_register.vbus[ch][1];
		voltage[ch] = voltage[ch] * 32000/0xFFFF;
	}
	logd("VBus 0: %dmV, VBus 1: %dmV, VBus 2: %dmV\n\r", voltage[0], voltage[1], voltage[2]);

	uint32_t current[3] = {0, 0, 0};  // register 0xb-0xd
	for(uint8_t ch = 0; ch < 3; ch++) {
		current[ch] = (tng_energy_monitor.pac193x_read_register.vsense[ch][0] << 8) | tng_energy_monitor.pac193x_read_register.vsense[ch][1];
		current[ch] = current[ch] * 5000/0xFFFF;
	}
	logd("VSense 0: %dmA, VSense 1: %dmA, VSense 2: %dmA\n\r", current[0], current[1], current[2]);

	system_timer_sleep_ms(1);
#endif
}

void tng_energy_monitor_init(void) {
	memset(&tng_energy_monitor, 0, sizeof(TNGEnergyMonitor));

	pac193x_init();
}

void tng_energy_monitor_tick(void) {
	pac193x_tick();
}