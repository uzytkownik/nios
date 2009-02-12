include CONFIG

ifdef DEBUG
ASFLAGS+=-ggdb
CXXFLAGS+=-O0 -ggdb
endif

CXXFLAGS+=-I.
GENERIC=

include arch/$(ARCH)/Makefile

