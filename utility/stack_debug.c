/* bricklib2
 * Copyright (C) 2018 Olaf Lüke <olaf@tinkerforge.com>
 *
 * stack_debug.c: Stack debug helper functions
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

#include "stack_debug.h"

#include "configs/config.h"

static uint32_t stack_debug_low_watermark = 0xFFFF;
extern uint32_t __stack_debug_end; // End of stack (defined in linker script)

void stack_debug_update(void) {
    uint32_t stack_free = __get_MSP() - (uint32_t)&__stack_debug_end;
    if(stack_free < stack_debug_low_watermark) {
		stack_debug_low_watermark = stack_free;
	}
}

uint32_t stack_debug_get_low_watermark(void) {
    return stack_debug_low_watermark;
}