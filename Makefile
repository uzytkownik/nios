include CONFIG

ifdef DEBUG
ASFLAGS+=-ggdb
CFLAGS+=-O0 -ggdb
endif

GENERIC=$(IAPI)

include arch/$(ARCH)/Makefile
