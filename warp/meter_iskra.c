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

#ifdef HAS_HARDWARE_VERSION
#include "hardware_version.h"
#endif

MeterIskra meter_iskra;

MeterType meter_iskra_is_connected(void) {
	static uint8_t find_meter_state = 0;

	switch(find_meter_state) {
		case 0: { // Check for wm3m4c
			XMC_USIC_CH_SetBaudrate(RS485_USIC, 115200, RS485_OVERSAMPLING);

			// Read model number register with slave address 0x21 (Iskra)
			meter_read_registers(MODBUS_FC_READ_INPUT_REGISTERS, 0x21, METER_ISKRA_INPUT_REG_MODEL_NUMBER, 2);
			find_meter_state++;
			return METER_TYPE_DETECTION;
		}


		case 1: { // Check for wm3m4c
			uint32_t model_number = 0xFFFFFFFF;
			bool ret = meter_get_read_registers_response(MODBUS_FC_READ_INPUT_REGISTERS, &model_number, 2);
			if(ret) {
				find_meter_state = 0;
				modbus_clear_request(&rs485);
				switch(model_number) {
					case ('W' << 24) | ('M' << 16) | ('3' << 8) | 'M': return METER_TYPE_WM3M4C;
					default: return METER_TYPE_UNKNOWN;
				}
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

void meter_iskra_handle_register_set_read_done() {
	// TODO: It is currently unclear how this works with the exponent and x1000.
	//       We need to figure this out with real-world measurements.
	meter_register_set.total_import_kwh.f = meter_iskra.energy_counter[0].f;
	meter_register_set.total_export_kwh.f = meter_iskra.energy_counter[1].f;
	meter_register_set.total_kwh_sum.f    = meter_register_set.total_import_kwh.f - meter_register_set.total_export_kwh.f;

	meter_handle_register_set_read_done();
	meter_handle_register_set_fast_read_done();
}

// The Iskra meter uses a baudrate of 115200, so we don't need the "fast-read" mechanic.
// TODO: Measure read-time and read in blocks similar to Eltako if necessary
void meter_iskra_tick(void) {
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
			meter.state = 0;
			break;
		}

		default: {
			meter.state = 0;
		}
	}
}