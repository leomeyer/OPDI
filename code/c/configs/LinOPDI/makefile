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
SRC += $(CPATH)/opdi_message.c $(CPATH)/opdi_port.c $(CPATH)/opdi_protocol.c $(CPATH)/opdi_slave_protocol.c $(CPATH)/opdi_strings.c $(CPATH)/opdi_aes.cpp $(CPATH)/opdi_rijndael.cpp

# master implementation
MPATH = $(CPATH)/master

SRC += $(MPATH)/opdi_AbstractProtocol.cpp $(MPATH)/opdi_BasicDeviceCapabilities.cpp $(MPATH)/opdi_BasicProtocol.cpp
SRC += $(MPATH)/opdi_DigitalPort.cpp $(MPATH)/opdi_IODevice.cpp $(MPATH)/opdi_main_io.cpp $(MPATH)/opdi_OPDIMessage.cpp
SRC += $(MPATH)/opdi_MessageQueueDevice.cpp $(MPATH)/opdi_OPDIPort.cpp $(MPATH)/opdi_PortFactory.cpp $(MPATH)/opdi_ProtocolFactory.cpp
SRC += $(MPATH)/opdi_StringTools.cpp $(MPATH)/opdi_TCPIPDevice.cpp $(MPATH)/opdi_SelectPort.cpp $(MPATH)/opdi_SerialDevice.cpp

# conio include path
CONIOINCPATH = ../../libraries/conio

# POCO include path
POCOINCPATH = ../../libraries/POCO/Util/include ../../libraries/POCO/Foundation/include ../../libraries/POCO/Net/include

# POCO library path
# for 32 bit systems
# POCOLIBPATH = ../../libraries/POCO/lib/Linux/x86_64
# for 64 bit systems
POCOLIBPATH = ../../libraries/POCO/lib/Linux/i686

# POCO libraries
POCOLIBS = -lPocoUtil -lPocoNet -lPocoFoundation 

# libctb serial communication library
LIBCTB = ../../libraries/libctb
LIBCTBINC = $(LIBCTB)/include
SRC += $(LIBCTB)/src/fifo.cpp $(LIBCTB)/src/getopt.cpp $(LIBCTB)/src/iobase.cpp $(LIBCTB)/src/kbhit.cpp $(LIBCTB)/src/linux/serport.cpp
SRC += $(LIBCTB)/src/linux/timer.cpp $(LIBCTB)/src/portscan.cpp $(LIBCTB)/src/serportx.cpp

# Additional libraries
LIBS = -lpthread

# The compiler to be used.
CC = g++

# List any extra directories to look for include files here.
# Each directory must be seperated by a space.
EXTRAINCDIRS = $(CPATH) $(MPATH) $(PPATHBASE) $(PPATH) $(POCOINCPATH) $(CONIOINCPATH) $(LIBCTBINC) .

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

