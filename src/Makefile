##################################################################
#
#	Makefile for xxxx using gcc on Linux. 
#
#
#	Modified	by	Reason
# 	--------	--	------
#	30-Jul-10       CBL     Original
#       09-Oct-21       CBL     no longer need -std=gnu++11 
#       29-Sep-22       CBL     git 
#       30-Oct-22       CBL     Geodetic removed, using proj8 C++
#       01-Nov-22       CBL     Implemented UserSignal
#       28-Apr-24       CBL     adding in a generic NMEA message 
#                               to put data in shared memory for
#                               general consumers. libNMEA.so
#       28-May-26       CBL     changed locations for a single archive
#
######################################################################
# Machine specific stuff
#
#
TARGET = Oncore
#
# Compile time resolution.
#
EXT_CFLAGS =  -g 
INCLUDE =  -I$(HOME)/common/libNMEA -I$(DRIVE)/common/utility \
	-I$(HOME)/common/iolib -I../lib

LIBS    =   -L../lib -lOncore $(ROOT_LIBS) -lRHTTP -L $(HOME)/lib_Linux \
	-lNMEA -lio -lutility -lproj

# Rules to make the object files depend on the sources.
SRC     = 
SRCCPP  = main.cpp GPS.cpp Average.cpp ProcessTime.cpp User.cpp UserSignals.cpp
SRCS    = $(SRC) $(SRCCPP)

HEADERS = GPS.hh Average.hh ProcessTime.hh User.hh UserSignals.hh

# When we build all, what do we build?
all:      $(TARGET)

include $(DRIVE)/common/makefiles/makefile.inc


