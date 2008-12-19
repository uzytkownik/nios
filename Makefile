include CONFIG

ifdef DEBUG
ASFLAGS+=-ggdb
CFLAGS+=-O0 -ggdb
endif

include arch/$(ARCH)/Makefile
