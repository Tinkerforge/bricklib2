.syntax unified
.cpu cortex-m0
.fpu softvfp

.thumb

.global PendSV_Handler
.type PendSV_Handler, %function
PendSV_Handler:
	cpsid	i // Disable interrupts

    /********************************************************************
	Exception frame saved by the NVIC to the stack:
	+------+
	|      | <- SP before interrupt (orig. SP)
	| xPSR |
	|  PC  |
	|  LR  |
	|  R12 |
	|  R3  |
	|  R2  |
	|  R1  |
	|  R0  | <- SP after entering interrupt (orig. SP + 32 bytes)
	+------+

	Registers saved here (PendSV_Handler):
	+------+
	|  R7  |
	|  R6  |
	|  R5  |
	|  R4  |
	|  R11 |
	|  R10 |
	|  R9  |
	|  R8  | <- Saved SP after PendSV_Handler (orig. SP + 64 bytes)
	+------+
    ********************************************************************/

	mov	r0, sp              // Save SP
	subs	r0, #16         // Subtract 16 bytes from SP to get to start of first 4 words
	stmia	r0!,{r4-r7}     // Use STMIA to save first 4 registers (in cortex m0 it can only save r0-r7 low-registers)
	mov	r4, r8              // Copy the 4 registers to r8-r11
	mov	r5, r9              //
	mov	r6, r10             //
	mov	r7, r11             //
	subs	r0, #32         // Subtract 32 bytes to get to start of next 4 words)
	stmia	r0!,{r4-r7}     // Now STMIA copies the correct 4 bytes (r4-r7)
	subs	r0, #16         // Subtract 16 bytes to get to new start of stack frame

	// Save stack pointer to current task handle
	ldr	r2, =coop_task_current
	ldr	r1, [r2]
	str	r0, [r1]

	// Load stack pointer from next task handle
	ldr	r2, =coop_task_next
	ldr	r1, [r2]
	ldr	r0, [r1]

	ldmia	r0!,{r4-r7}     // Use LDMIA to load first 4 registers
	mov	r8, r4              // Copy the 4 registers to the correct positions r8-r11
	mov	r9, r5              //
	mov	r10, r6             //
	mov	r11, r7             //
	ldmia	r0!,{r4-r7}     // Use LDMIA to load the next 4 registers
	mov	sp, r0              // r0 is now the end of the stack, copy it the the sp register

	ldr r0, =0xFFFFFFF9     // EXC_RETURN = Thread mode with MSP
	cpsie	i               // Enable interrupts

	bx r0                   // Jump out of PendSV_Handler

.size PendSV_Handler, .-PendSV_Handler
