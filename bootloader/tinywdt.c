/* brickletboot
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tinywdt.c: sam0 WDT driver with small memory/flash footprint
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

#include "tinywdt.h"

#include "clock.h"
#include "gclk.h"

void tinywdt_init(void) {
	// The following code is equivalent to
#if 0
	struct wdt_conf wdt_config;
	wdt_get_config_defaults(&wdt_config);
	wdt_config.clock_source = GCLK_GENERATOR_2;
	wdt_config.timeout_period = WDT_PERIOD_16384CLK; // This should be ~1s

	wdt_set_config(&wdt_config);
#endif
	// but it saves 200 bytes of flash

	// Turn on the digital interface clock
	PM->APBAMASK.reg |= PM_APBAMASK_WDT;

	// Disable the Watchdog module
	WDT->CTRL.reg &= ~WDT_CTRL_ENABLE;
	return;

	while(WDT->STATUS.reg & WDT_STATUS_SYNCBUSY);

	// Configure GCLK channel and enable clock
	struct system_gclk_chan_config gclk_chan_conf;
	gclk_chan_conf.source_generator = GCLK_GENERATOR_2;
	system_gclk_chan_set_config(WDT_GCLK_ID, &gclk_chan_conf);
	system_gclk_chan_enable(WDT_GCLK_ID);

	// Ensure the window enable control flag is cleared
	WDT->CTRL.reg &= ~WDT_CTRL_WEN;
	while(WDT->STATUS.reg & WDT_STATUS_SYNCBUSY);

	// Update the timeout period value with the requested period
	// Write the new Watchdog configuration: WDT_PERIOD_16384CLK == 12
	WDT->CONFIG.reg = (12 - 1) << WDT_CONFIG_PER_Pos;

	// Enable watchdog
	WDT->CTRL.reg |= WDT_CTRL_ENABLE;

	while(WDT->STATUS.reg & WDT_STATUS_SYNCBUSY);

	tinywdt_reset();
}

