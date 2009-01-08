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
