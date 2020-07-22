/* TNG
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tng_led.c: TNG status/channel LED driver
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


#include "tng_led.h"

#include "configs/config.h"

#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/hal/system_timer/system_timer.h"

#include "tng_led_cie1931.inc"
#include "tng_led_rainbow.inc"

#define TNG_LED_PERIOD 0xFFFF

// If there is one status pin defined, we assume that R, G and B are available
#ifdef TNG_LED_STATUS_R_PIN
const uint32_t tng_led_status_pin[TNG_LED_STATUS_NUM] = {
	TNG_LED_STATUS_R_PIN,
	TNG_LED_STATUS_G_PIN,
	TNG_LED_STATUS_B_PIN
};

const GPIO_TypeDef *tng_led_status_port[TNG_LED_STATUS_NUM] = {
	TNG_LED_STATUS_R_PORT,
	TNG_LED_STATUS_G_PORT,
	TNG_LED_STATUS_B_PORT
};

const uint8_t tng_led_status_tim_alt[TNG_LED_STATUS_NUM] = {
	TNG_LED_STATUS_R_TIM_ALT,
	TNG_LED_STATUS_G_TIM_ALT,
	TNG_LED_STATUS_B_TIM_ALT
};

const uint32_t tng_led_status_tim_ch[TNG_LED_STATUS_NUM] = {
	TNG_LED_STATUS_R_TIM_CH,
	TNG_LED_STATUS_G_TIM_CH,
	TNG_LED_STATUS_B_TIM_CH
};
#endif


#ifdef TNG_LED_CHANNEL_0_PIN
const uint32_t tng_led_channel_pin[TNG_LED_CHANNEL_NUM] = {
	TNG_LED_CHANNEL_0_PIN,
#ifdef TNG_LED_CHANNEL_1_PIN
	TNG_LED_CHANNEL_1_PIN,
#endif
#ifdef TNG_LED_CHANNEL_2_PIN
	TNG_LED_CHANNEL_2_PIN,
#endif
#ifdef TNG_LED_CHANNEL_3_PIN
	TNG_LED_CHANNEL_3_PIN,
#endif
#ifdef TNG_LED_CHANNEL_4_PIN
	TNG_LED_CHANNEL_4_PIN,
#endif
#ifdef TNG_LED_CHANNEL_5_PIN
	TNG_LED_CHANNEL_5_PIN,
#endif
#ifdef TNG_LED_CHANNEL_6_PIN
	TNG_LED_CHANNEL_6_PIN,
#endif
#ifdef TNG_LED_CHANNEL_7_PIN
	TNG_LED_CHANNEL_7_PIN,
#endif
#ifdef TNG_LED_CHANNEL_8_PIN
	TNG_LED_CHANNEL_8_PIN,
#endif
#ifdef TNG_LED_CHANNEL_9_PIN
	TNG_LED_CHANNEL_9_PIN,
#endif
#ifdef TNG_LED_CHANNEL_10_PIN
	TNG_LED_CHANNEL_10_PIN,
#endif
#ifdef TNG_LED_CHANNEL_11_PIN
	TNG_LED_CHANNEL_11_PIN,
#endif
};

const GPIO_TypeDef *tng_led_channel_port[TNG_LED_CHANNEL_NUM] = {
	TNG_LED_CHANNEL_0_PORT,
#ifdef TNG_LED_CHANNEL_1_PORT
	TNG_LED_CHANNEL_1_PORT,
#endif
#ifdef TNG_LED_CHANNEL_2_PORT
	TNG_LED_CHANNEL_2_PORT,
#endif
#ifdef TNG_LED_CHANNEL_3_PORT
	TNG_LED_CHANNEL_3_PORT,
#endif
#ifdef TNG_LED_CHANNEL_4_PORT
	TNG_LED_CHANNEL_4_PORT,
#endif
#ifdef TNG_LED_CHANNEL_5_PORT
	TNG_LED_CHANNEL_5_PORT,
#endif
#ifdef TNG_LED_CHANNEL_6_PORT
	TNG_LED_CHANNEL_6_PORT,
#endif
#ifdef TNG_LED_CHANNEL_7_PORT
	TNG_LED_CHANNEL_7_PORT,
#endif
#ifdef TNG_LED_CHANNEL_8_PORT
	TNG_LED_CHANNEL_8_PORT,
#endif
#ifdef TNG_LED_CHANNEL_9_PORT
	TNG_LED_CHANNEL_9_PORT,
#endif
#ifdef TNG_LED_CHANNEL_10_PORT
	TNG_LED_CHANNEL_10_PORT,
#endif
#ifdef TNG_LED_CHANNEL_11_PORT
	TNG_LED_CHANNEL_11_PORT,
#endif
};

#endif

#ifdef TNG_LED_STATUS_R_PIN
void tng_led_status_set(const uint8_t r, const uint8_t g, const uint8_t b) {
	TIM1->CCR1 = TNG_LED_PERIOD - tng_led_cie1931[r];
	TIM1->CCR2 = TNG_LED_PERIOD - tng_led_cie1931[g];
	TIM1->CCR3 = TNG_LED_PERIOD - tng_led_cie1931[b];
}
#endif

#ifdef TNG_LED_CHANNEL_0_PIN
void tng_led_channel_set(const TNGLEDChannel channel, const bool enable) {
	if(channel < TNG_LED_CHANNEL_NUM) {
		HAL_GPIO_WritePin((GPIO_TypeDef *)tng_led_channel_port[channel], tng_led_channel_pin[channel], enable ? GPIO_PIN_RESET : GPIO_PIN_SET);
	}
}
#endif

void tng_led_tick_breathing(void) {
	int16_t value = system_timer_get_ms()/4 % 512;
	if(value > 255) {
		value = 511 - value;
	}

	tng_led_status_set(0, value, 0);
}

void tng_led_tick_rainbow(void) {
	uint8_t value = system_timer_get_ms()/4 % 250;

	tng_led_status_set(tng_led_rainbow[value][0], tng_led_rainbow[value][1], tng_led_rainbow[value][2]);
}

void tng_led_tick(void) {
	tng_led_tick_breathing();
}

void tng_led_init(void) {
#if defined(TNG_LED_STATUS_R_PIN) || defined(TNG_LED_CHANNEL_0_PIN)
	// TODO: Only enable necessary GPIO ports
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
#endif

#ifdef TNG_LED_STATUS_R_PIN
	__HAL_RCC_TIM1_CLK_ENABLE(); // TODO: Add support for other TIMs
	TIM_HandleTypeDef tim = {
		.Instance = TIM1, // TODO: Add support for other TIMs

		.Init.Prescaler         = 0,
		.Init.Period            = TNG_LED_PERIOD - 1,
		.Init.ClockDivision     = 0,
		.Init.CounterMode       = TIM_COUNTERMODE_UP,
		.Init.RepetitionCounter = 0,
		.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE
	};
	HAL_TIM_PWM_Init(&tim);

	GPIO_InitTypeDef gpio_tim = {
		.Mode  = GPIO_MODE_AF_PP,
		.Pull  = GPIO_PULLUP,
		.Speed = GPIO_SPEED_FREQ_HIGH,
	};

	TIM_OC_InitTypeDef oc = {
		.OCMode       = TIM_OCMODE_PWM1,
  		.OCPolarity   = TIM_OCPOLARITY_HIGH,
		.OCFastMode   = TIM_OCFAST_DISABLE,
		.OCNPolarity  = TIM_OCNPOLARITY_HIGH,
		.OCNIdleState = TIM_OCNIDLESTATE_RESET,
		.OCIdleState  = TIM_OCIDLESTATE_RESET,
		.Pulse        = TNG_LED_PERIOD
	};

	for(uint8_t i = 0; i < TNG_LED_STATUS_NUM; i++) {
		gpio_tim.Pin       = tng_led_status_pin[i];
		gpio_tim.Alternate = tng_led_status_tim_alt[i];
		HAL_GPIO_Init((GPIO_TypeDef *)tng_led_status_port[i], &gpio_tim);
		HAL_TIM_PWM_ConfigChannel(&tim, &oc, tng_led_status_tim_ch[i]);
		HAL_TIM_PWM_Start(&tim, tng_led_status_tim_ch[i]);
	}
#endif

#ifdef TNG_LED_CHANNEL_0_PIN
	GPIO_InitTypeDef gpio_out = {
		.Mode  = GPIO_MODE_OUTPUT_PP,
		.Pull  = GPIO_NOPULL,
		.Speed = GPIO_SPEED_FREQ_LOW,
	};
	for(uint8_t i = 0; i < TNG_LED_CHANNEL_NUM; i++) {
		gpio_out.Pin = tng_led_channel_pin[i];
		HAL_GPIO_Init((GPIO_TypeDef *)tng_led_channel_port[i], &gpio_out);
		HAL_GPIO_WritePin((GPIO_TypeDef *)tng_led_channel_port[i], tng_led_channel_pin[i], GPIO_PIN_SET);
	}
#endif
}