# Makefile for AskSin OTA bootloader
#
# Instructions
#
# To make bootloader .hex file:
# make HM_LC_Sw1PBU_FM
# make HB_UW_Sen_THPL
# etc...
#

# program name should not be changed...
PROGRAM          = Bootloader-AskSin-OTA
F_CPU            = 8000000
SUFFIX           =

CC               = avr-gcc
OPTIMIZE         = -Os

OBJCOPY          = avr-objcopy
FORMAT           = ihex

# Override is only needed by avr-lib build system.
override CFLAGS  = -g -Wall $(OPTIMIZE) -mmcu=$(MCU) -DF_CPU=$(F_CPU)
override LDFLAGS = -Wl,--section-start=.text=${BOOTLOADER_START},--section-start=.addressData=${ADDRESS_DATA_START},--section-start=.boot1=${BOOT1_START}


all:

# Settings for HM_LC_Sw1PBU_FM (Atmega644, 4k Bootloader size)
#
# CODE_LEN:           lenth of program space - 2 bytes (adress of crc-check data)
# BOOTLOADER_START:   start address of the bootoader   (4k bootloader space)
# BOOT_PAGES		  Number of pages in the bootloader section (remember 328p and 644a have different page size)
# BOOT1_START		  Start address of protected .boot1 section for OTA self update function
# ADDRESS_DATA_START: start address of adressdata      (last 16 bytes in flash)
#
HM_LC_Sw1PBU_FM:    TARGET                = HM_LC_Sw1PBU_FM
HM_LC_Sw1PBU_FM:    MCU                   = atmega644
HM_LC_Sw1PBU_FM:    CODE_LEN              = 0xEFFE
HM_LC_Sw1PBU_FM:    BOOTLOADER_START      = 0xF000
HM_LC_Sw1PBU_FM:    BOOT_PAGES		      = 16
HM_LC_Sw1PBU_FM:    BOOT1_START		      = 0xFF00
HM_LC_Sw1PBU_FM:    ADDRESS_DATA_START    = 0xFEF0
HM_LC_Sw1PBU_FM:    hex

# Settings for HM_LC_Sw1PBU_FM (Atmega644, 8k Bootloader size)
HM_LC_Sw1PBU_FM_8k: TARGET                = HM_LC_Sw1PBU_FM
HM_LC_Sw1PBU_FM_8k: SUFFIX                = _8k
HM_LC_Sw1PBU_FM_8k: MCU                   = atmega644
HM_LC_Sw1PBU_FM_8k: CODE_LEN              = 0xDFFE
HM_LC_Sw1PBU_FM_8k: BOOTLOADER_START      = 0xE000
HM_LC_Sw1PBU_FM_8k: BOOT_PAGES		      = 32
HM_LC_Sw1PBU_FM_8k: BOOT1_START		      = 0xFF00
HM_LC_Sw1PBU_FM_8k: ADDRESS_DATA_START    = 0xFEF0
HM_LC_Sw1PBU_FM_8k: hex

# Settings for HM_LC_Sw1PBU_FM (Atmega328p, 4k Bootloader size)
HB_UW_Sen_THPL:     TARGET                = HB_UW_Sen_THPL
HB_UW_Sen_THPL:     MCU                   = atmega328p
HB_UW_Sen_THPL:     CODE_LEN              = 0x6FFE
HB_UW_Sen_THPL:     BOOTLOADER_START      = 0x7000
HB_UW_Sen_THPL:     BOOT_PAGES		      = 31
HB_UW_Sen_THPL:     BOOT1_START		      = 0x7F80
HB_UW_Sen_THPL:     ADDRESS_DATA_START    = 0x7F70
HB_UW_Sen_THPL:     hex

hex: uart_code
	$(CC) -Wall -c -std=c99 -mmcu=$(MCU) $(LDFLAGS)     -DF_CPU=$(F_CPU) -D$(TARGET) $(OPTIMIZE) cc.c -o cc.o
	$(CC) -Wall    -std=c99 -mmcu=$(MCU) $(LDFLAGS)     -DF_CPU=$(F_CPU) -D$(TARGET) -DBOOTLOADER_START=${BOOTLOADER_START} -DBOOT_PAGES=${BOOT_PAGES} -DCODE_LEN=${CODE_LEN} $(OPTIMIZE) bootloader.c cc.o uart/uart.o -o $(PROGRAM)-$(TARGET)$(SUFFIX).elf
	$(OBJCOPY) -j .text -j .data -j .addressData -j .boot1 -O $(FORMAT) $(PROGRAM)-$(TARGET)$(SUFFIX).elf $(PROGRAM)-$(TARGET)$(SUFFIX).hex

	avr-size -C --mcu=$(MCU) $(PROGRAM)-$(TARGET)$(SUFFIX).elf

uart_code:
	$(MAKE) -C ./uart/ MCU=$(MCU)
	
clean:
	rm -rf *.o *.elf *.lst *.map *.sym *.lss *.eep *.srec *.bin *.hex \
	uart/*.o uart/*.elf uart/*.lst uart/*.map uart/*.sym uart/*.lss uart/*.eep uart/*.srec uart/*.bin uart/*.hex
