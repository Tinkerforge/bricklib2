/* evse-v2-bricklet
 * Copyright (C) 2023 Olaf Lüke <olaf@tinkerforge.com>
 *
 * meter.c: Modbus meter driver (SDM630, SDM72DM-V2, DSZ15DZMOD)
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

#include "meter.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/utility/crc16.h"
#include "bricklib2/logging/logging.h"

#include "rs485.h"
#include "modbus.h"

#include <math.h>

Meter meter;

MeterFastRegisterSet meter_fast_register_set;
MeterFullRegisterSet meter_full_register_set;

MeterDefinition meter_sdm630_full[] = { 
    #include "meter_sdm630_full_def.inc"
};
MeterDefinition meter_sdm72v2_full[] = {
    #include "meter_sdm72v2_full_def.inc"
};
MeterDefinition meter_dsz15dzmod_full[] = {
    #include "meter_dsz15dzmod_full_def.inc"
};
MeterDefinition meter_sdm630_fast[] = {
    #include "meter_sdm630_fast_def.inc"
};
MeterDefinition meter_sdm72v2_fast[] = {
    #include "meter_sdm72v2_fast_def.inc"
};
MeterDefinition meter_dsz15dzmod_fast[] = {
    #include "meter_dsz15dzmod_fast_def.inc"
};

static void modbus_store_tx_frame_data_bytes(const uint8_t *data, const uint16_t length) {
	for(uint16_t i = 0; i < length; i++) {
		ringbuffer_add(&rs485.ringbuffer_tx, data[i]);
	}
}

static void modbus_store_tx_frame_data_shorts(const uint16_t *data, const uint16_t length) {
	for(uint16_t i = 0; i < length; i++) {
		ringbuffer_add(&rs485.ringbuffer_tx, data[i] >> 8);
		ringbuffer_add(&rs485.ringbuffer_tx, data[i] & 0xFF);
	}
}

static void modbus_add_tx_frame_checksum(void) {
	uint16_t checksum = crc16_modbus(rs485.ringbuffer_tx.buffer, ringbuffer_get_used(&rs485.ringbuffer_tx));

	ringbuffer_add(&rs485.ringbuffer_tx, checksum & 0xFF);
	ringbuffer_add(&rs485.ringbuffer_tx, checksum >> 8);
}

void meter_read_registers(uint8_t fc, uint8_t slave_address, uint16_t starting_address, uint16_t count) {
	modbus_init_new_request(&rs485, MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE, 10);

	// Fix endianness (LE->BE)
	starting_address = HTONS(starting_address-1);
	count = HTONS(count);

	// Constructing the frame in the TX buffer.
	modbus_store_tx_frame_data_bytes(&slave_address, 1); // Slave address.
	modbus_store_tx_frame_data_bytes(&fc, 1); // Function code.
	modbus_store_tx_frame_data_bytes((uint8_t *)&starting_address, 2);
	modbus_store_tx_frame_data_bytes((uint8_t *)&count, 2);

	// Calculate checksum and put it at the end of the TX buffer.
	modbus_add_tx_frame_checksum();

	// Start master request timeout timing.
	rs485.modbus_rtu.request.time_ref_master_request_timeout = system_timer_get_ms();

	// Start TX.
	modbus_start_tx_from_buffer(&rs485);
}

void meter_write_register(uint8_t fc, uint8_t slave_address, uint16_t starting_address, MeterRegisterType *payload) {
	uint16_t count = HTONS(2);
	uint8_t byte_count = 4;

	// Fix endianness (LE->BE).
	starting_address = HTONS(starting_address-1);

	// Constructing the frame in the TX buffer.
	modbus_store_tx_frame_data_bytes(&slave_address, 1); // Slave address.
	modbus_store_tx_frame_data_bytes(&fc, 1); // Function code.
	modbus_store_tx_frame_data_bytes((uint8_t *)&starting_address, 2); // Starting address.
	modbus_store_tx_frame_data_bytes((uint8_t *)&count, 2); // Count.
	modbus_store_tx_frame_data_bytes((uint8_t *)&byte_count, 1); // Byte count.
	modbus_store_tx_frame_data_shorts(&payload->u16[1], 1);
	modbus_store_tx_frame_data_shorts(&payload->u16[0], 1);
	modbus_add_tx_frame_checksum();

	modbus_init_new_request(&rs485, MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE, 13);

	// Start master request timeout timing.
	rs485.modbus_rtu.request.time_ref_master_request_timeout = system_timer_get_ms();

	modbus_start_tx_from_buffer(&rs485);
}

bool meter_get_read_registers_response(uint8_t fc, void *data, uint8_t count) {
	if((rs485.mode != MODE_MODBUS_MASTER_RTU) ||
		(rs485.modbus_rtu.request.state != MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE) ||
		(rs485.modbus_rtu.request.tx_frame[1] != fc) ||
		!rs485.modbus_rtu.request.cb_invoke) {
		return false; // don't increment state
	}

	// Check if the request has timed out.
	if(rs485.modbus_rtu.request.master_request_timed_out) {
		meter.error_wait_time = system_timer_get_ms();
		// Nothing
	} else if(rs485.modbus_rtu.request.rx_frame[1] == rs485.modbus_rtu.request.tx_frame[1] + 0x80) {
		// Check if the slave response is an exception.
		if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_ILLEGAL_FUNCTION) {
			rs485.modbus_common_error_counters.illegal_function++;
		} else if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_ILLEGAL_DATA_ADDRESS) {
			rs485.modbus_common_error_counters.illegal_data_address++;
		} else if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_ILLEGAL_DATA_VALUE) {
			rs485.modbus_common_error_counters.illegal_data_value++;
		} else if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_SLAVE_DEVICE_FAILURE) {
			rs485.modbus_common_error_counters.slave_device_failure++;
		}
	} else {
		// As soon as we were able to read our first package (including correct crc etc)
		// we assume that a SDM is attached to the EVSE.
		meter.available = true;

		uint8_t *d = &rs485.modbus_rtu.request.rx_frame[3];
        if(count == 1) {
		    *((uint16_t*)data) = (d[0] << 8) | (d[1] << 0);
        } else if(count == 2) {
		    *((uint32_t*)data) = (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
        }
	}

	return true; // increment state
}

bool meter_get_write_register_response(uint8_t fc) {
	if((rs485.mode != MODE_MODBUS_MASTER_RTU) ||
	   (rs485.modbus_rtu.request.state != MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE) ||
	   (rs485.modbus_rtu.request.tx_frame[1] != fc) ||
	   !rs485.modbus_rtu.request.cb_invoke) {
		return false; // don't increment state
	}

	// Check if the request has timed out.
	if(rs485.modbus_rtu.request.master_request_timed_out) {
		meter.error_wait_time = system_timer_get_ms();
		// Nothing
	} else if(rs485.modbus_rtu.request.rx_frame[1] == rs485.modbus_rtu.request.tx_frame[1] + 0x80) {
		// Check if the slave response is an exception.

		if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_ILLEGAL_FUNCTION) {
			rs485.modbus_common_error_counters.illegal_function++;
		} else if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_ILLEGAL_DATA_ADDRESS) {
			rs485.modbus_common_error_counters.illegal_data_address++;
		} else if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_ILLEGAL_DATA_VALUE) {
			rs485.modbus_common_error_counters.illegal_data_value++;
		} else if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_SLAVE_DEVICE_FAILURE) {
			rs485.modbus_common_error_counters.slave_device_failure++;
		}
	}
	
	return true; // increment state
}

void meter_init(void) {
	uint32_t relative_energy_save = meter.relative_energy.data;
	memset(&meter, 0, sizeof(Meter));

	// Initialize registers with NaN
	for(uint8_t i = 0; i < (sizeof(meter_full_register_set)/sizeof(uint32_t)); i++) {
		((float*)&meter_full_register_set)[i] = NAN;
	}
	for(uint8_t i = 0; i < (sizeof(meter_fast_register_set)/sizeof(uint32_t)); i++) {
		((float*)&meter_fast_register_set)[i] = NAN;
	}

	meter.relative_energy.data = relative_energy_save;
	meter.first_tick = system_timer_get_ms();
	meter.register_fast_time = system_timer_get_ms();
}

// Update phases connected bool array (this is used in communication.c)
void meter_handle_phases_connected(void) {
	meter.phases_connected[0] = meter_full_register_set.line_to_neutral_volts[0].f > 180.0f;
	meter.phases_connected[1] = meter_full_register_set.line_to_neutral_volts[1].f > 180.0f;
	meter.phases_connected[2] = meter_full_register_set.line_to_neutral_volts[2].f > 180.0f;
}

void meter_handle_new_system_type(void) {
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

void meter_set_meter_type(MeterType type) {
    meter.type = type;
    switch(type) {
        case METER_TYPE_SDM72V2:    meter.slave_address = 0x01; meter.current_meter_full = &meter_sdm72v2_full[0];    meter.current_meter_fast = &meter_sdm72v2_fast[0];    break;
        case METER_TYPE_SDM630:     meter.slave_address = 0x01; meter.current_meter_full = &meter_sdm630_full[0];     meter.current_meter_fast = &meter_sdm630_fast[0];     break;
        case METER_TYPE_DSZ15DZMOD: meter.slave_address = 0x37; meter.current_meter_full = &meter_dsz15dzmod_full[0]; meter.current_meter_fast = &meter_dsz15dzmod_fast[0]; break;
        default:                    meter.slave_address = 0;    meter.current_meter_full = NULL;                      meter.current_meter_fast = NULL;                      break;
    }

    // Reset meter timeout
    meter.timeout = system_timer_get_ms();
}

void meter_find_meter_type(void) {
    static uint8_t find_meter_state = 0;

    switch(find_meter_state) {
        case 0: {
            // Read meter code register with slave address 1 (SDM)
            meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, 1, METER_SDM_HOLDING_REG_METER_CODE, 1);
            find_meter_state++;
            break;
        }

        case 1: {
			uint16_t meter_code = 0xFFFF;
			bool ret = meter_get_read_registers_response(MODBUS_FC_READ_HOLDING_REGISTERS, &meter_code, 1);
			if(ret) {
				if(meter_code != 0xFFFF) {
					rs485.modbus_common_error_counters.timeout = 0;
				}

				modbus_clear_request(&rs485);
				switch(meter_code) {
					case 0x0084: meter_set_meter_type(METER_TYPE_UNSUPPORTED); find_meter_state = 0; return;  // 0x0084 is SDM72V1 (not supported)
					case 0x0089: meter_set_meter_type(METER_TYPE_SDM72V2);     find_meter_state = 0; return;  // Compare datasheet page 16 meter code
					case 0x0000: // Some early versions of the SDM630 return 0x0000 instead of 0x0070 for the meter type register.
					case 0x0070: meter_set_meter_type(METER_TYPE_SDM630);      find_meter_state = 0; return;
					default:     meter.type = METER_TYPE_UNKNOWN;                                    break; 
				}

				find_meter_state++;
			}
			break;
		}

        case 2: {
            // Read meter code register with slave address 0x37 (Eltako)
            meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, 0x37, METER_SDM_HOLDING_REG_METER_CODE, 2);
            find_meter_state++;
            break;
        }

        case 3: {
			uint32_t meter_code = 0xFFFFFFFF;
			bool ret = meter_get_read_registers_response(MODBUS_FC_READ_HOLDING_REGISTERS, &meter_code, 2);
			if(ret) {
				if(meter_code != 0xFFFFFFFF) {
					rs485.modbus_common_error_counters.timeout = 0;
				}

				modbus_clear_request(&rs485);
				switch(meter_code) {
                    case 0x0000000D: meter_set_meter_type(METER_TYPE_DSZ15DZMOD); find_meter_state = 0; rs485.modbus_common_error_counters.timeout = 0; return;
					default:         meter.type = METER_TYPE_UNKNOWN;                                   break; 
				}

				find_meter_state++;
			}
			break;
        }

        default: {
            find_meter_state = 0;
            break;
        }
    }
}

void meter_handle_new_data(MeterRegisterType data, MeterDefinition *definition) {
    if(definition->register_data_type == METER_REGISTER_DATA_TYPE_FLOAT) {
        definition->register_set_address->f = data.f * definition->scale_factor;
    } else if(definition->register_data_type == METER_REGISTER_DATA_TYPE_INT32) {
        definition->register_set_address->f = ((float)data.i32) * definition->scale_factor;
    } else {
        loge("Unsupported data type: %d\n\r", definition->register_data_type);
    }

    // The DSZ15DZMOD meter only has import and export register, but we need the sum.
    if(definition->register_set_address == &meter_fast_register_set.absolute_energy_export) {
        meter_fast_register_set.absolute_energy.f = meter_fast_register_set.absolute_energy_import.f + meter_fast_register_set.absolute_energy_export.f;
    }
}

// The complete register set has been read. Handle differences between meters.
void meter_handle_register_set_read_done(void) {
    if(meter.type == METER_TYPE_DSZ15DZMOD) {
        meter_full_register_set.total_kwh_sum.f = meter_full_register_set.total_import_kwh.f + meter_full_register_set.total_export_kwh.f;
        // Given by DSZ15DZMOD:
        // PF = power_factor (power factor)
        // P  = power (active power)

        // We want to calculate:
        // S  = volt_amps (apparent power)
        // Q  = volt_amps_reactive (reactive power)
        // φ  = phase_angle (phi)

        // Illustration
        //    /|
        // S / |
        //  /  | Q
        // /φ  |
        // -----
        //   P

        // Resulting formulas:
        // PF = P / S     => S = P / PF
        // S² = P² + Q²   => Q = sqrt(S² - P²)
        // P = S * cos(φ) => φ = arccos(P / S)
/*
        for(uint8_t i = 0; i < METER_PHASE_NUM; i++) {
			if(meter_full_register_set.power_factor[i].f == 0.0) {
				meter_full_register_set.volt_amps[i].f = 0.0;
				meter_full_register_set.volt_amps_reactive[i].f = 0.0;
			} else {
            	meter_full_register_set.volt_amps[i].f = meter_full_register_set.power[i].f / meter_full_register_set.power_factor[i].f;
            	meter_full_register_set.volt_amps_reactive[i].f = sqrt(meter_full_register_set.volt_amps[i].f * meter_full_register_set.volt_amps[i].f - meter_full_register_set.power[i].f * meter_full_register_set.power[i].f);
			}
            if(meter_full_register_set.volt_amps[i].f == 0.0) {
				meter_full_register_set.phase_angle[i].f = 0.0;
			} else {
            	meter_full_register_set.phase_angle[i].f = acosf(meter_full_register_set.power[i].f / meter_full_register_set.volt_amps[i].f);
			}
		}
*/
    }
}

