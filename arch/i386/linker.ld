/*
 * This file is part of nios
 *
 * Copyright (C) 2009 - Maciej Piechotka
 *
 * nios is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * nios is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nios; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

OUTPUT_FORMAT("elf32-i386")
ENTRY(start)
phys = 0x00100000;
virt = 0xC0000000;
SECTIONS
{
	. = phys;
	.setup :
	{
		*(.setup)
	}
	. += virt;
	. = ALIGN(0x1000);
	.text : AT(ADDR(.text) - virt)
	{
		code = .;
		phys_code = . - virt;
		*(.text)
	}
	. = ALIGN(0x1000);
	.data : AT(ADDR(.data) - virt)
	{
		data = .;
		phys_data = . - virt;
		*(.data)
	}
	. = ALIGN(0x1000);
	.bss : AT(ADDR(.bss) - virt)
	{
		bss = .;
		phys_bss = . - virt;
		*(COMMON)
		*(.bss)
	}
	. = ALIGN(0x1000);
	end = .;
	phys_end = . - virt;
	phys = phys;
	virt = virt;
}
