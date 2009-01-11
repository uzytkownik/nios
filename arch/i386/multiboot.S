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
	movl    %cr4, %ebx
	movl    $0x00000010, %esi
	movl    %cr0, %ecx
	movl    $0x80000000, %edi
	movl    start_paging, %eax

	or      %esi, %ebx
	or      %edi, %ecx
	
	movl    %eax, %cr3
	movl    %ebx, %cr4
	movl    %ecx, %cr0
	
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
	.word start_gdt_end - start_gdt_start - 1
	.long start_gdt_start
	.align 4
start_gdt_start:
	.byte 0x00, 0x00, 0, 0, 0, 0b00000000, 0b00000000, 0x00
	/* Kernel */
	.byte 0xFF, 0xFF, 0, 0, 0, 0b10011010, 0b11001111, 0x00 /* 0x08 */
	.byte 0xFF, 0xFF, 0, 0, 0, 0b10010010, 0b11001111, 0x00 /* 0x10 */
start_gdt_end:
	.align 0x1000
start_paging:	
	.long 0x00000103
	.fill 0xC0000000/0x400000-1, 4
	.long 0x00000103
	.long 0x00400103
	.long 0x00800103
	.long 0x00C00103
	.long 0x01000103
	.fill 0x40000000/0x400000-1, 4
	