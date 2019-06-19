/* Link Script
 * Copyright (C) 2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * stm32f0.ld: Linker Script for TNG modules with STM32F0. 
 *             Based on original STM32F072RBTx_FLASH.ld
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

/*OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
*/
/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = 0x20004000;    /* end of RAM */

/* Specify the memory areas */
MEMORY
{
	RAM (xrw)      : ORIGIN = 0x200000C4, LENGTH = 0x00003F3C
	FLASH (rx)     : ORIGIN =  0x8002000, LENGTH = 0x0000f000
}

/* Define output sections */
SECTIONS
{
    /* Add constants to start of firmware */
    /* firmware_crc: ORIGIN(FLASH) + 0 is done externaly */
    /* firmware_length: ORIGIN(FLASH) + 4 is done externaly */

    .firmware_crc ORIGIN(FLASH) + 0:
  	{
    	KEEP(*(.firmware_crc))
  	} > FLASH 

    .firmware_length ORIGIN(FLASH) + 4:
  	{
    	KEEP(*(.firmware_length))
  	} > FLASH 

    .magic_number ORIGIN(FLASH) + 8:
  	{
    	KEEP(*(.magic_number))
  	} > FLASH 
    
    .device_identifier ORIGIN(FLASH) + 12:
  	{
    	KEEP(*(.device_identifier))
  	} > FLASH

	/* The startup code goes first into FLASH */
	.isr_vector :
	{
		. = ALIGN(4);
		KEEP(*(.isr_vector)) /* Startup code */
		. = ALIGN(4);
	} >FLASH

	/* The program code and other data goes into FLASH */
	.text :
	{
		. = ALIGN(4);
		*(.text)           /* .text sections (code) */
		*(.text*)          /* .text* sections (code) */
		*(.glue_7)         /* glue arm to thumb code */
		*(.glue_7t)        /* glue thumb to arm code */
		*(.eh_frame)

		KEEP (*(.init))
		KEEP (*(.fini))

		. = ALIGN(4);
		_etext = .;        /* define a global symbols at end of code */
	} >FLASH

	/* Constant data goes into FLASH */
	.rodata :
	{
		. = ALIGN(4);
		*(.rodata)         /* .rodata sections (constants, strings, etc.) */
		*(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
		. = ALIGN(4);
	} >FLASH

	.ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
	.ARM : {
		__exidx_start = .;
		*(.ARM.exidx*)
		__exidx_end = .;
	} >FLASH

	.preinit_array     :
	{
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(.preinit_array*))
		PROVIDE_HIDDEN (__preinit_array_end = .);
	} >FLASH
	.init_array :
	{
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array*))
		PROVIDE_HIDDEN (__init_array_end = .);
	} >FLASH
	.fini_array :
	{
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP (*(SORT(.fini_array.*)))
		KEEP (*(.fini_array*))
		PROVIDE_HIDDEN (__fini_array_end = .);
	} >FLASH

    
	/* used by the startup to initialize data */
	_sidata = LOADADDR(.data);

	/* Initialized data sections goes into RAM, load LMA copy after code */
	.data : 
	{
		. = ALIGN(4);
		_sdata = .;        /* create a global symbol at data start */
		*(.data)           /* .data sections */
		*(.data*)          /* .data* sections */

		. = ALIGN(4);
		_edata = .;        /* define a global symbol at data end */
	} >RAM AT> FLASH
	
	/* Uninitialized data section */
	. = ALIGN(4);
	.bss :
	{
		/* This is used by the startup in order to initialize the .bss secion */
		_sbss = .;         /* define a global symbol at bss start */
		__bss_start__ = _sbss;
		*(.bss)
		*(.bss*)
		*(COMMON)

		. = ALIGN(4);
		_ebss = .;         /* define a global symbol at bss end */
		__bss_end__ = _ebss;
	} >RAM


	/* Remove information from the standard libraries */
	/DISCARD/ :
	{
		libc.a ( * )
		libm.a ( * )
		libgcc.a ( * )
	}

	.ARM.attributes 0 : { *(.ARM.attributes) }
}
