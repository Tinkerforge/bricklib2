/* TNG
 * Copyright (C) 2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tng.c: TNG system standard init/tick
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

#include "tng.h"

#include "configs/config.h"

#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/logging/logging.h"
#include "bricklib2/tng/usb_stm32/usb.h"
#include "bricklib2/protocols/tfp/tfp.h"

#include "communication.h"
#include "tng_communication.h"
#include "tng_led.h"

#include <string.h>

#define TNG_BUFFER_SIZE 256

static uint8_t tng_request_data[TNG_BUFFER_SIZE] = {0};
static uint8_t tng_response_data[TNG_BUFFER_SIZE] = {0};
static uint16_t tng_request_length = 0;

static TFPMessageHeader *tng_request_header = (TFPMessageHeader*)tng_request_data;
static TFPMessageHeader *tng_response_header = (TFPMessageHeader*)tng_response_data;

bool tng_is_valid_request(TFPMessageHeader *header) {
	if((header->uid != 0) && (tng_get_uid() != header->uid)) {
		return false;
	}

	return true;
}

void tng_tick(void) {
	if(usb_can_recv()) {
		if(tng_request_length < sizeof(TFPMessageHeader)) {
			tng_request_length += usb_recv(tng_request_data + tng_request_length, TNG_BUFFER_SIZE - tng_request_length);
		}

		if((tng_request_length >= sizeof(TFPMessageHeader)) && (tng_request_length < tng_request_header->length)) {
			tng_request_length += usb_recv(tng_request_data + tng_request_length, TNG_BUFFER_SIZE - tng_request_length);
		}

		if(tng_request_length >= tng_request_header->length) {
			if(tng_response_header->length == 0) {
				if(tng_is_valid_request(tng_request_header)) {
					memcpy(tng_response_data, tng_request_data, sizeof(TFPMessageHeader));
					tng_response_header->length = 0;
					TNGHandleMessageResponse handle_message_return = tng_handle_message(tng_request_data, tng_response_data);
					if(handle_message_return == HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED) {
						handle_message_return = handle_message(tng_request_data, tng_response_data);
					}

					if(tng_response_header->return_expected) {
						if(tng_response_header->length == 0) {
							tng_response_header->length = 8;
						}
						switch(handle_message_return) {
							case HANDLE_MESSAGE_RESPONSE_EMPTY:             tng_response_header->error = TFP_MESSAGE_ERROR_CODE_OK;                break;
							case HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED:     tng_response_header->error = TFP_MESSAGE_ERROR_CODE_NOT_SUPPORTED;     break;
							case HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER: tng_response_header->error = TFP_MESSAGE_ERROR_CODE_INVALID_PARAMETER; break;
							case HANDLE_MESSAGE_RESPONSE_NONE:              break;
							default: break;
						}
					}
				}

				if(tng_request_length > tng_request_header->length) {
					memmove(tng_request_data + tng_request_header->length, tng_request_data, TNG_BUFFER_SIZE - tng_request_header->length);
					tng_request_length -= tng_request_header->length;
				} else {
					tng_request_length = 0;
				}

			}
		}
	}

	if(tng_response_header->length >= 8) {
		if(usb_send(tng_response_data, tng_response_header->length)) {
			tng_response_header->length = 0;
		}
	}

	communication_tick();
	tng_led_tick();
}

void tng_init(void) {
#if (PREFETCH_ENABLE != 0)
  __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
#endif

	RCC_OscInitTypeDef rcc_osc = {
		.OscillatorType = RCC_OSCILLATORTYPE_HSE,
		.HSEState       = RCC_HSE_ON,
		.PLL.PLLState   = RCC_PLL_ON,
		.PLL.PLLSource  = RCC_PLLSOURCE_HSE,
		.PLL.PLLMUL     = RCC_PLL_MUL6,
		.PLL.PREDIV     = RCC_PREDIV_DIV1
	};
	HAL_StatusTypeDef status = HAL_RCC_OscConfig(&rcc_osc);
	if(status != HAL_OK) {
		loge("RCC_OscConfig error: %d\n\r", status);
	}

	RCC_ClkInitTypeDef rcc_clk = {
		.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1,
		.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK,
		.AHBCLKDivider  = RCC_SYSCLK_DIV1,
		.APB1CLKDivider = RCC_HCLK_DIV1
	};
	status = HAL_RCC_ClockConfig(&rcc_clk, FLASH_LATENCY_1);
	if(status != HAL_OK) {
		loge("RCC_ClockConfig error: %d\n\r", status);
	}

	RCC_PeriphCLKInitTypeDef rcc_periph_clk = {
		.PeriphClockSelection = RCC_PERIPHCLK_USB,
		.UsbClockSelection    = RCC_USBCLKSOURCE_PLL
	};
	status = HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk);
	if(status != HAL_OK) {
		loge("RCCEx_PeriphCLKConfig error: %d\n\r", status);
	}

	system_timer_init(HAL_RCC_GetHCLKFreq(), SYSTEM_TIMER_FREQUENCY);

	usb_init();
	communication_init();
	tng_led_init();
}

uint32_t tng_get_uid(void) {
	volatile uint32_t *uid = (volatile uint32_t *)STM32F0_UID_POSITION;
	if((*uid == 0) || (*uid == 0xFFFFFFFF)) {
		return 2;
	}

	return *uid;
}