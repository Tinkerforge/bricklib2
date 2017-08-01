/* bricklib2
 * Copyright (C) 2017 Olaf Lüke <olaf@tinkerforge.com>
 *
 * coop_task.h: Minimal cooperative (non-preemptive) scheduler
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

#ifndef COOP_TASK_H
#define COOP_TASK_H

#include "configs/config.h"

// Use a changeable stack fill, good for debugging!
#ifndef COOP_TASK_STACK_FILL
#define COOP_TASK_STACK_FILL 0
#endif

#ifndef COOP_TASK_STACK_SIZE
#define COOP_TASK_STACK_SIZE 2048
#endif

typedef struct {
	// Registers that the PendSV handler pushes on the stack,
	// after the NVIC has pushed the registers below
	struct {
		uint32_t r8;
		uint32_t r9;
		uint32_t r10;
		uint32_t r11;
		uint32_t r4;
		uint32_t r5;
		uint32_t r6;
		uint32_t r7;
	} sw_frame;

	// Registers pushed on the stack by NVIC (hardware),
	// before the other registers defined above.
	struct {
		uint32_t r0;
		uint32_t r1;
		uint32_t r2;
		uint32_t r3;
		uint32_t r12;
		void *lr;
		void *pc;
		uint32_t psr;
	} nvic_frame;
} __attribute__((packed)) CoopTaskStackFrame;

// Stack on cortex-m MCUs has to be 8-byte aligned
typedef struct {
	volatile uint32_t stack_pointer;
	uint8_t __attribute__((aligned(8))) stack[COOP_TASK_STACK_SIZE];
	CoopTaskStackFrame stack_frame;
} __attribute__((packed)) CoopTaskStack;


typedef void (*CoopTaskFunction)(void) ;

typedef struct {
	CoopTaskStack stack;
	CoopTaskFunction function;
} CoopTask;

void coop_task_sleep_ms(const uint32_t sleep);
void coop_task_yield(void);
void coop_task_init(CoopTask *task, CoopTaskFunction function);
void coop_task_tick(CoopTask *task);

#endif
