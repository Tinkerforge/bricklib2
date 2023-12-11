/* evse-v2-bricklet
 * Copyright (C) 2023 Olaf Lüke <olaf@tinkerforge.com>
 *
 * meter.h: Modbus meter driver (Meter630, SDM72DM-V2, DSZ15DZMOD)
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

#ifndef METER_H
#define METER_H

#include <stdint.h>
#include <stdbool.h>

#define METER_PHASE_NUM 3

// TODO: find out actual register num
#define METER_SDM630_REGISTER_NUM 85
#define METER_SDM72V2_REGISTER_NUM 70
#define METER_DSZ15DZMOD_REGISTER_NUM 70

#define METER_ELTAKO_FAST_REGISTER_NUM 6
#define METER_SDM_FAST_REGISTER_NUM 5

#define METER_SDM_HOLDING_REG_SYSTEM_TYPE 11    // 40011
#define METER_SDM_HOLDING_REG_SYSTEM_KPPA 15    // 40015
#define METER_SDM_HOLDING_REG_PASSWORD    25    // 40025
#define METER_SDM_HOLDING_REG_METER_CODE  64515 // 464515

#define METER_SDM_SYSTEM_TYPE_1P2W        1.0f
#define METER_SDM_SYSTEM_TYPE_3P43        2.0f
#define METER_SDM_SYSTEM_TYPE_3P4W        3.0f

#define METER_SDM_PASSWORD                1000.0f

typedef enum {
	METER_TYPE_UNKNOWN     = 0,
	METER_TYPE_UNSUPPORTED = 1,
	METER_TYPE_SDM630      = 2, // Eastron
	METER_TYPE_SDM72V2     = 3, // Eastron
	METER_TYPE_SDM72CTM    = 4, // Eastron
	METER_TYPE_SDM630MCTV2 = 5, // Eastron
	METER_TYPE_DSZ15DZMOD  = 6, // Eltako
	METER_TYPE_DEM4A       = 7  // YTL
} MeterType;

typedef enum {
	METER_REGISTER_DATA_TYPE_FLOAT = 0,
	METER_REGISTER_DATA_TYPE_INT32 = 1,
	METER_REGISTER_DATA_TYPE_INT16 = 2
} MeterRegisterDataType;

typedef union {
	float f;
	uint32_t data;
	int32_t i32;
	int16_t i16;
	uint16_t u16[2];
} MeterRegisterType;

typedef struct {
    uint32_t register_address;
    MeterRegisterType *register_set_address;
    float scale_factor;
    MeterRegisterDataType register_data_type;
	bool fast_read;
//    uint8_t register_length;
//    float offset;
} MeterDefinition;

typedef struct {
    MeterType type;
    uint16_t slave_address;

	uint8_t state;
	uint16_t register_full_position;
	uint16_t register_fast_position;

	uint32_t register_fast_time;

	bool available;
	bool reset_energy_meter;
	bool each_value_read_once;

	uint32_t timeout;
	uint32_t first_tick;
	uint32_t error_wait_time;

	MeterRegisterType system_type_write;
	MeterRegisterType system_type_read;
	bool new_system_type;

	bool phases_connected[3];

	// The relative values saved as last absolut value
	MeterRegisterType relative_energy_sum;
	MeterRegisterType relative_energy_import;
	MeterRegisterType relative_energy_export;

	const MeterDefinition *current_meter;
} Meter;

typedef struct {
	MeterRegisterType line_to_neutral_volts[METER_PHASE_NUM];
	MeterRegisterType current[METER_PHASE_NUM];
	MeterRegisterType power[METER_PHASE_NUM];
	MeterRegisterType volt_amps[METER_PHASE_NUM];
	MeterRegisterType volt_amps_reactive[METER_PHASE_NUM];
	MeterRegisterType power_factor[METER_PHASE_NUM];
	MeterRegisterType phase_angle[METER_PHASE_NUM];
	MeterRegisterType average_line_to_neutral_volts;
	MeterRegisterType average_line_current;
	MeterRegisterType sum_of_line_currents;
	MeterRegisterType total_system_power;
	MeterRegisterType total_system_volt_amps;
	MeterRegisterType total_system_var;
	MeterRegisterType total_system_power_factor;
	MeterRegisterType total_system_phase_angle;
	MeterRegisterType frequency_of_supply_voltages;
	MeterRegisterType total_import_kwh;
	MeterRegisterType total_export_kwh;
	MeterRegisterType total_import_kvarh;
	MeterRegisterType total_export_kvarh;
	MeterRegisterType total_vah;
	MeterRegisterType ah;
	MeterRegisterType total_system_power_demand;
	MeterRegisterType maximum_total_system_power_demand;
	MeterRegisterType total_system_va_demand;
	MeterRegisterType maximum_total_system_va_demand;
	MeterRegisterType neutral_current_demand;
	MeterRegisterType maximum_neutral_current_demand;
	MeterRegisterType line1_to_line2_volts;
	MeterRegisterType line2_to_line3_volts;
	MeterRegisterType line3_to_line1_volts;
	MeterRegisterType average_line_to_line_volts;
	MeterRegisterType neutral_current;
	MeterRegisterType ln_volts_thd[METER_PHASE_NUM];
	MeterRegisterType current_thd[METER_PHASE_NUM];
	MeterRegisterType average_line_to_neutral_volts_thd;
	MeterRegisterType average_line_to_current_thd;
	MeterRegisterType current_demand[METER_PHASE_NUM];
	MeterRegisterType maximum_current_demand[METER_PHASE_NUM];
	MeterRegisterType line1_to_line2_volts_thd;
	MeterRegisterType line2_to_line3_volts_thd;
	MeterRegisterType line3_to_line1_volts_thd;
	MeterRegisterType average_line_to_line_volts_thd;
	MeterRegisterType total_kwh_sum;
	MeterRegisterType total_kvarh_sum;
	MeterRegisterType import_kwh[METER_PHASE_NUM];
	MeterRegisterType export_kwh[METER_PHASE_NUM];
	MeterRegisterType total_kwh[METER_PHASE_NUM];
	MeterRegisterType import_kvarh[METER_PHASE_NUM];
	MeterRegisterType export_kvarh[METER_PHASE_NUM];
	MeterRegisterType total_kvarh[METER_PHASE_NUM];
	MeterRegisterType relative_total_kwh_sum;
	MeterRegisterType relative_total_import_kwh;
	MeterRegisterType relative_total_export_kwh;
} __attribute__((__packed__)) MeterRegisterSet;


extern Meter meter;
extern MeterRegisterSet meter_register_set;

void meter_init(void);
void meter_tick(void);

#endif