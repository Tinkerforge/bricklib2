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

#define TINYNVM_MEMORY    ((volatile uint16_t *)FLASH_ADDR)
#define TINYNVM_PAGE_SIZE 64

static inline bool tinynvm_is_ready(void) {
	return NVMCTRL->INTFLAG.bit.READY;
}

void tinynvm_init(void) {
	// Wait for NVM to be ready
	while(!tinynvm_is_ready());

	// Turn on the clock
	PM->AHBMASK.reg |= PM_AHBMASK_NVMCTRL;  // TODO: Is AHB unnecessary here?
	PM->APBBMASK.reg |= PM_APBBMASK_NVMCTRL;

	// Clear error flags
	NVMCTRL->STATUS.reg = NVMCTRL_STATUS_MASK;

	// TODO: Try different wait state
	// Writing configuration to the CTRLB register
	NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_SLEEPPRM(NVMCTRL_CTRLB_SLEEPPRM_WAKEONACCESS_Val) | NVMCTRL_CTRLB_RWS(NVMCTRL->CTRLB.bit.RWS) | NVMCTRL_CTRLB_READMODE(NVMCTRL_CTRLB_READMODE_NO_MISS_PENALTY_Val);
//	NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_SLEEPPRM(NVMCTRL_CTRLB_SLEEPPRM_DISABLED_Val) | NVMCTRL_CTRLB_RWS(15) | NVMCTRL_CTRLB_READMODE(NVMCTRL_CTRLB_READMODE_DETERMINISTIC_Val);

	// For manual write
#if 0
	| (0x01 << NVMCTRL_CTRLB_MANW_Pos);
#endif

	while(!tinynvm_is_ready());
}

// Note: Compared to nvm_erase_row in asf this function assumes that
//       address is an address at the beginning of a row (page*4)
void tinynvm_erase_row(const uint32_t address) {
	while(!tinynvm_is_ready());

	// Clear error flags
	NVMCTRL->STATUS.reg = NVMCTRL_STATUS_MASK;

	// Set address and command
	NVMCTRL->ADDR.reg   = address / 2;
	NVMCTRL->CTRLA.reg  = NVMCTRL_CTRLA_CMD_ER | NVMCTRL_CTRLA_CMDEX_KEY;

	while(!tinynvm_is_ready());
}

// Note: Compared to nvm_write_buffer in asf this function assumes that
//       address is an address at the beginning of a page
void tinynvm_write_page(const uint32_t address, const uint8_t *page_buffer) {
	// Wait for NVM to be ready
	while(!tinynvm_is_ready());

	// Erase the page buffer before buffering new data
	NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_PBC | NVMCTRL_CTRLA_CMDEX_KEY;

	// Wait for NVM to be ready
	while(!tinynvm_is_ready());

	// Clear error flags
	NVMCTRL->STATUS.reg = NVMCTRL_STATUS_MASK;

	// NVM memory is accessed in 16-bit steps
	uint32_t nvm_address = address / 2;

	// NVM _must_ be accessed as a series of 16-bit words, perform manual copy
	// to ensure alignment
	for(uint16_t i = 0; i < TINYNVM_PAGE_SIZE; i += 2) {
		// Store next 16-bit chunk to the NVM memory space
		TINYNVM_MEMORY[nvm_address++] = page_buffer[i] | (page_buffer[i + 1] << 8);
	}

	while(!tinynvm_is_ready());

	// For manual write
#if 0
	// Turn off cache before issuing flash commands
	const uint32_t ctrlb_backup = NVMCTRL->CTRLB.reg;
	NVMCTRL->CTRLB.reg = ctrlb_backup | NVMCTRL_CTRLB_CACHEDIS;

	// Clear error flags
	NVMCTRL->STATUS.reg = NVMCTRL_STATUS_MASK;

	while(!tinynvm_is_ready());

	// Set address, command will be issued elsewhere
	NVMCTRL->ADDR.reg = destination_address / 2;

	// Set command
	NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_WP | NVMCTRL_CTRLA_CMDEX_KEY;

	// Wait for NVM to be ready again
	while (!tinynvm_is_ready())

	// Restore the setting
	NVMCTRL->CTRLB.reg = ctrlb_backup;
#endif
}
