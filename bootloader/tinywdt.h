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

#ifndef TINYWDT_H
#define TINYWDT_H

#include "configs/config.h"

inline void tinywdt_reset(void) {
	WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
	// Do we need to wait for sync busy here?
//	while(WDT->STATUS.reg & WDT_STATUS_SYNCBUSY);
}

void tinywdt_init(void);

#endif
