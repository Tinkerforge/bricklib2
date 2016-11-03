/* Link Script
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * brickletboot.ld: Linker Script for Bricklets wit bootloader. 
 *                  Based on original Infinion XMC1100x0016.ld
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

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(Reset_Handler)

MEMORY
{
    FLASH(RX) : ORIGIN = 0x10003000, LENGTH = 0x2000
    SRAM(!RX) : ORIGIN = 0x20000000, LENGTH = 0x4000
}

/* We use all available ram for stack, stack size does not have to be defined */
/* stack_size = DEFINED(stack_size) ? stack_size : 1024; */

SECTIONS
{
    /* TEXT section */

    .text : 
    {
      sText = .;
      KEEP(*(.reset));
      *(.text .text.* .gnu.linkonce.t.*);

      /* C++ Support */
      KEEP(*(.init))
      KEEP(*(.fini))

      /* .ctors */
      *crtbegin.o(.ctors)
      *crtbegin?.o(.ctors)
      *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
      *(SORT(.ctors.*))
      *(.ctors)

      /* .dtors */
      *crtbegin.o(.dtors)
      *crtbegin?.o(.dtors)
      *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
      *(SORT(.dtors.*))
      *(.dtors)

      *(.rodata .rodata.*)
      *(.gnu.linkonce.r*)
    } > FLASH

    .eh_frame_hdr : ALIGN (4)
    {
      KEEP (*(.eh_frame_hdr))
    } > FLASH

    .eh_frame : ALIGN (4)
    {
      KEEP (*(.eh_frame))
    } > FLASH

    /* Exception handling, exidx needs a dedicated section */
    .ARM.extab : ALIGN(4)
    {
      *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    . = ALIGN(4);
    __exidx_start = .;
    .ARM.exidx : ALIGN(4)
    {
      *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;
    . = ALIGN(4);

    /* End of RO-DATA and start of LOAD region for the veneers */
    eROData = . ;

    .firmware_version ORIGIN(FLASH) + LENGTH(FLASH) - 12 :
  	{
    	KEEP(*(.bootloader_version))
  	} > FLASH 
    
    .device_identifier ORIGIN(FLASH) + LENGTH(FLASH) - 8 :
  	{
    	KEEP(*(.device_identifier))
  	} > FLASH
    
    /* firmware_crc: ORIGIN(FLASH) + LENGTH(FLASH) - 4 is done externaly */

    /* DSRAM layout (Lowest to highest)*/
    /* Veneer <-> DATA <-> BSS <-> Stack <->  HEAP */

    .VENEER_Code ABSOLUTE(0x2000000C): AT(eROData)
    {
        . = ALIGN(4); /* section size must be multiply of 4. See startup.S file */
        VeneerStart = .;
        KEEP(*(.XmcVeneerCode));
        . = ALIGN(4); /* section size must be multiply of 4. See startup.S file */
        VeneerEnd = .;
    } > SRAM

    VeneerSize = ABSOLUTE(VeneerEnd) - ABSOLUTE(VeneerStart);


     /* Standard DATA and user defined DATA/BSS/CONST sections */
    DataLoadAddr = eROData + VeneerSize;
    .data : AT(DataLoadAddr)
    {
      . = ALIGN(4); /* section size must be multiply of 4. See startup.S file */
      __data_start = .;
      *(vtable)        
      * (.data);
      * (.data*);
      *(*.data);
      *(.gnu.linkonce.d*)
      
      . = ALIGN(4);
      /* preinit data */
      PROVIDE_HIDDEN (__preinit_array_start = .);
      KEEP(*(.preinit_array))
      PROVIDE_HIDDEN (__preinit_array_end = .);

      . = ALIGN(4);
      /* init data */
      PROVIDE_HIDDEN (__init_array_start = .);
      KEEP(*(SORT(.init_array.*)))
      KEEP(*(.init_array))
      PROVIDE_HIDDEN (__init_array_end = .);


      . = ALIGN(4);
      /* finit data */
      PROVIDE_HIDDEN (__fini_array_start = .);
      KEEP(*(SORT(.fini_array.*)))
      KEEP(*(.fini_array))
      PROVIDE_HIDDEN (__fini_array_end = .);

      . = ALIGN(4); /* section size must be multiply of 4. See startup.S file */
      __data_end = .;
    } > SRAM
    __data_size = __data_end - __data_start;

    .ram_code : AT(DataLoadAddr + __data_size)
    {
        . = ALIGN(4); /* section size must be multiply of 4. See startup.S file */
        __ram_code_start = .;
        /* functions with __attribute__ ((section (".ram_code")))*/
        *(.ram_code)   
        . = ALIGN(4); /* section size must be multiply of 4. See startup.S file */
        __ram_code_end = .;
    } > SRAM
    __ram_code_load = LOADADDR (.ram_code);
    __ram_code_size = __ram_code_end - __ram_code_start;
    
    /* BSS section */
    .bss (NOLOAD) :
    {
        . = ALIGN(4); /* section size must be multiply of 4. See startup.S file */
        __bss_start = .;
        * (.bss);
        * (.bss*);
        * (COMMON);
        *(.gnu.linkonce.b*)
        . = ALIGN(4); /* section size must be multiply of 4. See startup.S file */
        __bss_end = .;
        . = ALIGN(8);
        Heap_Bank1_Start = .;
    } > SRAM
    __bss_size = __bss_end - __bss_start;
    
    /* Dummy section for stack */
    Stack (NOLOAD) : AT(0)
    {
        . = ALIGN(8);
    } > SRAM
    
    /* .no_init section contains SystemCoreClock. See system.c file */
    .no_init ORIGIN(SRAM) + LENGTH(SRAM) - 4 (NOLOAD): 
    {
        __initial_sp = .; /* We use all of the available RAM as Stack */
        * (.no_init);
    } > SRAM
    
    /DISCARD/ :
    {
        *(.comment)
    }

    .stab              0 (NOLOAD) : { *(.stab) }
    .stabstr           0 (NOLOAD) : { *(.stabstr) }

    /* DWARF 1 */
    .debug             0 : { *(.debug) }
    .line              0 : { *(.line) }

    /* GNU DWARF 1 extensions */
    .debug_srcinfo     0 : { *(.debug_srcinfo) }
    .debug_sfnames     0 : { *(.debug_sfnames) }

    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges     0 : { *(.debug_aranges) }
    .debug_pubnames    0 : { *(.debug_pubnames) }
    .debug_pubtypes    0 : { *(.debug_pubtypes) }

    /* DWARF 2 */
    .debug_info        0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev      0 : { *(.debug_abbrev) }
    .debug_line        0 : { *(.debug_line) }
    .debug_frame       0 : { *(.debug_frame) }
    .debug_str         0 : { *(.debug_str) }
    .debug_loc         0 : { *(.debug_loc) }
    .debug_macinfo     0 : { *(.debug_macinfo) }

    /* DWARF 2.1 */
    .debug_ranges      0 : { *(.debug_ranges) }

    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames   0 : { *(.debug_weaknames) }
    .debug_funcnames   0 : { *(.debug_funcnames) }
    .debug_typenames   0 : { *(.debug_typenames) }
    .debug_varnames    0 : { *(.debug_varnames) }

    /* Build attributes */
    .build_attributes  0 : { *(.ARM.attributes) }
}
