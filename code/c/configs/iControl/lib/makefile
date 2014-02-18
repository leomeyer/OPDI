###############################################################################
# Makefile for the project libwizslave
###############################################################################
## General Flags
####################################################################

PROJECT = WizSlaveAVRLib
MCU = atxmega192a3

####################################################################
## It is important to put "lib" in front of your project name
TARGET = libwizslave.a
CC = avr-gcc

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -O0 -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -Wp,-M,-MP,-MT,$(*F).o,-MF,dep/$(@F).d

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)

############################################################
LDFLAGS += -Wl,-WizSlaveAVRLib.map

## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom
HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0

## Include Directories
INCLUDES = -I "." -I "../../../platforms" -I "../../..//platforms/avr" -I "../../../common"

## Libraries
LIBS = -lm

## Objects that must be built in order to link
OBJECTS = opdi_platformfuncs.o opdi_messages.o opdi_ports.o opdi_protocol.o opdi_strings.o

## Build
all: $(TARGET)

## Compile
opdi_platformfuncs.o: ../../../platforms/avr/opdi_platformfuncs.c 
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@
opdi_messages.o: ../../../common/opdi_messages.c 
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@
opdi_ports.o: ../../../common/opdi_ports.c 
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@
opdi_protocol.o: ../../../common/opdi_protocol.c 
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@
opdi_strings.o: ../../../common/opdi_strings.c 
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

##Link
$(TARGET): $(OBJECTS)
	avr-ar -r $(TARGET) $(OBJECTS)
%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

## Clean target
.PHONY: clean

###############################################################
clean:
	-rm -rf $(OBJECTS) WizSlaveAVRLib.lss WizSlaveAVRLib.map

## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)