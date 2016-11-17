/* Link Script
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * linker_script.ld: Linker Script for xmc bootstrapper. 
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
ENTRY(entry)

MEMORY
{
    FLASH(RX) : ORIGIN = FLASH_ORIGIN, LENGTH = FLASH_LENGTH
    SRAM(!RX) : ORIGIN = SRAM_ORIGIN, LENGTH = SRAM_LENGTH
}


SECTIONS
{
    /* TEXT section */
    .text : 
    {
      sText = .;
      KEEP(*(.entry));
      *(.text .text.*);
    } > SRAM

    . = ALIGN(4);
    
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
