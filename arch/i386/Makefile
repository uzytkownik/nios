CFLAGS+=-std=c99

all: arch/i386/multiboot.o arch/i386/arch.a
	 ld -T arch/i386/linker.ld --cref -Map nios.map -o nios $^
clean: clean-arch-i386-arch
	rm -rf nios

arch/i386/arch.a: arch/i386/start.o arch/i386/memory.o
	ar rcs $@ $^

clean-arch-i386-arch:
	rm -rf arch/i386/arch.a arch/i386/*.o