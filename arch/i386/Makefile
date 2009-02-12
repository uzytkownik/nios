include arch/i386/kernel/Makefile

CXXFLAGS+=-DI386 -Iarch/i386/ -DDMA_START=$(DMA_START) -DDMA_END=$(DMA_END)

ARCH_I386=arch/i386/multiboot.o arch/i386/start.o 

all: $(ARCH_I386) $(ARCH_I386_KERNEL) $(GENERIC)
	g++ -Wl,-T arch/i386/linker.ld -Wl,--cref -Wl,-Map -Wl,nios.map -o nios $^ -nostartfiles

clean: 
	rm -rf nios nios.map
	rm -rf $(ARCH_I386) $(ARCH_I386_KERNEL) $(GENERIC)
