/* evse-v2-bricklet
 * Copyright (C) 2025 Olaf Lüke <olaf@tinkerforge.com>
 *
 * meter_eastron.c: Modbus meter driver for Eastron
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

#include "meter_eastron.h"

#include "meter.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/logging/logging.h"

#include "rs485.h"
#include "modbus.h"

#ifdef HAS_HARDWARE_VERSION
#include "hardware_version.h"
#endif

void meter_eastron_handle_new_system_type(void) {
	meter_handle_phases_connected();
	if(meter.system_type_read.f == METER_SDM_SYSTEM_TYPE_3P4W) {
		// Change system type if 3-phase is configured, but 1-phase is connected
		if(meter.phases_connected[0] && !meter.phases_connected[1] && !meter.phases_connected[2]) {
			meter.new_system_type = true;
		}
	} else if(meter.system_type_read.f == METER_SDM_SYSTEM_TYPE_1P2W) {
		// Change system type if 1-phase is configured, but 3-phase is connected
		if(meter.phases_connected[0] && meter.phases_connected[1] && meter.phases_connected[2]) {
			meter.new_system_type = true;
		}
	} else {
		// Change system type if un-allowed system type is configured
		meter.new_system_type = true;
	}
}

MeterType meter_eastron_is_connected(void) {
	static uint8_t find_meter_state = 0;

	switch(find_meter_state) {
		case 0: {
			XMC_USIC_CH_SetBaudrate(RS485_USIC, 9600, RS485_OVERSAMPLING);

			// Read meter code register with slave address 1 (SDM)
			meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, 1, METER_SDM_HOLDING_REG_METER_CODE, 1);
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
					case 0x0084: return METER_TYPE_UNSUPPORTED; // 0x0084 is SDM72V1 (not supported)
					case 0x0089: return METER_TYPE_SDM72V2;     // Compare datasheet page 16 meter code
					// Some early versions of the SDM630 return 0x0000 instead of 0x0070 for the meter type register.
#ifdef HAS_HARDWARE_VERSION
					case 0x0000: {
						if(!hardware_version.is_v2) {
							return METER_TYPE_UNKNOWN;
						}
						__attribute__((fallthrough));
					}
#else
					case 0x0000: __attribute__((fallthrough)); // fall-through for energy manager
#endif
					case 0x0070: return METER_TYPE_SDM630;
					case 0x0079: return METER_TYPE_SDM630MCTV2;
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

void meter_eastron_tick(void) {
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

		case 2: { // request system type from holding register
			if((meter.register_full_position != 0) || !meter.each_value_read_once) {
				meter.state = 0;
				break;
			}

			// Only SDM meters do not have system type configuration
			if((meter.type == METER_TYPE_DSZ15DZMOD) || (meter.type == METER_TYPE_DEM4A) || (meter.type == METER_TYPE_DMED341MID7ER) || (meter.type == METER_TYPE_DSZ16DZE) || (meter.type == METER_TYPE_WM3M4C)) {
				// For SDM meters this is done during system type check
				meter_handle_phases_connected();
				meter.state = 0;
				break;
			}

			meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, meter.slave_address, METER_SDM_HOLDING_REG_SYSTEM_TYPE, 2);
			meter.state++;
			break;
		}

		case 3: { // read system type from holding register
			bool ret = meter_get_read_registers_response(MODBUS_FC_READ_HOLDING_REGISTERS, &meter.system_type_read, 2);
			if(ret) {
				meter_eastron_handle_new_system_type();
				modbus_clear_request(&rs485);
				meter.state++;
			}
			break;
		}

		case 4: { // write password for phase change (if new phase configuration)
			if(!meter.new_system_type) {
				meter.state = 0;
				break;
			}
			meter.new_system_type = false;

			MeterRegisterType password;
			password.f = METER_SDM_PASSWORD;
			modbus_clear_request(&rs485);
			if(meter.type == METER_TYPE_SDM72V2) {
				// For SDM72V2 the password has to be written to KPPA register
				meter_write_register(MODBUS_FC_WRITE_MULTIPLE_REGISTERS, meter.slave_address, METER_SDM_HOLDING_REG_SYSTEM_KPPA, &password);
			} else {
				meter_write_register(MODBUS_FC_WRITE_MULTIPLE_REGISTERS, meter.slave_address, METER_SDM_HOLDING_REG_PASSWORD, &password);
			}
			meter.state++;
			break;
		}

		case 5: { // check write password response
			bool ret = meter_get_write_register_response(MODBUS_FC_WRITE_MULTIPLE_REGISTERS);
			if(ret) {
				modbus_clear_request(&rs485);
				meter.state++;
			}
			break;
		}

		case 6: { // write new system type
			MeterRegisterType system_type;
			if(meter.phases_connected[0] && !meter.phases_connected[1] && !meter.phases_connected[2]) {
				system_type.f = METER_SDM_SYSTEM_TYPE_1P2W;
			} else if(meter.phases_connected[0] && meter.phases_connected[1] && meter.phases_connected[2]) {
				system_type.f = METER_SDM_SYSTEM_TYPE_3P4W;
			} else {
				meter.state = 0;
				break;
			}

			modbus_clear_request(&rs485);
			meter_write_register(MODBUS_FC_WRITE_MULTIPLE_REGISTERS, meter.slave_address, METER_SDM_HOLDING_REG_SYSTEM_TYPE, &system_type);
			meter.state++;
			break;
		}

		case 7: { // check system type write response
			// This returns illegal function on SDM72V2
			bool ret = meter_get_write_register_response(MODBUS_FC_WRITE_MULTIPLE_REGISTERS);
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