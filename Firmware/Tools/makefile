######################################################################
# makefile
#
# Builds the RTK support programs
######################################################################

.ONESHELL:
SHELL=/bin/bash

##########
# Source files
##########

EXECUTABLES  = Compare
EXECUTABLES += NMEA_Client
EXECUTABLES += Read_Map_File
EXECUTABLES += RTK_Reset
EXECUTABLES += Split_Messages
EXECUTABLES += X.509_crt_bundle_bin_to_c

INCLUDES  = crc24q.h

##########
# Buid tools and rules
##########

GCC = gcc
CFLAGS = -flto -O3 -Wpedantic -pedantic-errors -Wall -Wextra -Werror -Wno-unused-variable -Wno-unused-parameter
CC = $(GCC) $(CFLAGS)

%.o: %.c $(INCLUDES)
	$(CC) -c -o $@ $<

%: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ $<

##########
# Buid all the sources - must be first
##########

.PHONY: all

all: $(EXECUTABLES)

########
# Clean the build directory
##########

.PHONY: clean

clean:
	rm -f *.o *.a $(EXECUTABLES)
