/* TNG
 * Copyright (C) 2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * usb_conf.c: TNG system STM32 USB configuration (for CubeMX API)
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

#include "usbd_conf.h"

#include "bricklib2/logging/logging.h"

#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"
#include "usbd_def.h"
#include "usbd_core.h"
#include "usbd_tfp.h"

PCD_HandleTypeDef usbd_pcd;

void USB_IRQHandler(void) {
	HAL_PCD_IRQHandler(&usbd_pcd);
}

static USBD_StatusTypeDef USBD_Get_USB_Status(HAL_StatusTypeDef hal_status) {
	USBD_StatusTypeDef usb_status = USBD_OK;

	switch (hal_status)	{
		case HAL_OK:      usb_status = USBD_OK;   break;
		case HAL_ERROR:   usb_status = USBD_FAIL; break;
		case HAL_BUSY:    usb_status = USBD_BUSY; break;
		case HAL_TIMEOUT: usb_status = USBD_FAIL; break;
		default:          usb_status = USBD_FAIL; break;
	}

	return usb_status;
}

// --- LL Driver Callbacks (PCD -> USB Device Library) ---

void HAL_PCD_MspInit(PCD_HandleTypeDef *pcd) {
	if(pcd->Instance == USB) {
		// Peripheral clock enable
		__HAL_RCC_USB_CLK_ENABLE();

		// Peripheral interrupt init
		NVIC_SetPriority(USB_IRQn, 0);
		NVIC_EnableIRQ(USB_IRQn);
	}
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *pcd) {
	if(pcd->Instance == USB) {
		// Peripheral clock disable
		__HAL_RCC_USB_CLK_DISABLE();

		// Peripheral interrupt Deinit
		NVIC_DisableIRQ(USB_IRQn);
	}
}


void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *pcd) {
	USBD_LL_SetupStage((USBD_HandleTypeDef*)pcd->pData, (uint8_t *)pcd->Setup);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *pcd, uint8_t epnum) {
	USBD_LL_DataOutStage((USBD_HandleTypeDef*)pcd->pData, epnum, pcd->OUT_ep[epnum].xfer_buff);
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *pcd, uint8_t epnum) {
	USBD_LL_DataInStage((USBD_HandleTypeDef*)pcd->pData, epnum, pcd->IN_ep[epnum].xfer_buff);
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *pcd) {
	USBD_LL_SOF((USBD_HandleTypeDef*)pcd->pData);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *pcd) { 
	USBD_SpeedTypeDef speed = USBD_SPEED_FULL;

	if(pcd->Init.speed != PCD_SPEED_FULL) {
		loge("Unrecognized speed: %d\n\r", pcd->Init.speed);
	}

	USBD_LL_SetSpeed((USBD_HandleTypeDef*)pcd->pData, speed);
	USBD_LL_Reset((USBD_HandleTypeDef*)pcd->pData);
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *pcd) {
	// TODO: Ignore suspend?
#if 0
	// Inform USB library that core enters in suspend Mode
	USBD_LL_Suspend((USBD_HandleTypeDef*)pcd->pData);
	if (pcd->Init.low_power_enable)
	{
		// Set SLEEPDEEP bit and SleepOnExit of Cortex System Control Register.
		SCB->SCR |= (uint32_t)((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));
	}
#endif
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *pcd) {
	// TODO: Ignore resume?
#if 0
	if (pcd->Init.low_power_enable) {
		// Reset SLEEPDEEP bit of Cortex System Control Register.
		SCB->SCR &= (uint32_t)~((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));
		SystemClockConfig_Resume();
	}
	USBD_LL_Resume((USBD_HandleTypeDef*)pcd->pData);
#endif
}

void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *pcd, uint8_t epnum) {
	USBD_LL_IsoOUTIncomplete((USBD_HandleTypeDef*)pcd->pData, epnum);
}

void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *pcd, uint8_t epnum) {
	USBD_LL_IsoINIncomplete((USBD_HandleTypeDef*)pcd->pData, epnum);
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *pcd) {
	USBD_LL_DevConnected((USBD_HandleTypeDef*)pcd->pData);
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *pcd) {
	USBD_LL_DevDisconnected((USBD_HandleTypeDef*)pcd->pData);
}

// --- LL Driver Interface (USB Device Library --> PCD) ---

USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *dev) {
	// Link the driver to the stack
	usbd_pcd.pData = dev;
	dev->pData = &usbd_pcd;

	usbd_pcd.Instance = USB;
	usbd_pcd.Init.dev_endpoints = 8;
	usbd_pcd.Init.speed = PCD_SPEED_FULL;
	usbd_pcd.Init.phy_itface = PCD_PHY_EMBEDDED;
	usbd_pcd.Init.low_power_enable = DISABLE;
	usbd_pcd.Init.lpm_enable = DISABLE;
	usbd_pcd.Init.battery_charging_enable = DISABLE;
	HAL_StatusTypeDef status = HAL_PCD_Init(&usbd_pcd);
	if(status != HAL_OK) {
		loge("HAL_PCD_Init error: %d\n\r", status);
	}

	// TODO: vvvvv ???
	// PCD_SNG_BUF = single buffer, why not PCD_DBL_BUF?

	// Endpoint DMA and buffer configuration
	HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*)dev->pData, 0x00, PCD_SNG_BUF, 0x40); // <- ?
	HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*)dev->pData, 0x80, PCD_SNG_BUF, 0x80); // <- ?
	HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*)dev->pData, USBD_TFP_IN_EP, PCD_SNG_BUF, 0xC0);
	HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*)dev->pData, USBD_TFP_OUT_EP, PCD_SNG_BUF, 0x110);

	return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev) {
	HAL_StatusTypeDef hal_status = HAL_PCD_DeInit(pdev->pData);
	return USBD_Get_USB_Status(hal_status);
}

USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev) {
	HAL_StatusTypeDef hal_status = HAL_PCD_Start(pdev->pData);
	return USBD_Get_USB_Status(hal_status);
}

USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev) {
	HAL_StatusTypeDef hal_status = HAL_PCD_Stop(pdev->pData);

	return USBD_Get_USB_Status(hal_status);
}

USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t ep_type, uint16_t ep_mps) {
	HAL_StatusTypeDef hal_status = HAL_PCD_EP_Open(pdev->pData, ep_addr, ep_mps, ep_type);
	return USBD_Get_USB_Status(hal_status);
}

USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
	HAL_StatusTypeDef hal_status = HAL_PCD_EP_Close(pdev->pData, ep_addr);
	return USBD_Get_USB_Status(hal_status);
}

USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
	HAL_StatusTypeDef hal_status = HAL_PCD_EP_Flush(pdev->pData, ep_addr);
	return USBD_Get_USB_Status(hal_status);
}

USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
	HAL_StatusTypeDef hal_status = HAL_PCD_EP_SetStall(pdev->pData, ep_addr);
	return USBD_Get_USB_Status(hal_status);
}

USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
	HAL_StatusTypeDef hal_status = HAL_PCD_EP_ClrStall(pdev->pData, ep_addr);  
	return USBD_Get_USB_Status(hal_status);
}

uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
	PCD_HandleTypeDef *pcd = (PCD_HandleTypeDef*) pdev->pData;
	
	if((ep_addr & 0x80) == 0x80) {
		return pcd->IN_ep[ep_addr & 0x7F].is_stall; 
	} else {
		return pcd->OUT_ep[ep_addr & 0x7F].is_stall; 
	}
}

USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t dev_addr) {
	HAL_StatusTypeDef hal_status = HAL_PCD_SetAddress(pdev->pData, dev_addr);
	return USBD_Get_USB_Status(hal_status);
}

USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint16_t size) {
	HAL_StatusTypeDef hal_status = HAL_PCD_EP_Transmit(pdev->pData, ep_addr, pbuf, size);
	return USBD_Get_USB_Status(hal_status);
}

USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint16_t size) {
	HAL_StatusTypeDef hal_status = HAL_PCD_EP_Receive(pdev->pData, ep_addr, pbuf, size);
	return  USBD_Get_USB_Status(hal_status);
}

uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
	return HAL_PCD_EP_GetRxCount((PCD_HandleTypeDef*)pdev->pData, ep_addr);
}

