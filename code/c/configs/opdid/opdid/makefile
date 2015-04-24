# Target file name (without extension).
TARGET = LinuxOPDID

# OPDI platform specifier
PLATFORM = linux

# Relative path to common directory (without trailing slash)
# This also becomes an additional include directory.
CPATH = ../../../common

# Relative path to platform directory (without trailing slash)
# This also becomes an additional include directory.
PPATHBASE = ../../../platforms
PPATH = $(PPATHBASE)/$(PLATFORM)

# List C source files of the configuration here.
SRC = LinuxOPDID.cpp OPDIDConfigurationFile.cpp

# platform specific files
SRC += $(PPATH)/opdi_platformfuncs.c

# common files
SRC += $(CPATH)/opdi_message.c $(CPATH)/opdi_port.c $(CPATH)/opdi_protocol.c $(CPATH)/opdi_slave_protocol.c $(CPATH)/opdi_strings.c
# $(CPATH)/opdi_aes.cpp $(CPATH)/opdi_rijndael.cpp

# master implementation
MPATH = $(CPATH)/master

# master files
# SRC += $(MPATH)/opdi_AbstractProtocol.cpp $(MPATH)/opdi_BasicDeviceCapabilities.cpp $(MPATH)/opdi_BasicProtocol.cpp $(MPATH)/opdi_DigitalPort.cpp $(MPATH)/opdi_IODevice.cpp $(MPATH)/opdi_main_io.cpp $(MPATH)/opdi_OPDIMessage.cpp $(MPATH)/opdi_MessageQueueDevice.cpp $(MPATH)/opdi_OPDIPort.cpp $(MPATH)/opdi_PortFactory.cpp $(MPATH)/opdi_ProtocolFactory.cpp $(MPATH)/opdi_StringTools.cpp $(MPATH)/opdi_TCPIPDevice.cpp

# C++ wrapper
CPPPATH = $(CPATH)/cppwrapper

# C++ wrapper files
SRC += $(CPPPATH)/OPDI.cpp $(CPPPATH)/OPDI_Ports.cpp

# additional source files
SRC += ./AbstractOPDID.cpp ./OPDID_Ports.cpp ./opdid_linux.cpp

# conio include path
CONIOINCPATH = ../../../libraries/conio

# POCO include path
POCOINCPATH = ../../../libraries/POCO/Util/include ../../../libraries/POCO/Foundation/include ../../../libraries/POCO/Net/include

# POCO library path
POCOLIBPATH = ../../../libraries/POCO/lib/Linux/x86_64

# POCO libraries
POCOLIBS = -lPocoUtil -lPocoNet -lPocoFoundation -lPocoXML -lPocoJSON

# ExprTk expression library path
EXPRTK = ../../../libraries/ExprTk

# Additional libraries
LIBS = -lpthread -ldl -lrt

# The compiler to be used.
CC = g++

# List any extra directories to look for include files here.
# Each directory must be seperated by a space.
EXTRAINCDIRS = $(CPATH) $(CPPPATH) $(MPATH) $(PPATHBASE) $(PPATH) $(POCOINCPATH) $(CONIOINCPATH) $(EXPRTK) .

# Place -I options here
CINCS =

# Defines
CDEFINES = -Dlinux

# Compiler flags.
CFLAGS = -Wall $(CDEFS) $(CINCS) -L $(POCOLIBPATH) $(CDEFINES)
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS)) -std=c++11 -static-libstdc++

# Debug flags
CFLAGS += -ggdb -O0 -fbuiltin

OBJECTS = $(SRC)

all: $(SRC) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(POCOLIBS) $(LIBS)

.cpp.o:
	@echo
	@echo Compiling: $<
	$(CC) -c $(CFLAGS) $< -o $@

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(PPATH)/*.o
	rm -f $(CPATH)/*.o
	rm -f $(MPATH)/*.o
	rm $(TARGET)

