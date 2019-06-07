/* bricklib2
 * Copyright (C) 2017 Olaf Lüke <olaf@tinkerforge.com>
 *
 * coop_task.c: Minimal cooperative (non-preemptive) scheduler
 *              for cortex-m MCUs
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
**/

#include "coop_task.h"

#if defined(STM32F0)
#include "bricklib2/stm32cubef0/Drivers/CMSIS/Include/core_cm0.h"
#else
#include "bricklib2/xmclib/CMSIS/Include/core_cm0.h"
#endif

#include "bricklib2/hal/system_timer/system_timer.h"

#include <string.h>

// Use uint32_t for coop_task_main (we only need to save the stack pointer for main task.
// All other tasks are of type CoopTaskStack and they contain the stack, the
// stack frame as well as the stack pointer.
// In CoopTaskStack the stack pointer is always at the first position (so it is compatible)
// to the main task uint32_t. From there the stack and stack frame is 8-byte aligned
// packed directly below the stack pointer.
// This may seem a bit like a hack, but it works well and makes debugging a lot easier,
// since all of the stack, stack frame and pointer of all tasks are always at the
// same offset to each other.
volatile uint32_t coop_task_main = 0;
volatile void *coop_task_current = &coop_task_main;
volatile void *coop_task_next = NULL;

static void coop_task_default_return_from_task(void) {
	while(true);
}

// This function can not be called from "main task"
void coop_task_sleep_ms(const uint32_t sleep) {
	const uint32_t time = system_timer_get_ms();
	while(!system_timer_is_time_elapsed_ms(time, sleep)) {
		coop_task_yield();
	}
}

// This function can not be called from "main task"
void coop_task_yield(void) {
	coop_task_current = coop_task_next;
	coop_task_next = &coop_task_main;

	// Trigger PendSV handler
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
}

// This function can only be called from main task
void coop_task_tick(CoopTask *task) {
#ifdef COOP_TASK_DEBUG_STACK_LOW_WATERMARK
	// Calculate currently available stack and save it as low watermark if it is
	// lower then previous low watermark.
	uint32_t stack_free = (uint32_t)task->stack.stack_pointer - (uint32_t)task->stack.stack;
	if(stack_free < task->stack_low_watermark) {
		task->stack_low_watermark = stack_free;
	}
#endif

	coop_task_current = &coop_task_main;
	coop_task_next = task;

	// Trigger PendSV handler
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
}

void coop_task_init(CoopTask *task, CoopTaskFunction function) {
	task->function = function;
	memset(&task->stack, COOP_TASK_STACK_FILL, sizeof(CoopTaskStack));

	task->stack.stack_pointer = (uint32_t)(task->stack.stack + COOP_TASK_STACK_SIZE);

	task->stack.stack_frame.nvic_frame.r0  = 0xfffffff0;
	task->stack.stack_frame.nvic_frame.r1  = 0xfffffff1;
	task->stack.stack_frame.nvic_frame.r2  = 0xfffffff2;
	task->stack.stack_frame.nvic_frame.r3  = 0xfffffff3;
	task->stack.stack_frame.sw_frame.r4    = 0xfffffff4;
	task->stack.stack_frame.sw_frame.r5    = 0xfffffff5;
	task->stack.stack_frame.sw_frame.r6    = 0xfffffff6;
	task->stack.stack_frame.sw_frame.r7    = 0xfffffff7;
	task->stack.stack_frame.sw_frame.r8    = 0xfffffff8;
	task->stack.stack_frame.sw_frame.r9    = 0xfffffff9;
	task->stack.stack_frame.sw_frame.r10   = 0xfffffffa;
	task->stack.stack_frame.sw_frame.r11   = 0xfffffffb;
	task->stack.stack_frame.nvic_frame.r12 = 0xfffffffc;
	task->stack.stack_frame.nvic_frame.lr  = coop_task_default_return_from_task;
	task->stack.stack_frame.nvic_frame.pc  = task->function;
	task->stack.stack_frame.nvic_frame.psr = 0x21000000; // Everybody uses 0x21000000 here, seems to be the default

#ifdef COOP_TASK_DEBUG_STACK_LOW_WATERMARK
	task->stack_low_watermark = COOP_TASK_STACK_SIZE;
#endif
}
