/* bricklib2 warp
 * Copyright (C) 2023 Olaf Lüke <olaf@tinkerforge.com>
 *
 * timer.h: Timer handling for RS485 in WARP products
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

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>

#include "configs/config_timer.h"

#define TIMER_RESET() \
	do {\
		TIMER_CCU_CC41->TCCLR = CCU4_CC4_TCCLR_TCC_Msk;\
		TIMER_CCU_CC41->TCSET = CCU4_CC4_TCSET_TRBS_Msk;\
	} while(false)

#if 0
	#define TIMER_RESET() \
		do {\
			TIMER_CCU_CC41->SWS = CCU4_CC4_SWS_SE0A_Msk;\
		} while(false)
#endif

bool timer_us_elapsed_since_last_timer_reset(const uint32_t us);
void timer_init(void);
void timer_tick(void);

#endif
