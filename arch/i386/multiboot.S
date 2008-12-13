#define STACK_SIZE 0x4000

.text
	.globl	start, _start
start:
_start:
	jmp	multiboot_entry
	.align	4
multiboot_header:
	.long 	0x1BADB002 			/* Magick */
	.long 	0x00010003 			/* Flags [ELF] */
	.long 	-(0x1BADB002 + 0x00010003) 	/* Checksum */
	.long 	multiboot_header
	.long 	code
	.long 	data
	.long 	end
	.long 	multiboot_entry
multiboot_entry:
	movl 	$(stack + STACK_SIZE), %esp  	/* Initalize stack pointer */

	pushl   $0				/* Reset EFLAGS */
	popf

	pushl	%ebx
	pushl	%eax

	call	kstart

.comm stack, STACK_SIZE