#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------

.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifeq ($(USE_LIBCUSTOMFAT),1)
PPC_FAT_LIBRARY := libcustomfat/libogc/lib/wii/libfat.a
else
PPC_FAT_LIBRARY := fatfs/libfat-ppc.a
endif

SUBPROJECTS := multidol kernel/asm resetstub \
	fatfs/libfat-arm.a fatfs/libfat-ppc.a \
	libcustomfat/libogc/lib/wii/libfat.a \
	codehandler kernel \
	loader/source/ppc loader
.PHONY: all forced clean $(SUBPROJECTS)

all: loader
forced: clean all

multidol:
	@echo " "
	@echo "Building Multi-DOL loader"
	@echo " "
	$(MAKE) -C multidol

kernel/asm:
	@echo " "
	@echo "Building asm files"
	@echo " "
	$(MAKE) -C kernel/asm

resetstub:
	@echo " "
	@echo "Building reset stub"
	@echo " "
	$(MAKE) -C resetstub

fatfs/libfat-arm.a:
	@echo " "
	@echo "Building FatFS library for ARM"
	@echo " "
	$(MAKE) -C fatfs -f Makefile.arm

fatfs/libfat-ppc.a:
	@echo " "
	@echo "Building FatFS library for PPC"
	@echo " "
	$(MAKE) -C fatfs -f Makefile.ppc

libcustomfat/libogc/lib/wii/libfat.a:
	@echo " "
	@echo "Building libcustomfat library for PPC"
	@echo " "
	$(MAKE) -C libcustomfat wii-release

codehandler:
	@echo " "
	@echo "Building Nintendont code handler"
	@echo " "
	$(MAKE) -C codehandler

kernel: kernel/asm fatfs/libfat-arm.a codehandler
	@echo " "
	@echo "Building Nintendont kernel"
	@echo " "
	$(MAKE) -C kernel

loader/source/ppc:
	@echo " "
	@echo "Building Nintendont HID"
	@echo " "
	$(MAKE) -C loader/source/ppc

loader: multidol resetstub $(PPC_FAT_LIBRARY) kernel loader/source/ppc
	@echo " "
	@echo "Building Nintendont loader"
	@echo " "
	$(MAKE) -C loader USE_LIBCUSTOMFAT=$(USE_LIBCUSTOMFAT)

clean:
	@echo " "
	@echo "Cleaning all subprojects..."
	@echo " "
	$(MAKE) -C multidol clean
	$(MAKE) -C kernel/asm clean
	$(MAKE) -C resetstub clean
	$(MAKE) -C fatfs -f Makefile.arm clean
	$(MAKE) -C fatfs -f Makefile.ppc clean
	$(MAKE) -C libcustomfat clean
	$(MAKE) -C codehandler clean
	$(MAKE) -C kernel clean
	$(MAKE) -C loader/source/ppc clean
	$(MAKE) -C loader clean
