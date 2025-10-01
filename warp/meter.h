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

// For BootloaderHandleMessageResponse
#include "bricklib2/bootloader/bootloader.h"

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

#define METER_ELTAKO_HOLDING_REG_MANUFACTURING_CODE  64515 // 464515
#define METER_ELTAKO_HOLDING_REG_METER_CODE          64525 // 464525
#define METER_ELTAKO_HOLDING_REG_REVERSE_DIRECTION   63767 // 464767

#define METER_ISKRA_INPUT_REG_MODEL_NUMBER 2

#define METER_SDM_SYSTEM_TYPE_1P2W        1.0f
#define METER_SDM_SYSTEM_TYPE_3P43        2.0f
#define METER_SDM_SYSTEM_TYPE_3P4W        3.0f

#define METER_SDM_PASSWORD                1000.0f

#define METER_ELTAKO_REGISTER_COUNT       76

typedef enum {
	METER_TYPE_DETECTION     = -1, // Detection ongoing
	METER_TYPE_UNKNOWN       = 0,
	METER_TYPE_UNSUPPORTED   = 1,
	METER_TYPE_SDM630        = 2, // Eastron
	METER_TYPE_SDM72V2       = 3, // Eastron
	METER_TYPE_SDM72CTM      = 4, // Eastron
	METER_TYPE_SDM630MCTV2   = 5, // Eastron
	METER_TYPE_DSZ15DZMOD    = 6, // Eltako
	METER_TYPE_DEM4A         = 7, // YTL
	METER_TYPE_DMED341MID7ER = 8, // Lovato
	METER_TYPE_DSZ16DZE      = 9, // Eltako
	METER_TYPE_WM3M4C        = 10 // Iskra
} MeterType;

typedef enum {
	METER_REGISTER_DATA_TYPE_FLOAT = 0,
	METER_REGISTER_DATA_TYPE_INT32 = 1,
	METER_REGISTER_DATA_TYPE_INT16 = 2,
	METER_REGISTER_DATA_TYPE_T2    = 3, // Iskra types
	METER_REGISTER_DATA_TYPE_T3    = 4,
	METER_REGISTER_DATA_TYPE_T5    = 5,
	METER_REGISTER_DATA_TYPE_T6    = 6,
	METER_REGISTER_DATA_TYPE_T7    = 7,
	METER_REGISTER_DATA_TYPE_T16   = 8,
	METER_REGISTER_DATA_TYPE_T17   = 9

} MeterRegisterDataType;

typedef union {
	float f;
	uint32_t data;
	int32_t i32;
	uint32_t u32;
	int16_t i16_single;
	uint16_t u16_single;
	uint16_t u16[2];
} MeterRegisterType;

typedef struct {
	uint16_t register_address;
	MeterRegisterType *register_set_address;
	float scale_factor;
	MeterRegisterDataType register_data_type;
	bool fast_read;
} __attribute__((packed)) MeterDefinition;

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

	bool new_fast_value_callback;

	// The relative values saved as last absolut value
	MeterRegisterType relative_energy_sum;
	MeterRegisterType relative_energy_import;
	MeterRegisterType relative_energy_export;

	const MeterDefinition *current_meter;
	uint8_t current_meter_size;
	uint8_t current_meter_index;
	uint8_t current_meter_definition_size;
} Meter;

