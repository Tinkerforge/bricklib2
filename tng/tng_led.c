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
    // TODO: PWM

    HAL_GPIO_WritePin((GPIO_TypeDef *)tng_led_status_port[TNG_LED_STATUS_R], tng_led_status_pin[TNG_LED_STATUS_R], r == 255 ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin((GPIO_TypeDef *)tng_led_status_port[TNG_LED_STATUS_G], tng_led_status_pin[TNG_LED_STATUS_G], g == 255 ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin((GPIO_TypeDef *)tng_led_status_port[TNG_LED_STATUS_B], tng_led_status_pin[TNG_LED_STATUS_B], b == 255 ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
#endif

#ifdef TNG_LED_CHANNEL_0_PIN
void tng_led_channel_set(const TNGLEDChannel channel, const bool enable) {
    if(channel < TNG_LED_CHANNEL_NUM) {
	    HAL_GPIO_WritePin((GPIO_TypeDef *)tng_led_channel_port[channel], tng_led_channel_pin[channel], enable ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
}
#endif

void tng_led_tick(void) {

}

void tng_led_init(void) {
#if defined(TNG_LED_STATUS_R_PIN) || defined(TNG_LED_CHANNEL_0_PIN)
    // TODO: Only enable necessary GPIO ports
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_InitTypeDef gpio_out = {
		.Mode      = GPIO_MODE_OUTPUT_PP,
		.Pull      = GPIO_NOPULL,
		.Speed     = GPIO_SPEED_FREQ_LOW,
	};
#endif

#ifdef TNG_LED_STATUS_R_PIN
    for(uint8_t i = 0; i < TNG_LED_STATUS_NUM; i++) {
        gpio_out.Pin = tng_led_status_pin[i];
	    HAL_GPIO_Init((GPIO_TypeDef *)tng_led_status_port[i], &gpio_out);
	    HAL_GPIO_WritePin((GPIO_TypeDef *)tng_led_status_port[i], tng_led_status_pin[i], GPIO_PIN_SET);
    }
#endif

#ifdef TNG_LED_CHANNEL_0_PIN
    for(uint8_t i = 0; i < TNG_LED_CHANNEL_NUM; i++) {
        gpio_out.Pin = tng_led_channel_pin[i];
	    HAL_GPIO_Init((GPIO_TypeDef *)tng_led_channel_port[i], &gpio_out);
	    HAL_GPIO_WritePin((GPIO_TypeDef *)tng_led_channel_port[i], tng_led_channel_pin[i], GPIO_PIN_SET);
    }
#endif
}