/* brickletboot
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tinynvm.c: sam0 NVM write/erase driver with small memory/flash footprint
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

#include "tinynvm.h"

#include "system.h"

#define TINYNVM_MEMORY ((volatile uint16_t *)FLASH_ADDR)

static inline bool tinynvm_is_ready(void) {
	return NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY;
}

void tinynvm_init(void) {
	// Wait for NVM to be ready
	while(!tinynvm_is_ready());

	// Turn on the clock
	PM->APBBMASK.reg |= PM_APBBMASK_NVMCTRL;

	// Clear error flags
	NVMCTRL->STATUS.reg = NVMCTRL_STATUS_MASK;

	// Writing configuration to the CTRLB register
	NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_SLEEPPRM(NVMCTRL_CTRLB_SLEEPPRM_WAKEONACCESS_Val) | NVMCTRL_CTRLB_RWS(NVMCTRL->CTRLB.bit.RWS) | NVMCTRL_CTRLB_READMODE(NVMCTRL_CTRLB_READMODE_NO_MISS_PENALTY_Val);

	while(!tinynvm_is_ready());
}

// Note: Compared to nvm_erase_row in asf this function assumes that
//       row_address is an address at the beginning of a row (page*4)
void tinynvm_erase_row(const uint32_t row_address) {
	while(!tinynvm_is_ready());

	// Clear error flags
	NVMCTRL->STATUS.reg = NVMCTRL_STATUS_MASK;

	// Set address and command
	// TODO: Is "/ 4" correct here? Shouldn't it be "/ 2"?
	NVMCTRL->ADDR.reg  = (uintptr_t)&TINYNVM_MEMORY[row_address / 4];
	NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_ER | NVMCTRL_CTRLA_CMDEX_KEY;

	while(!tinynvm_is_ready());
}

// Note: Compared to nvm_write_buffer in asf this function assumes that
//   * All input is well-formed
//   * Length equals to one full page size
void tinynvm_write_buffer(const uint32_t destination_address, const uint8_t *buffer, const uint16_t length) {
	// Wait for NVM to be ready
	while(!tinynvm_is_ready());

	// Erase the page buffer before buffering new data
	NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_PBC | NVMCTRL_CTRLA_CMDEX_KEY;

	// Wait for NVM to be ready
	while(!tinynvm_is_ready());

	// Clear error flags
	NVMCTRL->STATUS.reg = NVMCTRL_STATUS_MASK;

	uint32_t nvm_address = destination_address / 2;

	// NVM _must_ be accessed as a series of 16-bit words, perform manual copy
	// to ensure alignment
	for(uint16_t i = 0; i < length; i += 2) {
		// Store next 16-bit chunk to the NVM memory space
		TINYNVM_MEMORY[nvm_address++] = buffer[i] | (buffer[i + 1] << 8);
	}

	while(!tinynvm_is_ready());
}
