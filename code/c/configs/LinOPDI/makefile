# Target file name (without extension).
TARGET = LinOPDI

# OPDI platform specifier
PLATFORM = linux

# Relative path to common directory (without trailing slash)
# This also becomes an additional include directory.
CPATH = ../../common

# Relative path to platform directory (without trailing slash)
# This also becomes an additional include directory.
PPATHBASE = ../../platforms
PPATH = $(PPATHBASE)/$(PLATFORM)

# List C source files of the configuration here.
SRC = $(TARGET).cpp ../test/test.c ../test/master.cpp

# platform specific files
SRC += $(PPATH)/opdi_platformfuncs.c

# common files
SRC += $(CPATH)/opdi_messages.c $(CPATH)/opdi_ports.c $(CPATH)/opdi_protocol.c $(CPATH)/opdi_slave_protocol.c $(CPATH)/opdi_strings.c $(CPATH)/opdi_aes.cpp $(CPATH)/opdi_rijndael.cpp

# master implementation
MPATH = $(CPATH)/master

SRC += $(MPATH)/opdi_AbstractProtocol.cpp $(MPATH)/opdi_BasicDeviceCapabilities.cpp $(MPATH)/opdi_BasicProtocol.cpp $(MPATH)/opdi_DigitalPort.cpp $(MPATH)/opdi_IODevice.cpp $(MPATH)/opdi_main_io.cpp $(MPATH)/opdi_Message.cpp $(MPATH)/opdi_MessageQueueDevice.cpp $(MPATH)/opdi_Port.cpp $(MPATH)/opdi_PortFactory.cpp $(MPATH)/opdi_ProtocolFactory.cpp $(MPATH)/opdi_StringTools.cpp $(MPATH)/opdi_TCPIPDevice.cpp

# conio include path
CONIOINCPATH = ../../libraries/conio

# POCO include path
POCOINCPATH = ../../libraries/POCO/Util/include ../../libraries/POCO/Foundation/include ../../libraries/POCO/Net/include

# POCO library path
POCOLIBPATH = ../../libraries/POCO/lib/Linux/x86_64

# POCO libraries
POCOLIBS = -lPocoUtil -lPocoNet -lPocoFoundation 

# Additional libraries
LIBS = -lpthread

# The compiler to be used.
CC = g++

# List any extra directories to look for include files here.
# Each directory must be seperated by a space.
EXTRAINCDIRS = $(CPATH) $(MPATH) $(PPATHBASE) $(PPATH) $(POCOINCPATH) $(CONIOINCPATH) .

# Place -I options here
CINCS =

# Defines
CDEFINES = -Dlinux

# Compiler flags.
CFLAGS = -Wall $(CDEFS) $(CINCS) -L $(POCOLIBPATH) $(CDEFINES)
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS)) -std=c++11 -static-libstdc++

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

