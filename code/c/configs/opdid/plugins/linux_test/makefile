# Target file name (without extension).
TARGET = linux_test

# OPDI platform specifier
PLATFORM = linux

# Relative path to common directory (without trailing slash)
# This also becomes an additional include directory.
CPATH = ../../../../common

# Relative path to platform directory (without trailing slash)
# This also becomes an additional include directory.
PPATHBASE = ../../../../platforms
PPATH = $(PPATHBASE)/$(PLATFORM)

# List C source files of the plugin here.
SRC = $(TARGET).cpp

# platform specific files
SRC += $(PPATH)/opdi_platformfuncs.c

# OPDID includes
OPDIDPATH = ../../opdid

# C++ wrapper
CPPPATH = $(CPATH)/cppwrapper

# C++ wrapper files
SRC += $(CPPPATH)/OPDI_Ports.cpp

# additional source files
# SRC += 

# conio include path
CONIOINCPATH = ../../../libraries/conio

# POCO include path
POCOINCPATH = ../../../../libraries/POCO/Util/include ../../../../libraries/POCO/Foundation/include ../../../../libraries/POCO/Net/include

# POCO library path
POCOLIBPATH = ../../../../libraries/POCO/lib/Linux/x86_64

# POCO libraries
POCOLIBS = -lPocoUtil -lPocoNet -lPocoFoundation 

# Additional libraries
LIBS = -lpthread 

# The compiler to be used.
CC = g++

# List any extra directories to look for include files here.
# Each directory must be seperated by a space.
EXTRAINCDIRS = $(CPATH) $(CPPPATH) $(PPATHBASE) $(PPATH) $(POCOINCPATH) $(CONIOINCPATH) $(OPDIDPATH) .

# Place -I options here
CINCS =

# Defines
CDEFINES = -Dlinux

# Compiler flags.
CFLAGS = -Wall $(CDEFS) $(CINCS) -L $(POCOLIBPATH) $(CDEFINES)
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS)) -O3 -std=c++11 -static-libstdc++

OBJECTS = $(SRC)

all: $(SRC) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -shared -fPIC -O3 -o $@.so $(POCOLIBS) $(LIBS)

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