void meter_tick(void) {
	static uint8_t last_state = 255;
	static bool read_fast = false;

    if(meter.type == METER_TYPE_UNSUPPORTED) {
        // If meter is not supported, do nothing
        return;
    }

	if(meter.error_wait_time != 0) {
		if(!system_timer_is_time_elapsed_ms(meter.error_wait_time, 500)) {
			return;
		}
		meter.error_wait_time = 0;
	}

	if(meter.first_tick != 0) {
		if(!system_timer_is_time_elapsed_ms(meter.first_tick, 2500)) {
			return;
		}
		meter.first_tick = 0;
	}

    if(meter.type == METER_TYPE_UNKNOWN) {
        meter_find_meter_type();
        return;
    }

	if(last_state != meter.state) {
		meter.timeout = system_timer_get_ms();
		last_state = meter.state;
	} else {
		// If there is no state change at all for 60 seconds we assume that something is broken and trigger the watchdog.
		// This should never happen.
		if(system_timer_is_time_elapsed_ms(meter.timeout, 60000)) {
			while(true) {
				__NOP();
			}
		}
	}

	switch(meter.state) {
        case 0: { // request
            if(system_timer_is_time_elapsed_ms(meter.register_fast_time, 500) || (meter.register_fast_position > 0)) {
                meter_read_registers(MODBUS_FC_READ_INPUT_REGISTERS, meter.slave_address, meter.current_meter_fast[meter.register_fast_position].register_address, 2);
				read_fast = true;
			} else {
                meter_read_registers(MODBUS_FC_READ_INPUT_REGISTERS, meter.slave_address, meter.current_meter_full[meter.register_full_position].register_address, 2);
				read_fast = false;
			}
			meter.state++;
			break;
		}

		case 1: { // read
			bool ret = false;
            MeterRegisterType data;
            ret = meter_get_read_registers_response(MODBUS_FC_READ_INPUT_REGISTERS, &data, 2);
			if(ret) {
                meter_handle_new_data(data, read_fast ? &meter.current_meter_fast[meter.register_fast_position] : &meter.current_meter_full[meter.register_full_position]);
				modbus_clear_request(&rs485);
				meter.state++;
				if(read_fast) {
					meter.register_fast_position++;
                    if(meter.current_meter_fast[meter.register_fast_position].register_set_address == NULL) {
						meter.register_fast_position = 0;
						meter.register_fast_time += 500;
						if(system_timer_is_time_elapsed_ms(meter.register_fast_time, 500)) {
							meter.register_fast_time = system_timer_get_ms();
						}
					}
				} else {
					meter.register_full_position++;
                    if(meter.current_meter_full[meter.register_full_position].register_set_address == NULL) {
						meter.register_full_position = 0;
						meter.each_value_read_once   = true;
                        meter_handle_register_set_read_done();
					}
				}
			}
			break;
		}

		case 2: { // request system type from holding register
			if(meter.register_full_position != 0) {
				meter.state = 0;
				break;
			}

            // Eltako meter does not have system type configuration
            if(meter.type == METER_TYPE_DSZ15DZMOD) {
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
				meter_handle_new_system_type();
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