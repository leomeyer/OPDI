# Target file name (without extension).
TARGET = LinOPDI

# OPDI platform specifier
PLATFORM = linux

# Relative path to common directory (without trailing slash)
# This also becomes an additional include directory.
CPATH = ../../common

# Relative path to platform directory (without trailing slash)
# This also becomes an additional include directory.
PPATH = ../../platforms/$(PLATFORM)
PPATH1 = ../../platforms

# List C source files of the configuration here.
SRC = $(TARGET).cpp server.cpp device.c

# platform specific files
SRC += $(PPATH)/opdi_platformfuncs.c

# common files
SRC += $(CPATH)/opdi_messages.c $(CPATH)/opdi_ports.c $(CPATH)/opdi_protocol.c $(CPATH)/opdi_strings.c $(CPATH)/opdi_aes.cpp  $(CPATH)/opdi_rijndael.cpp

# The compiler to be used.
CC = g++

# List any extra directories to look for include files here.
# Each directory must be seperated by a space.
EXTRAINCDIRS = $(CPATH) $(PPATH) $(PPATH1) .

# Place -I options here
CINCS =

# Compiler flags.
CFLAGS = $(CDEFS) $(CINCS)
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))

OBJECTS = $(SRC:.cpp=.o)

all: $(SRC) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

.cpp.o:
	@echo
	@echo Compiling: $<
	$(CC) -c $(CFLAGS) $< -o $@

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

