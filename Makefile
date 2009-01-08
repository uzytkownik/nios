include CONFIG

ifdef DEBUG
ASFLAGS+=-ggdb
CFLAGS+=-O0 -ggdb
endif

CFLAGS+=-I. -Iarch/$(ARCH)/utils -Ilibk/ -Iliballoc -std=c99

include iapi/Makefile

GENERIC=$(IAPI) liballoc/liballoc.o

include arch/$(ARCH)/Makefile
