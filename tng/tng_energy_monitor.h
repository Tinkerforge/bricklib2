/* TNG
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tng_energy_monitor.h: TNG energy monitor driver (PAC1933)
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

#ifndef TNG_ENERGY_MONITOR_H
#define TNG_ENERGY_MONITOR_H

#include "configs/config.h"

#include <stdint.h>

#ifndef PAC193X_CHANNEL_NUM
#define PAC193X_CHANNEL_NUM 3
#endif

typedef enum {
    PAC193X_STATE_REFRESH      = 0,
    PAC193X_STATE_REFRESH_WAIT = 1,
    PAC193X_STATE_READ_TX      = 2,
    PAC193X_STATE_READ_TX_WAIT = 3,
    PAC193X_STATE_READ_RX      = 4,
    PAC193X_STATE_READ_RX_WAIT = 5,
    PAC193X_STATE_SLEEP        = 6
} PAC193XState;

typedef struct {
    uint8_t ctrl;
    uint8_t acc_count[3];
    uint8_t vpower_acc[PAC193X_CHANNEL_NUM][6];
    uint8_t vbus[PAC193X_CHANNEL_NUM][2];
    uint8_t vsense[PAC193X_CHANNEL_NUM][2];
    uint8_t vbus_avg[PAC193X_CHANNEL_NUM][2];
    uint8_t vsense_avg[PAC193X_CHANNEL_NUM][2];
    uint8_t vpower[PAC193X_CHANNEL_NUM][2];
} __attribute__((__packed__)) PAC193XReadRegister;

typedef struct {
    PAC193XReadRegister pac193x_read_register;
    PAC193XState pac193x_state;

    uint32_t voltage[PAC193X_CHANNEL_NUM];
    uint32_t current[PAC193X_CHANNEL_NUM];
    uint32_t power[PAC193X_CHANNEL_NUM];
    uint64_t energy[PAC193X_CHANNEL_NUM];

    I2C_HandleTypeDef pac193x_i2c;
} TNGEnergyMonitor;

extern TNGEnergyMonitor tng_energy_monitor;

void tng_energy_monitor_init(void);
void tng_energy_monitor_tick(void);

#define PAC193X_REG_REFRESH 0x00
#define PAC193X_REG_CTRL    0x01

#endif