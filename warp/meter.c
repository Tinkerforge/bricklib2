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

// On some systems, math.h declares a function named logf(float) to calculate
// the natural logarithm of a float number. bricklib2's logging.h defines
// a macro called logf(...) to log fatal errors. The hack below renames the
// unused logf from math.h to logf_math to avoid colliding with logf from
// logging.h.
#define logf logf_math
#include <math.h>
#undef logf

#include "meter.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/utility/crc16.h"
#include "bricklib2/logging/logging.h"

#include "rs485.h"
#include "modbus.h"
#include "meter_eltako.h"
#include "meter_eastron.h"
#include "meter_iskra.h"
#include "meter_generic.h"

#ifdef HAS_HARDWARE_VERSION
#include "hardware_version.h"
#endif

Meter meter;
MeterRegisterSet meter_register_set;

static const MeterDefinition meter_sdm630[] = {
	#include "meter_sdm630_def.inc"
};
static const MeterDefinition meter_sdm72v2[] = {
	#include "meter_sdm72v2_def.inc"
};
static const MeterDefinition meter_dsz15dzmod[] = {
	#include "meter_dsz15dzmod_def.inc"
};
static const MeterDefinition meter_dsz16dze[] = {
	#include "meter_dsz16dze_def.inc"
};
static const MeterDefinition meter_dem4a[] = {
	#include "meter_dem4a_def.inc"
};
static const MeterDefinition meter_dmed341mid7er[] = {
	#include "meter_dmed341mid7er_def.inc"
};
static const MeterDefinition meter_wm3m4c[] = {
	#include "meter_wm3m4c_def.inc"
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

void meter_reset_error_counter(void) {
	rs485.modbus_common_error_counters.illegal_function     = 0;
	rs485.modbus_common_error_counters.illegal_data_address = 0;
	rs485.modbus_common_error_counters.illegal_data_value   = 0;
	rs485.modbus_common_error_counters.slave_device_failure = 0;
	rs485.modbus_common_error_counters.timeout              = 0;
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
	if(fc == MODBUS_FC_WRITE_MULTIPLE_REGISTERS) {
		modbus_store_tx_frame_data_bytes((uint8_t *)&count, 2); // Count.
		modbus_store_tx_frame_data_bytes((uint8_t *)&byte_count, 1); // Byte count.
		modbus_store_tx_frame_data_shorts(&payload->u16[1], 1);
		modbus_store_tx_frame_data_shorts(&payload->u16[0], 1);
	} else if(fc == MODBUS_FC_WRITE_SINGLE_REGISTER) {
		modbus_store_tx_frame_data_shorts(&payload->u16[0], 1);
	}
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
		if(count == 1) { // one 16-bit value
			*((uint16_t*)data) = (d[0] << 8) | (d[1] << 0);
		} else if(count == 2) { // one 32-bit value
			*((uint32_t*)data) = (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
		} else { // count/2 32-bit values
			for(uint8_t i = 0; i < count/2; i++) {
				((uint32_t*)data)[i] = (d[i*4] << 24) | (d[i*4 + 1] << 16) | (d[i*4 + 2] << 8) | (d[i*4+3] << 0);
			}
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
	const uint32_t relative_energy_sum_save    = meter.relative_energy_sum.data;
	const uint32_t relative_energy_import_save = meter.relative_energy_import.data;
	const uint32_t relative_energy_export_save = meter.relative_energy_export.data;

	memset(&meter, 0, sizeof(Meter));

	// Initialize registers with NaN
	for(uint8_t i = 0; i < (sizeof(meter_register_set)/sizeof(uint32_t)); i++) {
		((float*)&meter_register_set)[i] = NAN;
	}

	meter.relative_energy_sum.data    = relative_energy_sum_save;
	meter.relative_energy_import.data = relative_energy_import_save;
	meter.relative_energy_export.data = relative_energy_export_save;
	meter.first_tick                  = system_timer_get_ms();
	meter.register_fast_time          = system_timer_get_ms();
}

// Update phases connected bool array (this is used in communication.c)
void meter_handle_phases_connected(void) {
	meter.phases_connected[0] = meter_register_set.line_to_neutral_volts[0].f > 180.0f;
	meter.phases_connected[1] = meter_register_set.line_to_neutral_volts[1].f > 180.0f;
	meter.phases_connected[2] = meter_register_set.line_to_neutral_volts[2].f > 180.0f;
}

void meter_set_meter_type(MeterType type) {
	meter.type = type;
	switch(type) {
		case METER_TYPE_SDM72V2:       meter.slave_address = 0x01; meter.current_meter = &meter_sdm72v2[0];       break;
		case METER_TYPE_SDM630:        meter.slave_address = 0x01; meter.current_meter = &meter_sdm630[0];        break;
		case METER_TYPE_SDM630MCTV2:   meter.slave_address = 0x01; meter.current_meter = &meter_sdm630[0];        break;
		case METER_TYPE_DSZ15DZMOD:    meter.slave_address = 0x01; meter.current_meter = &meter_dsz15dzmod[0];    break;
		case METER_TYPE_DEM4A:         meter.slave_address = 0x01; meter.current_meter = &meter_dem4a[0];         break;
		case METER_TYPE_DMED341MID7ER: meter.slave_address = 0x01; meter.current_meter = &meter_dmed341mid7er[0]; break;
		case METER_TYPE_DSZ16DZE:      meter.slave_address = 0x01; meter.current_meter = &meter_dsz16dze[0];      break;
		case METER_TYPE_WM3M4C:        meter.slave_address = 0x21; meter.current_meter = &meter_wm3m4c[0];        break;
		default:                       meter.slave_address = 0;    meter.current_meter = NULL;                    break;
	}

	// Reset meter timeout
	meter.timeout = system_timer_get_ms();
}

void meter_find_meter_type(void) {
	static uint8_t find_meter_state = 0;

	MeterType meter_type;

	switch(find_meter_state) {
		case 0: meter_type = meter_generic_is_connected(); break;
		case 1: meter_type = meter_eastron_is_connected(); break;
		case 2: meter_type = meter_eltako_is_connected();  break;
		case 3: meter_type = meter_iskra_is_connected();   break;
		default: find_meter_state = 0; return;
	}

	if(meter_type == METER_TYPE_DETECTION) { // detection ongoing
		return;
	}

	if(meter_type != METER_TYPE_UNKNOWN) { // meter found
		meter_set_meter_type(meter_type);
		meter_reset_error_counter();
		find_meter_state = 0;
		return;
	}

	// Try next manufacturer
	find_meter_state++;
}

void meter_handle_new_data(MeterRegisterType data, const MeterDefinition *definition) {
	switch(definition->register_data_type) {
		case METER_REGISTER_DATA_TYPE_FLOAT: {
			definition->register_set_address->f = data.f * definition->scale_factor;
			break;
		}
		case METER_REGISTER_DATA_TYPE_INT32: {
			definition->register_set_address->f = ((float)data.i32) * definition->scale_factor;
			break;
		}
		case METER_REGISTER_DATA_TYPE_T2:  // Signed Value (16 bit)
		case METER_REGISTER_DATA_TYPE_INT16: {
			definition->register_set_address->f = ((float)data.i16_single) * definition->scale_factor;
			break;
		}
		case METER_REGISTER_DATA_TYPE_T3: { // Unsigned Long Value (32 bit)
			definition->register_set_address->f = ((float)data.u32) * definition->scale_factor;
			break;
		}
		case METER_REGISTER_DATA_TYPE_T5: { // Decade Exponent (Signed 8 bit), Binary Unsigned Value (24 bit), Example: 123456*10^(-3) stored as 0xFD01 0xE240
			const int8_t exponent = data.u32 >> 24;
			const int32_t value   = data.u32 & 0x00FFFFFF;
			float scale = 1.0f;
			if(exponent < 0) {
				for(int8_t i = 0; i < -exponent; i++) {
					scale *= 0.1f;
				}
			} else {
				for(int8_t i = 0; i < exponent; i++) {
					scale *= 10.0f;
				}
			}
			definition->register_set_address->f = ((float)value) * scale * definition->scale_factor;
			break;
		}
		case METER_REGISTER_DATA_TYPE_T6: {  // Decade Exponent (Signed 8 bit), Binary Signed Value (24 bit), Example: -123456*10^(-3) stored as 0xFDFE 0x1DC0
			const int8_t exponent = data.u32 >> 24;
			int32_t value         = data.u32 & 0x00FFFFFF;
			if(value & 0x00800000) { // sign extend negative number
				value |= 0xFF000000;
			}
			float scale = 1.0f;
			if(exponent < 0) {
				for(int8_t i = 0; i < -exponent; i++) {
					scale *= 0.1f;
				}
			} else {
				for(int8_t i = 0; i < exponent; i++) {
					scale *= 10.0f;
				}
			}
			definition->register_set_address->f = ((float)value) * scale * definition->scale_factor;
			break;
		}
		case METER_REGISTER_DATA_TYPE_T7: { // Sign: Import/Export (00/FF), Sign: Inductive/Capacitive (00/FF), Unsigned Value (16 bit), 4 decimal places, Example: 0.9876 CAP stored as 0x00FF 0x2694
			const int8_t sign_ie = (data.u32 >> 24) & 0xFF;
			const int8_t sign_ic = (data.u32 >> 16) & 0xFF;
			const uint16_t value = data.u32 & 0x0000FFFF;
			definition->register_set_address->f = ((float)value) * 0.0001f * definition->scale_factor;
			if(sign_ie == 0xFF) {
				// TODO
			}
			if(sign_ic == 0xFF) {
				// TODO
			}
			break;
		}
		case METER_REGISTER_DATA_TYPE_T16: { // Unsigned Value (16 bit), 2 decimal places, Example: 123.45 stored as 123.45 = 0x3039
			definition->register_set_address->f = ((float)data.u16_single) * 0.01f * definition->scale_factor;
			break;
		}
		case METER_REGISTER_DATA_TYPE_T17: { // Signed Value (16 bit), 2 decimal places, Example: -123.45 stored as -123.45 = 0xCFC7
			definition->register_set_address->f = ((float)data.i16_single) * 0.01f * definition->scale_factor;
			break;
		}
		default: {
			loge("Unsupported data type: %d\n\r", definition->register_data_type);
			break;
		}
	}
}

void meter_handle_register_set_fast_read_done(void) {
	meter.new_fast_value_callback = true;
}

// The complete register set has been read. Handle differences between meters.
// State 0 updates the mandatory values.
// State 1 to n updates the optional values. Call 1 to n with 1 per tick. It takes about 150us to calculate all values.
void meter_handle_register_set_read_done(void) {
	// Update relative values
	meter_register_set.relative_total_import_kwh.f = meter_register_set.total_import_kwh.f - meter.relative_energy_import.f;
	meter_register_set.relative_total_export_kwh.f = meter_register_set.total_export_kwh.f - meter.relative_energy_export.f;
	meter_register_set.relative_total_kwh_sum.f    = meter_register_set.total_kwh_sum.f    - meter.relative_energy_sum.f;
	meter.each_value_read_once = true;
}

uint8_t meter_get_register_size(uint16_t position) {
	switch(meter.current_meter[position].register_data_type) {
		case METER_REGISTER_DATA_TYPE_FLOAT: return 2;
		case METER_REGISTER_DATA_TYPE_INT32: return 2;
		case METER_REGISTER_DATA_TYPE_T2:    // Signed Value (16 bit)
		case METER_REGISTER_DATA_TYPE_INT16: return 1;
		case METER_REGISTER_DATA_TYPE_T3:    return 2;
		case METER_REGISTER_DATA_TYPE_T5:    return 2;
		case METER_REGISTER_DATA_TYPE_T6:    return 2;
		case METER_REGISTER_DATA_TYPE_T7:    return 2;
		case METER_REGISTER_DATA_TYPE_T16:   return 1;
		case METER_REGISTER_DATA_TYPE_T17:   return 1;
		default: return 2;
	}
}



void meter_tick(void) {
	static uint8_t last_state = 255;

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

	switch(meter.type) {
		case METER_TYPE_SDM72V2:
		case METER_TYPE_SDM630:
		case METER_TYPE_SDM630MCTV2:
			meter_eastron_tick();
			break;
		case METER_TYPE_DEM4A:
		case METER_TYPE_DMED341MID7ER:
			meter_generic_tick();
			break;
		case METER_TYPE_WM3M4C:
			meter_iskra_tick();
			break;
		case METER_TYPE_DSZ15DZMOD:
		case METER_TYPE_DSZ16DZE:
			meter_eltako_tick();
			break;
		default:
			return;
	}
}
