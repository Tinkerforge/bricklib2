/* evse-v2-bricklet
 * Copyright (C) 2025 Olaf Lüke <olaf@tinkerforge.com>
 *
 * meter_eltako.c: Modbus meter driver for Eltako
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

#include "meter_eltako.h"

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
#include "bricklib2/logging/logging.h"

#include "rs485.h"
#include "modbus.h"

#ifdef HAS_HARDWARE_VERSION
#include "hardware_version.h"
#endif


// Define USE_SIMPLE_SQRTF and USE_SIMPLE_ACOSF
// to save ~2800 bytes flash

float simple_sqrtf(const float x) {
#ifdef USE_SIMPLE_SQRTF
    if (x <= 0.0f) {
		return 0.0f;
	}

    // Initial guess via bit-level hack
    union { float f; uint32_t i; } u = { x };
    u.i = (u.i >> 1) + 0x1fc00000; // approximate sqrt(x)
    float y = u.f;

    // Two Newton-Raphson iterations: y = 0.5*(y + x/y)
    y = 0.5f * (y + x / y);
    y = 0.5f * (y + x / y);

	// This should be accurate to 0.01%
	// If we use 1 iteration it is 0.1%-0.3% accurate
	// We can go to 1 iteration if we need the speed

    return y;
#else
	return sqrtf(x);
#endif
}

float simple_acosf(float x) {
#ifdef USE_SIMPLE_ACOSF
    if (x <= -1.0f) return 3.14159265f;
    if (x >=  1.0f) return 0.0f;

    float y = simple_sqrtf(1.0f - x);
    return y * (1.5707963f + (-0.2146018f)*x + 0.0865667f*x*x);
#else
	return acosf(x);
#endif
}

MeterType meter_eltako_is_connected(void) {
	static uint8_t find_meter_state = 0;

	switch(find_meter_state) {
		case 0: {
			XMC_USIC_CH_SetBaudrate(RS485_USIC, 9600, RS485_OVERSAMPLING);

			// Read manufacturing code register with slave address 0x01 (Eltako)
			meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, 0x01, METER_ELTAKO_HOLDING_REG_MANUFACTURING_CODE, 2);
			find_meter_state++;
			return METER_TYPE_DETECTION;
		}

		case 1: {
			uint32_t manufacturing_code = 0xFFFFFFFF;
			bool ret = meter_get_read_registers_response(MODBUS_FC_READ_HOLDING_REGISTERS, &manufacturing_code, 2);
			if(ret) {
				modbus_clear_request(&rs485);
				switch(manufacturing_code) {
					case 0x0000000D: break; // Manufacturer is Eltako (handled in next state)
					default: {
						find_meter_state = 0;
						return METER_TYPE_UNKNOWN;
					}
				}

				find_meter_state++;
			}
			return METER_TYPE_DETECTION;
		}

		case 2: {
			// Read meter code register with slave address 0x01 (Eltako)
			meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, 0x01, METER_ELTAKO_HOLDING_REG_METER_CODE, 2);
			find_meter_state++;
			return METER_TYPE_DETECTION;
		}

		case 3: {
			uint32_t meter_code = 0xFFFFFFFF;
			bool ret = meter_get_read_registers_response(MODBUS_FC_READ_HOLDING_REGISTERS, &meter_code, 2);
			if(ret) {
				modbus_clear_request(&rs485);
				switch(meter_code) {
					case 0x00000001: return METER_TYPE_DSZ15DZMOD;
					case 0x00000003: break; // For DSZ16DZE we have to configure direction
					// Assume DSZ15DZMOD as default
					default:         return METER_TYPE_DSZ15DZMOD;
				}

				find_meter_state++;
			}
			return METER_TYPE_DETECTION;
		}

		// case 4-7: Set direction for DSZ16DZE.
		// This is done with two single register writes.
		// DSZ16DZE does not seem to support multiple register writes.
		case 4: {
			// Set reverse direction first word (only DSZ16DZE)
			MeterRegisterType direction;
			direction.u16[0] = 0;

			modbus_clear_request(&rs485);
			meter_write_register(MODBUS_FC_WRITE_SINGLE_REGISTER, 0x01, METER_ELTAKO_HOLDING_REG_REVERSE_DIRECTION, &direction);
			find_meter_state++;
			return METER_TYPE_DETECTION;
		}

		case 5: { // check direction first word
			bool ret = meter_get_write_register_response(MODBUS_FC_WRITE_SINGLE_REGISTER);
			if(ret) {
				modbus_clear_request(&rs485);
				find_meter_state++;
			}
			return METER_TYPE_DETECTION;
		}

		case 6: {
			// Set reverse direction second word (only DSZ16DZE)
			MeterRegisterType direction;
			direction.u16[0] = 1;

			modbus_clear_request(&rs485);
			meter_write_register(MODBUS_FC_WRITE_SINGLE_REGISTER, 0x01, METER_ELTAKO_HOLDING_REG_REVERSE_DIRECTION+1, &direction);
			find_meter_state++;
			return METER_TYPE_DETECTION;
		}

		case 7: { // check direction second word
			bool ret = meter_get_write_register_response(MODBUS_FC_WRITE_SINGLE_REGISTER);
			if(ret) {
				modbus_clear_request(&rs485);

				find_meter_state = 0;
				return METER_TYPE_DSZ16DZE;
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

// The complete register set has been read. Handle differences between meters.
// State 0 updates the mandatory values.
// State 1 to n updates the optional values. Call 1 to n with 1 per tick. It takes about 150us to calculate all values.
bool meter_eltako_handle_register_set_read_done(uint8_t state) {
#ifdef IS_CHARGER
	static float value = 0; // temporary value between states
#endif
	if(state == 0) {
		meter_handle_register_set_read_done();
	} else if(meter.type == METER_TYPE_DSZ15DZMOD) {
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

		switch(state) {
			case 1:  meter_register_set.volt_amps[0].f                  = meter_register_set.power_factor[0].f == 0.0f ? 0.0f : meter_register_set.power[0].f / meter_register_set.power_factor[0].f; break;
			case 2:  meter_register_set.volt_amps[1].f                  = meter_register_set.power_factor[1].f == 0.0f ? 0.0f : meter_register_set.power[1].f / meter_register_set.power_factor[1].f; break;
			case 3:  meter_register_set.volt_amps[2].f                  = meter_register_set.power_factor[2].f == 0.0f ? 0.0f : meter_register_set.power[2].f / meter_register_set.power_factor[2].f; break;
// We don't compile support for sqrt and acos in Energy Manager firmware currently
#ifdef IS_CHARGER
			case 4:  meter_register_set.volt_amps_reactive[0].f         = meter_register_set.power_factor[0].f == 0.0f ? 0.0f : simple_sqrtf(meter_register_set.volt_amps[0].f * meter_register_set.volt_amps[0].f - meter_register_set.power[0].f * meter_register_set.power[0].f); break;
			case 5:  meter_register_set.volt_amps_reactive[1].f         = meter_register_set.power_factor[1].f == 0.0f ? 0.0f : simple_sqrtf(meter_register_set.volt_amps[1].f * meter_register_set.volt_amps[1].f - meter_register_set.power[1].f * meter_register_set.power[1].f); break;
			case 6:  meter_register_set.volt_amps_reactive[2].f         = meter_register_set.power_factor[2].f == 0.0f ? 0.0f : simple_sqrtf(meter_register_set.volt_amps[2].f * meter_register_set.volt_amps[2].f - meter_register_set.power[2].f * meter_register_set.power[2].f); break;
			case 7:  meter_register_set.phase_angle[0].f                = meter_register_set.volt_amps[0].f    == 0.0f ? 0.0f : simple_acosf(meter_register_set.power[0].f / meter_register_set.volt_amps[0].f); break;
			case 8:  meter_register_set.phase_angle[1].f                = meter_register_set.volt_amps[1].f    == 0.0f ? 0.0f : simple_acosf(meter_register_set.power[1].f / meter_register_set.volt_amps[1].f); break;
			case 9:  meter_register_set.phase_angle[2].f                = meter_register_set.volt_amps[2].f    == 0.0f ? 0.0f : simple_acosf(meter_register_set.power[2].f / meter_register_set.volt_amps[2].f); break;
			case 10: value                                              = meter_register_set.current[0].f * meter_register_set.current[0].f + meter_register_set.current[1].f * meter_register_set.current[1].f + meter_register_set.current[2].f * meter_register_set.current[2].f - meter_register_set.current[0].f * meter_register_set.current[1].f - meter_register_set.current[0].f * meter_register_set.current[2].f - meter_register_set.current[1].f * meter_register_set.current[2].f; break;
			case 11: meter_register_set.neutral_current.f               = simple_sqrtf(ABS(value)); break;
#endif
			case 12: meter_register_set.average_line_to_neutral_volts.f = (meter_register_set.line_to_neutral_volts[0].f + meter_register_set.line_to_neutral_volts[1].f + meter_register_set.line_to_neutral_volts[2].f) / 3.0f; break;
			case 13: meter_register_set.average_line_current.f          = (meter_register_set.current[0].f + meter_register_set.current[1].f + meter_register_set.current[2].f) / 3.0f; break;
			case 14: meter_register_set.sum_of_line_currents.f          = meter_register_set.current[0].f + meter_register_set.current[1].f + meter_register_set.current[2].f; break;
			case 15: meter_register_set.total_system_volt_amps.f        = meter_register_set.volt_amps[0].f + meter_register_set.volt_amps[1].f + meter_register_set.volt_amps[2].f; break;
			case 16: meter_register_set.total_system_var.f              = meter_register_set.volt_amps_reactive[0].f + meter_register_set.volt_amps_reactive[1].f + meter_register_set.volt_amps_reactive[2].f; break;
			case 17: meter_register_set.total_system_phase_angle.f      = meter_register_set.phase_angle[0].f + meter_register_set.phase_angle[1].f + meter_register_set.phase_angle[2].f; break;
			case 18: meter_register_set.total_kwh_sum.f                 = meter_register_set.total_import_kwh.f + meter_register_set.total_export_kwh.f; break;
			default: return false;
		}
	} else if(meter.type == METER_TYPE_DSZ16DZE) {
		// Convert phase angle from acos(phi) to degrees
		switch(state) {
#ifdef IS_CHARGER
			case 1: meter_register_set.phase_angle[0].f                 = simple_acosf(meter_register_set.phase_angle[0].f); break;
			case 2: meter_register_set.phase_angle[0].f                 = (meter_register_set.volt_amps_reactive[0].f < 0) ? -meter_register_set.phase_angle[0].f*57.29577951308232f : meter_register_set.phase_angle[0].f*57.29577951308232f; break;
			case 3: meter_register_set.phase_angle[1].f                 = simple_acosf(meter_register_set.phase_angle[1].f); break;
			case 4: meter_register_set.phase_angle[1].f                 = (meter_register_set.volt_amps_reactive[1].f < 0) ? -meter_register_set.phase_angle[1].f*57.29577951308232f : meter_register_set.phase_angle[1].f*57.29577951308232f; break;
			case 5: meter_register_set.phase_angle[2].f                 = simple_acosf(meter_register_set.phase_angle[2].f); break;
			case 6: meter_register_set.phase_angle[2].f                 = (meter_register_set.volt_amps_reactive[2].f < 0) ? -meter_register_set.phase_angle[2].f*57.29577951308232f : meter_register_set.phase_angle[2].f*57.29577951308232f; break;
			case 7: meter_register_set.total_system_phase_angle.f       = simple_acosf(meter_register_set.total_system_phase_angle.f); break;
			case 8: meter_register_set.total_system_phase_angle.f       = (meter_register_set.total_system_var.f < 0) ? -meter_register_set.total_system_phase_angle.f*57.29577951308232f : meter_register_set.total_system_phase_angle.f*57.29577951308232f; break;
#endif
			default: return false;
		}
	}
	return true;
}

// Since the eltako meter only has a few registers, we read them all at once.
// This means we don't need to differentiate between fast and slow read. We can read the complete register set about 3 times per second.
void meter_eltako_tick(void) {
	static uint32_t eltako_time_since_read = 0;
	static MeterRegisterType eltako_data[METER_ELTAKO_REGISTER_COUNT];
	static bool eltako_data_new = false;
	static uint8_t eltako_data_i = 0;

	switch(meter.state) {
		case 0: { // request all registers
			meter_read_registers(MODBUS_FC_READ_INPUT_REGISTERS, meter.slave_address, meter.current_meter[0].register_address, METER_ELTAKO_REGISTER_COUNT);
			meter.state++;
			break;
		}
		case 1: { // read all registers
			bool ret = meter_get_read_registers_response(MODBUS_FC_READ_INPUT_REGISTERS, &eltako_data, METER_ELTAKO_REGISTER_COUNT);
			if(ret) {
				eltako_time_since_read = system_timer_get_ms();
				meter.state++;
				eltako_data_new = true;
				eltako_data_i = 0;
			}
			break;
		}
		case 2: { // handle new data
			if(eltako_data_new) {
				uint16_t reg = meter.current_meter[eltako_data_i].register_address;
				if(reg != 0) {
					meter_handle_new_data(eltako_data[(reg-1)/2], &meter.current_meter[eltako_data_i]);
					eltako_data_i++;
				} else {
					meter.state++;
					modbus_clear_request(&rs485);
					meter_handle_phases_connected();

					// reuse new and i for next state and start with i=1
					eltako_data_new = true;
					eltako_data_i   = 1;
				}
			} else {
				meter.state++;
			}
			break;
		}

		case 3: { // calculate values from data
			if(system_timer_is_time_elapsed_ms(eltako_time_since_read, 100)) {
				eltako_data_new = false;
				eltako_data_i   = 0;
				meter.state     = 0;
			} else if(eltako_data_new) {
				if(meter_eltako_handle_register_set_read_done(eltako_data_i)) {
					eltako_data_i++;
				} else {
					meter_eltako_handle_register_set_read_done(0);
					meter_handle_register_set_fast_read_done();
					eltako_data_new = false;
					eltako_data_i   = 0;
				}
			}
			break;
		}

		default: {
			meter.state = 0;
			break;
		}
	}
}