// Use the same names as in esp32 firmware for easier comparison
typedef struct {
	MeterRegisterType VoltageL1N; // line_to_neutral_volts[0]
	MeterRegisterType VoltageL2N; // line_to_neutral_volts[1]
	MeterRegisterType VoltageL3N; // line_to_neutral_volts[2]
	MeterRegisterType CurrentL1ImExSum; // current[0]
	MeterRegisterType CurrentL2ImExSum; // current[1]
	MeterRegisterType CurrentL3ImExSum; // current[2]
	MeterRegisterType PowerActiveL1ImExDiff; // power[0]
	MeterRegisterType PowerActiveL2ImExDiff; // power[1]
	MeterRegisterType PowerActiveL3ImExDiff; // power[2]
	MeterRegisterType PowerApparentL1ImExSum; // volt_amps[0]
	MeterRegisterType PowerApparentL2ImExSum; // volt_amps[1]
	MeterRegisterType PowerApparentL3ImExSum; // volt_amps[2]
	MeterRegisterType PowerReactiveL1IndCapDiff; // volt_amps_reactive[0]
	MeterRegisterType PowerReactiveL1IndCapSum;
	MeterRegisterType PowerReactiveL2IndCapDiff; // volt_amps_reactive[1]
	MeterRegisterType PowerReactiveL2IndCapSum;
	MeterRegisterType PowerReactiveL3IndCapDiff; // volt_amps_reactive[2]
	MeterRegisterType PowerReactiveL3IndCapSum;
	MeterRegisterType PowerFactorL1Directional; // power_factor[0]
	MeterRegisterType PowerFactorL2Directional; // power_factor[1]
	MeterRegisterType PowerFactorL3Directional; // power_factor[2]
	MeterRegisterType PhaseAngleL1; // phase_angle[0]
	MeterRegisterType PhaseAngleL2; // phase_angle[1]
	MeterRegisterType PhaseAngleL3; // phase_angle[2]
	MeterRegisterType VoltageLNAvg; // average_line_to_neutral_volts
	MeterRegisterType CurrentLAvgImExSum; // average_line_current
	MeterRegisterType CurrentLSumImExSum; // sum_of_line_currents
	MeterRegisterType PowerActiveLSumImExDiff; // total_system_power
	MeterRegisterType PowerApparentLSumImExSum; // total_system_volt_amps
	MeterRegisterType PowerReactiveLSumIndCapDiff; // total_system_var
	MeterRegisterType PowerReactiveLSumIndCapSum;
	MeterRegisterType PowerFactorLSumDirectional; // total_system_power_factor
	MeterRegisterType PhaseAngleLSum; // total_system_phase_angle
	MeterRegisterType FrequencyLAvg; // frequency_of_supply_voltages
	MeterRegisterType EnergyActiveLSumImport; // total_import_kwh
	MeterRegisterType EnergyActiveLSumExport; // total_export_kwh
	MeterRegisterType EnergyReactiveLSumInductive; // total_import_kvarh
	MeterRegisterType EnergyReactiveLSumCapacitive; // total_export_kvarh
	MeterRegisterType EnergyApparentLSumImExSum; // total_vah
	MeterRegisterType ElectricCharge; // ah
	MeterRegisterType VoltageL1L2; // line1_to_line2_volts
	MeterRegisterType VoltageL2L3; // line2_to_line3_volts
	MeterRegisterType VoltageL3L1; // line3_to_line1_volts
	MeterRegisterType VoltageLLAvg; // average_line_to_line_volts
	MeterRegisterType CurrentNImExSum; // neutral_current
	MeterRegisterType VoltageTHDL1N; // ln_volts_thd[0]
	MeterRegisterType VoltageTHDL2N; // ln_volts_thd[1]
	MeterRegisterType VoltageTHDL3N; // ln_volts_thd[2]
	MeterRegisterType CurrentTHDL1; // current_thd[0]
	MeterRegisterType CurrentTHDL2; // current_thd[1]
	MeterRegisterType CurrentTHDL3; // current_thd[2]
	MeterRegisterType VoltageTHDLNAvg; // average_line_to_neutral_volts_thd
	MeterRegisterType CurrentTHDLAvg; // average_line_to_current_thd
	MeterRegisterType VoltageTHDL1L2; // line1_to_line2_volts_thd
	MeterRegisterType VoltageTHDL2L3; // line2_to_line3_volts_thd
	MeterRegisterType VoltageTHDL3L1; // line3_to_line1_volts_thd
	MeterRegisterType VoltageTHDLLAvg; // average_line_to_line_volts_thd
	MeterRegisterType EnergyActiveLSumImExSum; // total_kwh_sum
	MeterRegisterType EnergyReactiveLSumIndCapSum; // total_kvarh_sum
	MeterRegisterType EnergyActiveL1Import; // import_kwh[0]
	MeterRegisterType EnergyActiveL2Import; // import_kwh[1]
	MeterRegisterType EnergyActiveL3Import; // import_kwh[2]
	MeterRegisterType EnergyActiveL1Export; // export_kwh[0]
	MeterRegisterType EnergyActiveL2Export; // export_kwh[1]
	MeterRegisterType EnergyActiveL3Export; // export_kwh[2]
	MeterRegisterType EnergyActiveL1ImExSum; // total_kwh[0]
	MeterRegisterType EnergyActiveL2ImExSum; // total_kwh[1]
	MeterRegisterType EnergyActiveL3ImExSum; // total_kwh[2]
	MeterRegisterType EnergyReactiveL1Inductive; // import_kvarh[0]
	MeterRegisterType EnergyReactiveL2Inductive; // import_kvarh[1]
	MeterRegisterType EnergyReactiveL3Inductive; // import_kvarh[2]
	MeterRegisterType EnergyReactiveL1Capacitive; // export_kvarh[0]
	MeterRegisterType EnergyReactiveL2Capacitive; // export_kvarh[1]
	MeterRegisterType EnergyReactiveL3Capacitive; // export_kvarh[2]
	MeterRegisterType EnergyReactiveL1IndCapSum; // total_kvarh[0]
	MeterRegisterType EnergyReactiveL2IndCapSum; // total_kvarh[1]
	MeterRegisterType EnergyReactiveL3IndCapSum; // total_kvarh[2]
	MeterRegisterType EnergyActiveLSumImExSumResettable; // relative_total_kwh_sum
	MeterRegisterType EnergyActiveLSumImportResettable; // relative_total_import_kwh
	MeterRegisterType EnergyActiveLSumExportResettable; // relative_total_export_kwh
	MeterRegisterType Temperature;
	MeterRegisterType RunTime;
} MeterRegisterSet;


extern Meter meter;
extern MeterRegisterSet meter_register_set;

void meter_init(void);
void meter_tick(void);

void meter_read_registers(uint8_t fc, uint8_t slave_address, uint16_t starting_address, uint16_t count);
void meter_write_register(uint8_t fc, uint8_t slave_address, uint16_t starting_address, MeterRegisterType *payload);
bool meter_get_read_registers_response(uint8_t fc, void *data, uint8_t count);
bool meter_get_write_register_response(uint8_t fc);
void meter_write_string(uint8_t slave_address, uint16_t starting_address, char *payload, uint8_t payload_count);
bool meter_get_read_registers_response_string(uint8_t fc, char *data, uint8_t count);
void meter_handle_phases_connected(void);
void meter_handle_register_set_read_done(void);
void meter_handle_register_set_fast_read_done(void);
void meter_handle_new_data(MeterRegisterType data, const MeterDefinition *definition);
uint8_t meter_get_register_size(uint16_t position);
float meter_get_next_value(void);
bool meter_supports_eichrecht(void);

typedef struct {
	TFPMessageHeader header;
	uint16_t values_length;
	uint16_t values_chunk_offset;
	float values_chunk_data[15];
} __attribute__((__packed__)) GenericMeterValues_Response;
BootloaderHandleMessageResponse meter_fill_communication_values(GenericMeterValues_Response *response);

#endif