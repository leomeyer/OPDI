###############################################################################
# ArduinOPDILib
# Makefile for the dependent Arduino OPDI library
# Leo Meyer, leo@leomeyer.de, 2013-2014
###############################################################################
## General Flags
####################################################################

PROJECT = ArduinOPDILib

# MCU is defined in the enclosing project's build environment variables
MCU = $(AVRTARGETMCU)

####################################################################
## It is important to put "lib" in front of your project name
TARGET = libarduinopdi.a
CC = avr-gcc
CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -O0 -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -Wp,-M,-MP,-MT,$(*F).o,-MF,dep/$(@F).d

## Compile options for C++ compilation units.
CPPFLAGS = $(COMMON)
CPPFLAGS += -w -Os -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -fno-exceptions

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)

############################################################
LDFLAGS += -Wl,-WizSlaveAVRLib.map

## Include Directories
INCLUDES = -I "." -I "../../../../platforms" -I "../../../../platforms/avr" -I "../../../../common"

## Libraries
LIBS = -lm

## Objects that must be built in order to link
OBJECTS = opdi_platformfuncs.o opdi_message.o opdi_port.o opdi_protocol.o opdi_slave_protocol.o opdi_strings.o OPDI.o

## Build
all: $(TARGET)

## Compile
opdi_platformfuncs.o: ../../../../platforms/avr/opdi_platformfuncs.c 
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@
opdi_message.o: ../../../../common/opdi_message.c 
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@
opdi_port.o: ../../../../common/opdi_port.c 
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@
opdi_protocol.o: ../../../../common/opdi_protocol.c 
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@
opdi_slave_protocol.o: ../../../../common/opdi_slave_protocol.c 
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@
opdi_strings.o: ../../../../common/opdi_strings.c 
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
	-rm -rf $(OBJECTS)

## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)