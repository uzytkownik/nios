#define STACK_SIZE 0x4000

.section .setup
	.globl start, _start
start:
_start:
	jmp multiboot_entry
	.align  4
multiboot_header:
	.long 	0x1BADB002 			/* Magick */
	.long 	0x00000003 			/* Flags [ELF] */
	.long 	-(0x1BADB002 + 0x00000003) 	/* Checksum */
multiboot_entry:
	lgdt    (start_gdt)
	movw    $0x10, %ax
	movw    %ax,   %ds
	movw    %ax,   %es
	movw    %ax,   %fs
	movw    %ax,   %gs
	movw    %ax,   %ss

	ljmp    $0x08, $hh_start

.text
hh_start:	
	movl 	$(stack + STACK_SIZE), %esp  	/* Initalize stack pointer */

	pushl   $0				/* Reset EFLAGS */
	popf

	pushl	%ebx
	pushl	%eax

	call	kstart

	.lcomm stack, STACK_SIZE
	
.section .setup
	.align 4
	.globl start_gdt, start_gdt_start
start_gdt:
	.word 3
	.long start_gdt_start
	.align 4
start_gdt_start:
	.byte 0x00, 0x00, 0, 0, 0, 0b00000000, 0b00000000, 0x00
	.byte 0xFF, 0xFF, 0, 0, 0, 0b10011010, 0b11001111, 0x00
	.byte 0xFF, 0xFF, 0, 0, 0, 0b10010010, 0b11001111, 0x00
start_gdt_end:
	