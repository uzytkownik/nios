OUTPUT_FORMAT("elf32-i386")
ENTRY(start)
phys = 0x00100000;
SECTIONS
{
	. = phys;
	.text :
	{
	      code = .;
	      *(.text)
	}
	. = ALIGN(0x1000);
	.data :
	{
		data = .;
		*(.data)
	}
	. = ALIGN(0x1000);
	.bss :
	{
		bss = .;
		*(COMMON)
		*(.bss)
	}
	. = ALIGN(0x1000);
	end = .;
}
/*SECTIONS
{
	.text phys : AT(phys)
	{
		code = .;
		*(.text)
		. = ALIGN(4096);
	}
	.data : AT(phys + (data - code))
	{
		data = .;
		*(.data)
		. = ALIGN(4096);
	}
	.bss : AT(phys + (bss - code))
	{
		bss = .;
		*(.bss)
		*(COMMON)
		. = ALIGN(4096);
	}
	end = .;
}*/