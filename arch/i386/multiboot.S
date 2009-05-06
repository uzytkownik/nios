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

#define STACK_SIZE 0x4000

.section .setup,"ax",@progbits
	.align 4
multiboot_header:
	.type	multiboot_header, @object
	.size 	multiboot_header, 12
	.long 	0x1BADB002 			/* Magick */
	.long 	0x00000003 			/* Flags [ELF] */
	.long 	-(0x1BADB002 + 0x00000003) 	/* Checksum */

	.globl 	start
	.type 	start, @function
start:
	movl    %eax, %ecx
	movl    %ebx, %edx
	
	lgdt    (start_gdt)
	movw    $0x10, %ax
	movw    %ax,   %ds
	movw    %ax,   %es
	movw    %ax,   %fs
	movw    %ax,   %gs
	movw    %ax,   %ss

	ljmp    $0x08, $_paging
_paging:
	movl 	$start_paging, %eax
	movl 	%eax, %cr3

	movl  	%cr4, %eax
	btsl 	$4, %eax
	movl  	%eax, %cr4

	movl    %cr0, %eax
	btsl	$31, %eax
	movl    %eax, %cr0

	jmp    	hh_start

	.size	start, .-start
	.align	4
	.globl 	start_gdt, start_gdt_start
start_gdt:
	.type 	start_gdt, @object
	.word 	_start_gdt_end - start_gdt_start - 1
	.long 	start_gdt_start
	.size 	start_gdt, .-start_gdt
	.align 	4
start_gdt_start:
	.type 	start_gdt_start, @object
	.byte	 0x00, 0x00, 0, 0, 0, 0b00000000, 0b00000000, 0x00
	/* Kernel */
	.byte 	0xFF, 0xFF, 0, 0, 0, 0b10011010, 0b11001111, 0x00 /* 0x08 */
	.byte 	0xFF, 0xFF, 0, 0, 0, 0b10010010, 0b11001111, 0x00 /* 0x10 */
	.size 	start_gdt_start, .-start_gdt_start
_start_gdt_end:
	.align 	0x1000
	.globl 	start_paging
	.type 	start_paging, @object
start_paging:
	.long 	0x00000183
	.fill 	0x300-(. - start_paging)/4, 4
	.long 	0x00000183
	.long 	0x00400183
	.long 	0x00800183
	.long 	0x00C00183
	.long 	0x01000183
	.fill 	0x400 - (. - start_paging)/4, 4
	.size 	start_paging, .-start_paging
.text
	.globl 	hh_start
	.type 	hh_start, @function
hh_start:	
	movl 	$(stack + STACK_SIZE), %esp  	/* Initalize stack pointer */

	pushl   $0				/* Reset EFLAGS */
	popf

	pushl	%edx
	pushl	%ecx

	call	kstart
	
	.size	hh_start, hh_start-.
	.lcomm stack, STACK_SIZE
