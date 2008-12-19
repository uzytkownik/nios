include CONFIG

ifdef DEBUG
ASFLAGS+=-ggdb
CFLAGS+=-O0 -ggdb
endif

CFLAGS+=-I. -Iarch/$(ARCH)/utils

include iapi/Makefile

GENERIC=$(IAPI)

include arch/$(ARCH)/Makefile
