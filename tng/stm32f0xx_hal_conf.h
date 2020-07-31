/* tng-stm32
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * stm32f0xx_hal_conf.h: Default config included by stm32cubef0
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

#include "configs/config.h"

#ifndef STM32F0XX_HAL_CONF_H
#define STM32F0XX_HAL_CONF_H

// HAL Drivers (used be default)
#define HAL_MODULE_ENABLED  
#define HAL_SPI_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_SMBUS_MODULE_ENABLED
#define HAL_WWDG_MODULE_ENABLED
#define HAL_PCD_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED

// HAL Drivers (not used be default)
// #define HAL_ADC_MODULE_ENABLED
// #define HAL_CRYP_MODULE_ENABLED
// #define HAL_CAN_MODULE_ENABLED
// #define HAL_CEC_MODULE_ENABLED
// #define HAL_COMP_MODULE_ENABLED
// #define HAL_CRC_MODULE_ENABLED
// #define HAL_CRYP_MODULE_ENABLED
// #define HAL_TSC_MODULE_ENABLED 
// #define HAL_DAC_MODULE_ENABLED
// #define HAL_I2S_MODULE_ENABLED
// #define HAL_IWDG_MODULE_ENABLED
// #define HAL_LCD_MODULE_ENABLED
// #define HAL_LPTIM_MODULE_ENABLED
// #define HAL_RNG_MODULE_ENABLED
// #define HAL_RTC_MODULE_ENABLED
// #define HAL_UART_MODULE_ENABLED
// #define HAL_USART_MODULE_ENABLED
// #define HAL_IRDA_MODULE_ENABLED
// #define HAL_SMARTCARD_MODULE_ENABLED

// Default HSE/HSI values (can be overwritten through configs/config.h)
#if !defined  (HSE_VALUE) 
 #define HSE_VALUE             ((uint32_t)8000000)  // Value of the External oscillator in Hz
#endif

#if !defined  (HSE_STARTUP_TIMEOUT)
 #define HSE_STARTUP_TIMEOUT   ((uint32_t)100)      // Time out for HSE start up, in ms
#endif

#if !defined  (HSI_VALUE)
 #define HSI_VALUE             ((uint32_t)8000000)  // Value of the Internal oscillator in Hz
#endif

#if !defined  (HSI_STARTUP_TIMEOUT) 
 #define HSI_STARTUP_TIMEOUT   ((uint32_t)5000)     // Time out for HSI start up
#endif  

#if !defined  (HSI14_VALUE) 
#define HSI14_VALUE            ((uint32_t)14000000) // Value of the Internal High Speed oscillator for ADC in Hz.
#endif                                              // The real value may vary depending on the variations in voltage and temperature.

#if !defined  (HSI48_VALUE) 
#define HSI48_VALUE            ((uint32_t)48000000) // Value of the Internal High Speed oscillator for USB in Hz.
#endif                                              // The real value may vary depending on the variations in voltage and temperature.

#if !defined  (LSI_VALUE) 
 #define LSI_VALUE             ((uint32_t)40000)    // Value of the Internal Low Speed oscillator in Hz
#endif                                              // The real value may vary depending on the variations in voltage and temperature.

#if !defined  (LSE_VALUE)
 #define LSE_VALUE             ((uint32_t)32768)    // Value of the External Low Speed oscillator in Hz
#endif    

#if !defined  (LSE_STARTUP_TIMEOUT)
 #define LSE_STARTUP_TIMEOUT   ((uint32_t)5000)     // Time out for LSE start up, in ms
#endif


// System Configuration
#define VDD_VALUE                 ((uint32_t)3300)  // Value of VDD in mV
#define TICK_INT_PRIORITY         ((uint32_t)0)     // tick interrupt priority (lowest by default)

#define USE_RTOS                  0
#define PREFETCH_ENABLE           1
#define INSTRUCTION_CACHE_ENABLE  0
#define DATA_CACHE_ENABLE         0

// SPI peripheral configuration
#define USE_SPI_CRC               0

// Includes
#ifdef HAL_RCC_MODULE_ENABLED
 #include "stm32f0xx_hal_rcc.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
 #include "stm32f0xx_hal_gpio.h"
#endif

#ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32f0xx_hal_dma.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
 #include "stm32f0xx_hal_cortex.h"
#endif

#ifdef HAL_ADC_MODULE_ENABLED
 #include "stm32f0xx_hal_adc.h"
#endif

#ifdef HAL_CAN_MODULE_ENABLED
 #include "stm32f0xx_hal_can.h"
#endif

#ifdef HAL_CEC_MODULE_ENABLED
 #include "stm32f0xx_hal_cec.h"
#endif

#ifdef HAL_COMP_MODULE_ENABLED
 #include "stm32f0xx_hal_comp.h"
#endif

#ifdef HAL_CRC_MODULE_ENABLED
 #include "stm32f0xx_hal_crc.h"
#endif

#ifdef HAL_DAC_MODULE_ENABLED
 #include "stm32f0xx_hal_dac.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
 #include "stm32f0xx_hal_flash.h"
#endif 

#ifdef HAL_I2C_MODULE_ENABLED
 #include "stm32f0xx_hal_i2c.h"
#endif

#ifdef HAL_I2S_MODULE_ENABLED
 #include "stm32f0xx_hal_i2s.h"
#endif

#ifdef HAL_IRDA_MODULE_ENABLED
 #include "stm32f0xx_hal_irda.h"
#endif

#ifdef HAL_IWDG_MODULE_ENABLED
 #include "stm32f0xx_hal_iwdg.h"
#endif

#ifdef HAL_PCD_MODULE_ENABLED
 #include "stm32f0xx_hal_pcd.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
 #include "stm32f0xx_hal_pwr.h"
#endif

#ifdef HAL_RTC_MODULE_ENABLED
 #include "stm32f0xx_hal_rtc.h"
#endif

#ifdef HAL_SMARTCARD_MODULE_ENABLED
 #include "stm32f0xx_hal_smartcard.h"
#endif

#ifdef HAL_SMBUS_MODULE_ENABLED
 #include "stm32f0xx_hal_smbus.h"
#endif

#ifdef HAL_SPI_MODULE_ENABLED
 #include "stm32f0xx_hal_spi.h"
#endif

#ifdef HAL_TIM_MODULE_ENABLED
 #include "stm32f0xx_hal_tim.h"
#endif

#ifdef HAL_TSC_MODULE_ENABLED
 #include "stm32f0xx_hal_tsc.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
 #include "stm32f0xx_hal_uart.h"
#endif

#ifdef HAL_USART_MODULE_ENABLED
 #include "stm32f0xx_hal_usart.h"
#endif

#ifdef HAL_WWDG_MODULE_ENABLED
 #include "stm32f0xx_hal_wwdg.h"
#endif

// Assert
#ifdef  USE_FULL_ASSERT
 #define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
 void assert_failed(uint8_t* file, uint32_t line);
#else
 #define assert_param(expr) ((void)0U)
#endif

#endif 