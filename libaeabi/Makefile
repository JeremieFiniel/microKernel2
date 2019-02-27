PREFIX ?= /opt/gcc-arm-none-eabi-4_8-2014q3
ARMGNU ?= $(PREFIX)/bin/arm-none-eabi

AS=$(ARMGNU)-as -mthumb -k -mfloat-abi=soft

DEPENDFLAGS := -MD -MP
INCLUDES    := -I .
BASEFLAGS   := -O2 -fpic -pedantic -pedantic-errors -nostdlib
BASEFLAGS   += -nostartfiles -ffreestanding -nodefaultlibs
BASEFLAGS   += -fno-builtin -fomit-frame-pointer -mcpu=arm1176jzf-s
WARNFLAGS   := -Wall -Wextra -Wshadow -Wcast-align -Wwrite-strings
WARNFLAGS   += -Wredundant-decls -Winline
WARNFLAGS   += -Wno-attributes -Wno-deprecated-declarations
WARNFLAGS   += -Wno-div-by-zero -Wno-endif-labels -Wfloat-equal
WARNFLAGS   += -Wformat=2 -Wno-format-extra-args -Winit-self
WARNFLAGS   += -Winvalid-pch -Wmissing-format-attribute
WARNFLAGS   += -Wmissing-include-dirs -Wno-multichar
WARNFLAGS   += -Wredundant-decls -Wshadow
WARNFLAGS   += -Wno-sign-compare -Wswitch -Wsystem-headers -Wundef
WARNFLAGS   += -Wno-pragmas -Wno-unused-but-set-parameter -Wno-unused-parameter
WARNFLAGS   += -Wno-unused-but-set-variable -Wno-unused-result -Wno-unused-variable
WARNFLAGS   += -Wwrite-strings -Wdisabled-optimization -Wpointer-arith
WARNFLAGS   += -Werror
ASFLAGS     := $(INCLUDES) $(DEPENDFLAGS) -D__ASSEMBLY__
CFLAGS    := $(INCLUDES) $(DEPENDFLAGS) $(BASEFLAGS) $(WARNFLAGS)
#CFLAGS    += -fno-exceptions -std=c++0x
CFLAGS    += -fno-exceptions -std=c99

AS=$(ARMGNU)-gcc -mfloat-abi=soft $(CFLAGS) -c

SRC_FILES=$(wildcard *.S) 

%.o: %.S
	$(AS) $< -o $@

libaeabi.a: $(SRC_FILES:%.S=%.o)
	ar rcs libaeabi.a $(SRC_FILES:%.S=%.o)

.PHONY: all clean

all: libaeabi.a

clean:
	-rm -f *.o *.a
