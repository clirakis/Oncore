##################################################################
#
#	Makefile for lassen_gps using gcc on Linux. 
#
#
#	Modified	by	Reason
# 	--------	--	------
#	15-Dec-02       CBL     Original
#
# BBN Technologies
# 127 John Clarke Rd.
# Middletown, R.I. 02842
# (401) - 849 - 2543 
######################################################################
# Machine specific stuff
#
#
TARGET = lassen
COMMON = $(DRIVE)/common
LASSEN = $(HOME)/code/lassen/lib
#
# Compile time resolution.
#
INCLUDE =  -I$(LASSEN) -I$(COMMON)/iolib -I$(COMMON)/utility -I$(ROOT_INC) \
	-I$(COMMON)/geoutils -I$(COMMON)/libdaq 
LIBS = $(LASSEN)/liblassen.a \
	-lutility -lio -lgeoutil -ldaq -lrt -lproj $(ROOT_LIBS) 

EXT_CFLAGS = -fbounds-check

# Rules to make the object files depend on the sources.
SRC     = 
SRCCPP  = main.cpp serial.cpp User.cpp ProcessTime.cpp Average.cpp
SRCS    = $(SRC) $(SRCCPP)

HEADERS = User.hh ProcessTime.hh Average.hh

# When we build all, what do we build?
all:      $(TARGET)

install: $(TARGET)
	cp $(TARGET) $(HOME)/bin

include $(COMMON)/makefiles/makefile.inc


#dependencies
include make.depend 
# DO NOT DELETE
