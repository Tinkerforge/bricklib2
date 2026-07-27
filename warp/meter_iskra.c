/* evse-v2-bricklet
 * Copyright (C) 2025 Olaf Lüke <olaf@tinkerforge.com>
 *
 * meter_iskra.c: Modbus meter driver for Iskra meters
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

#include "meter_iskra.h"

#include "meter.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/logging/logging.h"

#include "rs485.h"
#include "modbus.h"

#if defined(HAS_HARDWARE_VERSION) && defined(IS_CHARGER)
#include "hardware_version.h"
#include "eichrecht.h"
#include "iskra_display.h"
#endif

MeterIskra meter_iskra;

#define METER_ISKRA_FAST_READ_ENABLED 1

// Fast-read of the R37-critical registers. Two contiguous Modbus blocks cover
// all seven values:
//   - FrequencyLAvg (106) + VoltageL1N/L2N/L3N (108..113) -> registers 106..113
//   - CurrentL1/L2/L3 ImExSum (127..132)                  -> registers 127..132
#define METER_ISKRA_FAST_INTERVAL_MS  100
#define METER_ISKRA_FAST_REG_VF_START 106
#define METER_ISKRA_FAST_REG_VF_COUNT 8
#define METER_ISKRA_FAST_REG_I_START  127
#define METER_ISKRA_FAST_REG_I_COUNT  6

#if METER_ISKRA_FAST_READ_ENABLED
static const MeterDefinition meter_wm3m4c_fast[] = {
	{106, &meter_register_set.FrequencyLAvg,    1.0, METER_REGISTER_DATA_TYPE_T5, false},
	{108, &meter_register_set.VoltageL1N,       1.0, METER_REGISTER_DATA_TYPE_T5, false},
	{110, &meter_register_set.VoltageL2N,       1.0, METER_REGISTER_DATA_TYPE_T5, false},
	{112, &meter_register_set.VoltageL3N,       1.0, METER_REGISTER_DATA_TYPE_T5, false},
	{127, &meter_register_set.CurrentL1ImExSum, 1.0, METER_REGISTER_DATA_TYPE_T5, false},
	{129, &meter_register_set.CurrentL2ImExSum, 1.0, METER_REGISTER_DATA_TYPE_T5, false},
	{131, &meter_register_set.CurrentL3ImExSum, 1.0, METER_REGISTER_DATA_TYPE_T5, false},
};

static bool meter_iskra_response_had_error(void) {
	return rs485.modbus_rtu.request.master_request_timed_out || (rs485.modbus_rtu.request.rx_frame[1] == (rs485.modbus_rtu.request.tx_frame[1] + 0x80));
}

static void meter_iskra_fast_tick(void) {
	switch(meter_iskra.fast_state) {
		case 0: { // request frequency + 3 voltages
			meter_iskra.fast_active     = true;
			meter_iskra.fast_start_time = system_timer_get_ms();
			meter_read_registers(MODBUS_FC_READ_INPUT_REGISTERS, meter.slave_address, METER_ISKRA_FAST_REG_VF_START, METER_ISKRA_FAST_REG_VF_COUNT);
			meter_iskra.fast_state++;
			break;
		}

		case 1: { // read frequency + 3 voltages
			MeterRegisterType data[METER_ISKRA_FAST_REG_VF_COUNT / 2];
			if(meter_get_read_registers_response(MODBUS_FC_READ_INPUT_REGISTERS, &data, METER_ISKRA_FAST_REG_VF_COUNT)) {
				if(!meter_iskra_response_had_error()) {
					meter_handle_new_data(data[0], &meter_wm3m4c_fast[0]); // FrequencyLAvg
					meter_handle_new_data(data[1], &meter_wm3m4c_fast[1]); // VoltageL1N
					meter_handle_new_data(data[2], &meter_wm3m4c_fast[2]); // VoltageL2N
					meter_handle_new_data(data[3], &meter_wm3m4c_fast[3]); // VoltageL3N
					meter_handle_phases_connected();
					meter.register_fast_time = system_timer_get_ms();
				}
				modbus_clear_request(&rs485);
				meter_iskra.fast_state++;
			}
			break;
		}

		case 2: { // request 3 currents
			meter_read_registers(MODBUS_FC_READ_INPUT_REGISTERS, meter.slave_address, METER_ISKRA_FAST_REG_I_START, METER_ISKRA_FAST_REG_I_COUNT);
			meter_iskra.fast_state++;
			break;
		}

		case 3: { // read 3 currents
			MeterRegisterType data[METER_ISKRA_FAST_REG_I_COUNT / 2];
			if(meter_get_read_registers_response(MODBUS_FC_READ_INPUT_REGISTERS, &data, METER_ISKRA_FAST_REG_I_COUNT)) {
				if(!meter_iskra_response_had_error()) {
					meter_handle_new_data(data[0], &meter_wm3m4c_fast[4]); // CurrentL1ImExSum
					meter_handle_new_data(data[1], &meter_wm3m4c_fast[5]); // CurrentL2ImExSum
					meter_handle_new_data(data[2], &meter_wm3m4c_fast[6]); // CurrentL3ImExSum
				}
				modbus_clear_request(&rs485);
				meter_iskra.fast_state  = 0;
				meter_iskra.fast_active = false;
			}
			break;
		}

		default: {
			meter_iskra.fast_state  = 0;
			meter_iskra.fast_active = false;
			break;
		}
	}
}
#endif

MeterType meter_iskra_is_connected(void) {
	static uint8_t find_meter_state = 0;

	switch(find_meter_state) {
		case 0: { // Check for wm3m4c
			XMC_USIC_CH_SetBaudrate(RS485_USIC, 115200, RS485_OVERSAMPLING);

			// Read model number register with slave address 0x21 (Iskra)
			meter_read_registers(MODBUS_FC_READ_INPUT_REGISTERS, 0x21, METER_ISKRA_INPUT_REG_MODEL_NUMBER, 4);
			find_meter_state++;
			return METER_TYPE_DETECTION;
		}


		case 1: { // Check for wm3m4c
			uint32_t model_number[2] = {0xFFFFFFFF, 0xFFFFFFFF};
			bool ret = meter_get_read_registers_response(MODBUS_FC_READ_INPUT_REGISTERS, &model_number, 4);
			if(ret) {
				find_meter_state = 0;
				modbus_clear_request(&rs485);
				if(model_number[0] == (('W' << 24) | ('M' << 16) | ('3' << 8) | 'M')) {
					if((model_number[1] & 0xFFFF0000) == (('4' << 24) | ('C' << 16))) {
						return METER_TYPE_WM3M4C;
					} else if((model_number[1] & 0xFF000000) == ('4' << 24)) {
						return METER_TYPE_WM3M4;
					}
				}

				return METER_TYPE_UNKNOWN;
			}
			return METER_TYPE_DETECTION;
		}

		default: {
			find_meter_state = 0;
			break;
		}
	}

	return METER_TYPE_UNKNOWN;
}

void meter_iskra_handle_register_set_read_done(void) {
	// TODO: It is currently unclear how this works with the exponent and x1000.
	//       We need to figure this out with real-world measurements.
	meter_register_set.EnergyActiveLSumImport.f  = meter_iskra.energy_counter[0].f/1000.0f;
	meter_register_set.EnergyActiveLSumExport.f  = meter_iskra.energy_counter[1].f/1000.0f;
	meter_register_set.EnergyActiveLSumImExSum.f = meter_register_set.EnergyActiveLSumImport.f - meter_register_set.EnergyActiveLSumExport.f;

	meter_handle_register_set_read_done();
	meter_handle_register_set_fast_read_done();
}

// The full register set is read one register at a time in a round-robin (the
// switch below). On top of that, the R37-critical registers (3 phase voltages
// + frequency + 3 phase currents) are read in two contiguous blocks at ~10 Hz,
// interleaved with the full cycle, so the OVE R37 grid-support checks get fresh
// values fast enough (see meter_iskra_fast_tick()).
void meter_iskra_tick(void) {
#if defined(HAS_HARDWARE_VERSION) && defined(IS_CHARGER)
	if(hardware_version.is_v4) {
		// Initialize Eichrecht before initial meter value reading,
		// so we can be sure that the public key is read before we
		// announce the meter type to ESP.
		if(!eichrecht.init_done) {
			eichrecht_iskra_init_tick();
			return;
		}
		// Check if eichrecht transaction is ongoing and pause meter reading if so
		if((meter.state == 0) && (eichrecht.transaction_state > 0)) {
			eichrecht_iskra_tick();
			return;
		}
	}

	if((meter.state == 0) && iskra_display_has_work()) {
		iskra_display_modbus_tick();
		return;
	}
#endif

	// Interleave the high-priority fast read of the R37-critical registers.
#if METER_ISKRA_FAST_READ_ENABLED
	if(meter_iskra.fast_active || ((meter.state == 0) && system_timer_is_time_elapsed_ms(meter_iskra.fast_start_time, METER_ISKRA_FAST_INTERVAL_MS))) {
		meter_iskra_fast_tick();
		return;
	}
#endif

	switch(meter.state) {
		case 0: { // request
			meter_read_registers(MODBUS_FC_READ_INPUT_REGISTERS, meter.slave_address, meter.current_meter[meter.register_full_position].register_address, meter_get_register_size(meter.register_full_position));
			meter.state++;
			break;
		}

		case 1: { // read
			bool ret = false;
			MeterRegisterType data;
			ret = meter_get_read_registers_response(MODBUS_FC_READ_INPUT_REGISTERS, &data, meter_get_register_size(meter.register_full_position));
			if(ret) {
				meter_handle_new_data(data, &meter.current_meter[meter.register_full_position]);
				modbus_clear_request(&rs485);
				meter.state++;
				meter.register_full_position++;
				if(meter.current_meter[meter.register_full_position].register_set_address == NULL) {
					meter.register_full_position = 0;
					meter_iskra_handle_register_set_read_done();
				}
			}
			break;
		}

		case 2: {
			if((meter.register_full_position != 0) || !meter.each_value_read_once) {
				meter.state = 0;
				break;
			}

			meter_handle_phases_connected();
			meter.state++;
			break;
		}

		// Once every full read cycle also read the important Eichrecht status registers
		// We read this even if the Eichrecht functionality is not used,
		// it doesn't hurt to get the status information anyway.
		case 3: { // request measurement status from holding register
			meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, meter.slave_address, 7000+1, 1);
			meter.state++;
			break;
		}

		case 4: { // read measurement status from holding register
			bool ret = meter_get_read_registers_response(MODBUS_FC_READ_HOLDING_REGISTERS, &meter_iskra.measurement_status, 1);
			if(ret) {
				modbus_clear_request(&rs485);
				meter.state++;
			}
			break;
		}

		case 5: { // request signature status from holding register
			meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, meter.slave_address, 7052+1, 1);
			meter.state++;
			break;
		}

		case 6: { // read signature status from holding register
			bool ret = meter_get_read_registers_response(MODBUS_FC_READ_HOLDING_REGISTERS, &meter_iskra.signature_status, 1);
			if(ret) {
				modbus_clear_request(&rs485);
				meter.state++;
			}
			break;
		}

		default: {
			meter.state = 0;
		}
	}
}
