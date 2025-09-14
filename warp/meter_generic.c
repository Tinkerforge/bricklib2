/* evse-v2-bricklet
 * Copyright (C) 2025 Olaf Lüke <olaf@tinkerforge.com>
 *
 * meter_generic.c: Modbus meter driver for generic meters
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

#include "meter_generic.h"

#include "meter.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/logging/logging.h"

#include "rs485.h"
#include "modbus.h"

#ifdef HAS_HARDWARE_VERSION
#include "hardware_version.h"
#endif

MeterType meter_generic_is_connected(void) {
	static uint8_t find_meter_state = 0;

	switch(find_meter_state) {
		case 0: {
			XMC_USIC_CH_SetBaudrate(RS485_USIC, 9600, RS485_OVERSAMPLING);

			// Read meter code for YTL meters register with slave address 1
			meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, 1, 0x100D, 1);
			find_meter_state++;

			return METER_TYPE_DETECTION;
		}

		case 1: {
			uint16_t meter_code = 0xFFFF;
			bool ret = meter_get_read_registers_response(MODBUS_FC_READ_HOLDING_REGISTERS, &meter_code, 1);
			if(ret) {
				find_meter_state = 0;
				modbus_clear_request(&rs485);
				switch(meter_code) {
					case 0x0006: return METER_TYPE_DEM4A;
					default:     return METER_TYPE_UNKNOWN;
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

void meter_generic_tick(void) {
	static bool read_fast = false;

	switch(meter.state) {
		case 0: { // request
			if(system_timer_is_time_elapsed_ms(meter.register_fast_time, 500) || read_fast) {
				meter_read_registers(MODBUS_FC_READ_INPUT_REGISTERS, meter.slave_address, meter.current_meter[meter.register_fast_position].register_address, meter_get_register_size(meter.register_fast_position));
				read_fast = true;
			} else {
				meter_read_registers(MODBUS_FC_READ_INPUT_REGISTERS, meter.slave_address, meter.current_meter[meter.register_full_position].register_address, meter_get_register_size(meter.register_full_position));
				read_fast = false;
			}
			meter.state++;
			break;
		}

		case 1: { // read
			bool ret = false;
			MeterRegisterType data;
			ret = meter_get_read_registers_response(MODBUS_FC_READ_INPUT_REGISTERS, &data, meter_get_register_size(read_fast ? meter.register_fast_position : meter.register_full_position));
			if(ret) {
				meter_handle_new_data(data, read_fast ? &meter.current_meter[meter.register_fast_position] : &meter.current_meter[meter.register_full_position]);
				modbus_clear_request(&rs485);
				if(read_fast) {
					meter.state = 0;
					do {
						meter.register_fast_position++;
						if(meter.current_meter[meter.register_fast_position].register_set_address == NULL) {
							meter.register_fast_position = 0;
							meter.register_fast_time += 500;
							if(system_timer_is_time_elapsed_ms(meter.register_fast_time, 500)) {
								meter.register_fast_time = system_timer_get_ms();
							}
							// We read all fast registers once, go back to full read.
							// Fast read will start again after 500ms
							read_fast = false;
							meter_handle_register_set_fast_read_done();
						}
					} while(!meter.current_meter[meter.register_fast_position].fast_read);
				} else {
					meter.state++;
					do {
						meter.register_full_position++;
						if(meter.current_meter[meter.register_full_position].register_set_address == NULL) {
							meter.register_full_position = 0;
							meter_handle_register_set_read_done();
						}
					} while(meter.current_meter[meter.register_full_position].fast_read);
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