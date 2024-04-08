/* bricklib2 warp
 * Copyright (C) 2020-2023 Olaf Lüke <olaf@tinkerforge.com>
 *
 * contactor_check.c: Welded/defective contactor check functions
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

#include "contactor_check.h"

#include "configs/config_contactor_check.h"
#include "configs/config_evse.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/logging/logging.h"

#include "configs/config.h"

#ifdef HAS_HARDWARE_VERSION
#include "hardware_version.h"
#endif

#include <string.h>

#define CONTACTOR_CHECK_INTERVAL 250

ContactorCheck contactor_check;

void contactor_check_init(void) {
	memset(&contactor_check, 0, sizeof(ContactorCheck));
	const XMC_GPIO_CONFIG_t pin_config_input = {
		.mode             = XMC_GPIO_MODE_INPUT_TRISTATE,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

#ifdef HAS_HARDWARE_VERSION
	if(hardware_version.is_v2) {
#endif
		XMC_GPIO_Init(CONTACTOR_CHECK_AC1_PIN, &pin_config_input);
		XMC_GPIO_Init(CONTACTOR_CHECK_AC2_PIN, &pin_config_input);

		contactor_check.ac1_last_value = XMC_GPIO_GetInput(CONTACTOR_CHECK_AC1_PIN);
		contactor_check.ac2_last_value = XMC_GPIO_GetInput(CONTACTOR_CHECK_AC2_PIN);
#ifdef HAS_HARDWARE_VERSION
	} else {
		XMC_GPIO_Init(CONTACTOR_CHECK_FB1_PIN, &pin_config_input);
		XMC_GPIO_Init(CONTACTOR_CHECK_FB2_PIN, &pin_config_input);
		XMC_GPIO_Init(CONTACTOR_CHECK_PE_PIN, &pin_config_input);

		contactor_check.pe_last_value = XMC_GPIO_GetInput(CONTACTOR_CHECK_PE_PIN);
	}
#endif
}

void contactor_check_tick(void) {
#ifdef HAS_HARDWARE_VERSION
	if(hardware_version.is_v2) {
#endif
		bool ac1_new_value = XMC_GPIO_GetInput(CONTACTOR_CHECK_AC1_PIN);
		bool ac2_new_value = XMC_GPIO_GetInput(CONTACTOR_CHECK_AC2_PIN);

		if(ac1_new_value != contactor_check.ac1_last_value) {
			contactor_check.ac1_last_value = ac1_new_value;
			contactor_check.ac1_edge_count++;
		}

		if(ac2_new_value != contactor_check.ac2_last_value) {
			contactor_check.ac2_last_value = ac2_new_value;
			contactor_check.ac2_edge_count++;
		}

		if(system_timer_is_time_elapsed_ms(contactor_check.last_check, CONTACTOR_CHECK_INTERVAL)) {
			contactor_check.last_check = system_timer_get_ms();

			if(contactor_check.invalid_counter > 0) {
				contactor_check.invalid_counter--;
			} else {
				// Check for edge count of 10. We expect an edge count of 25,
				// but an edge count > 0 should already be enough to detect the 230V.
				// To make sure that we don't see any random glitches we check for > 10 as a compromise.
				const bool ac1_live = contactor_check.ac1_edge_count > 10;
				const bool ac2_live = contactor_check.ac2_edge_count > 10;

				contactor_check.state = ac1_live | (ac2_live << 1);

				const bool relay_state = CONTACTOR_CHECK_RELAY_PIN_IS_INVERTED ? !XMC_GPIO_GetInput(EVSE_CONTACTOR_PIN) : XMC_GPIO_GetInput(EVSE_CONTACTOR_PIN);
				if(relay_state) {
					// If contact is switched on, we expect to have 230V AC on both sides of it
					switch(contactor_check.state) {
						case CONTACTOR_CHECK_STATE_AC1_NLIVE_AC2_NLIVE: contactor_check.error = 1; break;
						case CONTACTOR_CHECK_STATE_AC1_LIVE_AC2_NLIVE:  contactor_check.error = 2; break;
						case CONTACTOR_CHECK_STATE_AC1_NLIVE_AC2_LIVE:  contactor_check.error = 3; break;
						case CONTACTOR_CHECK_STATE_AC1_LIVE_AC2_LIVE:   contactor_check.error = 0; break;
					}
				} else {
					// If contact is switched off, we expect to have 230V AC on AC1 and nothing on AC2
					switch(contactor_check.state) {
						case CONTACTOR_CHECK_STATE_AC1_NLIVE_AC2_NLIVE: contactor_check.error = 4; break;
						case CONTACTOR_CHECK_STATE_AC1_LIVE_AC2_NLIVE:  contactor_check.error = 0; break;
						case CONTACTOR_CHECK_STATE_AC1_NLIVE_AC2_LIVE:  contactor_check.error = 5; break;
						case CONTACTOR_CHECK_STATE_AC1_LIVE_AC2_LIVE:   contactor_check.error = 6; break;
					}
				}
			}

			contactor_check.ac1_edge_count = 0;
			contactor_check.ac2_edge_count = 0;
		}
#ifdef HAS_HARDWARE_VERSION
	} else {
		const bool check_n_l1   = XMC_GPIO_GetInput(CONTACTOR_CHECK_FB1_PIN); // low = contactor active, high = contactor not active
		const bool check_l2_l3  = XMC_GPIO_GetInput(CONTACTOR_CHECK_FB2_PIN); // low = contactor active, high = contactor not active
		const bool check_pe     = XMC_GPIO_GetInput(CONTACTOR_CHECK_PE_PIN);  // 50hz = PE check OK, constant = PE check fail

		const bool contactor    = XMC_GPIO_GetInput(EVSE_CONTACTOR_PIN);      // low = contactor aux active, high = contactor aux not active
		const bool phase_switch = XMC_GPIO_GetInput(EVSE_PHASE_SWITCH_PIN);   // low = contactor aux active, high = contactor aux not active


		// PE check
		if(check_pe != contactor_check.pe_last_value) {
			contactor_check.pe_last_value = check_pe;
			contactor_check.pe_edge_count++;
		}
		if(system_timer_is_time_elapsed_ms(contactor_check.last_check, CONTACTOR_CHECK_INTERVAL)) {
			contactor_check.last_check = system_timer_get_ms();

			if(contactor_check.invalid_counter > 0) {
				contactor_check.invalid_counter--;
			} else {
				// Check for edge count of 10. We expect an edge count of 25,
				// but an edge count > 0 should already be enough to detect the 230V.
				// To make sure that we don't see any random glitches we check for > 10 as a compromise.
				contactor_check.error = contactor_check.pe_edge_count > 10 ? 0 : 1;
			}
			contactor_check.pe_edge_count = 0;
		}

		// Only keep the last value of PE check
		contactor_check.error = contactor_check.error & 1;

		// N/L1 and L2/L3 check
		uint8_t error = 0;
		switch((contactor << 3) | (phase_switch << 2) | (check_n_l1 << 1) | (check_l2_l3 << 0)) {
			// contactor active + 3phase
			case 0b0000: error = 0;  break; // contactor aux and phase switch aux active -> OK
			case 0b0001: error = 1;  break;
			case 0b0010: error = 2;  break;
			case 0b0011: error = 3;  break;

			// contactor active + 1phase
			case 0b0100: error = 4;  break;
			case 0b0101: error = 0;  break; // contactor aux active and phase switch aux not active -> OK
			case 0b0110: error = 5;  break;
			case 0b0111: error = 6;  break;

			// contactor not active (1/3phase not relevant)
			case 0b1000: error = 7;  break;
			case 0b1001: error = 8;  break;
			case 0b1010: error = 9;  break;
			case 0b1011: error = 0;  break; // contactor aux not active and phase switch aux not active -> OK
			case 0b1100: error = 10; break;
			case 0b1101: error = 11; break;
			case 0b1110: error = 12; break;
			case 0b1111: error = 0;  break; // contactor aux not active and phase switch aux not active -> OK

			default:     error = 13; break; // Impossible
		}

		if(error == 0) {
			contactor_check.last_error_time = 0;
		} else {
			if(contactor_check.last_error_time == 0) {
				contactor_check.last_error_time = system_timer_get_ms();
			} else if(system_timer_is_time_elapsed_ms(contactor_check.last_error_time, 250)) { // 250ms error debounce
				contactor_check.error |= (error << 1);

				// Make sure we reach here again if the error persists
				contactor_check.last_error_time = system_timer_get_ms() - 251;
			}
		}

		// The data that we send to the Brick uses "active high", so we invert the inputs here
		contactor_check.state = (!check_n_l1) | ((!check_l2_l3) << 1) | ((contactor_check.error != 0) << 2) | ((!contactor) << 3) | ((!phase_switch) << 4);
	}
#endif
}
