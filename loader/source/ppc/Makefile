# Nintendont: PadReadGC module for PowerPC

#echo "PadReadGC.c"
#$DEVKITPPC/bin/powerpc-eabi-gcc -T openstub.ld PADReadGC.c -o ../../data/PADReadGC.bin
#cd ../../data
#$DEVKITPPC/bin/powerpc-eabi-objcopy -S -O binary PADReadGC.bin
#cd ../source/ppc

#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------

.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

CFLAGS	:= -O3 -g -std=gnu89 -Wall $(MACHDEP) $(INCLUDE)
LDFLAGS	:= -s -nostartfiles -T openstub.ld
ARFLAGS	:= cru

TARGET_BIN := ../../data/PADReadGC.bin
TARGET_ELF := PADReadGC.elf

.PHONY: FORCE

all: $(TARGET_BIN)

-include PADReadGC.d

$(TARGET_BIN): $(TARGET_ELF)
	@echo  "OBJCOPY $@"
	@$(OBJCOPY) -S -O binary $< $@

$(TARGET_ELF): PADReadGC.c
	@echo  "CC	$@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -MMD -MP -MF PADReadGC.d -o $@ $<

clean:
	-rm -f $(TARGET_BIN) $(TARGET_ELF) PADReadGC.d
