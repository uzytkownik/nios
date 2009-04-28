include CONFIG

ifdef DEBUG
ASFLAGS+=-ggdb
CXXFLAGS+=-O0 -ggdb
endif

CXXFLAGS+=-I. -std=gnu++0x
GENERIC=

include arch/$(ARCH)/Makefile

