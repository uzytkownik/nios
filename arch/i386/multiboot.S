#define STACK_SIZE 0x4000

.section .setup,"ax",@progbits
	.align 4
multiboot_header:
	.long 	0x1BADB002 			/* Magick */
	.long 	0x00000003 			/* Flags [ELF] */
	.long 	-(0x1BADB002 + 0x00000003) 	/* Checksum */

	.globl start
start:
	lgdt    (start_gdt)
	movw    $0x10, %ax
	movw    %ax,   %ds
	movw    %ax,   %es
	movw    %ax,   %fs
	movw    %ax,   %gs
	movw    %ax,   %ss

	ljmp    $0x08, $paging
paging:
	movl    %eax, %ecx
	movl    %ebx, %edx
	
	movl 	$start_paging, %eax
	movl 	%eax, %cr3

	movl  	%cr4, %eax
	movl    $0x00000010, %esi
	or 	%esi, %eax
	movl  	%eax, %cr4

	movl    %cr0, %eax
	movl    $0x80000000, %edi
	or	%edi, %eax
	movl    %eax, %cr0

	jmp    	hh_start

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
	.globl start_paging
start_paging:	
	.long 0x00000183
	.fill 0x300-(. - start_paging)/4, 4
	.long 0x00000183
	.long 0x00400183
	.long 0x00800183
	.long 0x00C00183
	.long 0x01000183
	.fill 0x400 - (. - start_paging)/4, 4
	
.text
hh_start:	
	movl 	$(stack + STACK_SIZE), %esp  	/* Initalize stack pointer */

	pushl   $0				/* Reset EFLAGS */
	popf

	pushl	%edx
	pushl	%ecx

	call	kstart

	.lcomm stack, STACK_SIZE
