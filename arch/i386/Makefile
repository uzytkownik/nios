CFLAGS+=-std=c99

ARCH_I386=arch/i386/multiboot.o arch/i386/start.o arch/i386/memory.o

all: $(ARCH_I386) $(GENERIC)
	ld -T arch/i386/linker.ld --cref -Map nios.map -o nios $^

clean: 
	rm -rf nios nios.map
	rm -rf $(ARCH_I386) $(GENERIC)
