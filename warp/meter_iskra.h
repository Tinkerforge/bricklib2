/* evse-v2-bricklet
 * Copyright (C) 2025 Olaf Lüke <olaf@tinkerforge.com>
 *
 * meter_iskra.h: Modbus meter driver for Iskra meters
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

#ifndef METER_ISKRA_H
#define METER_ISKRA_H

#include "meter.h"

typedef struct {
    MeterRegisterType energy_counter_exponent[2];
    MeterRegisterType energy_counter[2];
    MeterRegisterType energy_counter_1000x[2];

    uint16_t measurement_status;
    uint16_t signature_status;
} MeterIskra;

extern MeterIskra meter_iskra;

MeterType meter_iskra_is_connected(void);
void meter_iskra_tick(void);

#endif