/**
 ******************************************************************
 *
 * Module Name : serial.cpp
 *
 * Author/Date : C.B. Lirakis / 24-Dec-05
 *
 * Description :
 *
 * Restrictions/Limitations :
 *
 * Change Descriptions :
 *
 * Classification : Unclassified
 *
 * References :
 *
 *
 * RCS header info.
 * ----------------
 * $RCSfile$
 * $Author$
 * $Date$
 * $Locker$
 * $Name$
 * $Revision$
 *
 * $Log: $
 *
 *
 *******************************************************************
 */

#ifndef lint
/// RCS Information
static char rcsid[]="$Header$";
#endif

// System includes.
#include <iostream>
using namespace std;
#include <string>
#include <cmath>
#include <csignal>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>

/// Local Includes.
#include "serial.hh"
#include "debug.h"

/**
 ******************************************************************
 *
 * Function Name :
 *
 * Description :
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
int SerialOpen(const char *port, speed_t BaudRate)
{
    int rc = -1;
    struct termios newtio;


    if (port != NULL)
    {
	rc = open( port, O_RDWR | O_NOCTTY | O_NDELAY);	
	if (rc <0)
	{
	    LOG_ERROR(strerror(errno));
	}
	else
	{
	    /* 
	     * See page 354 of Steven's Advanced UNIX Programming book. 
	     * Get existing termios properties. 
	     * 
	     * Alternative reference is:
	     * Posix Programmer's Guide - Donald A. Lewine
	     * c 1991 O'Oreily and Associates
	     * page 153  pertains  to Termial  I/O
	     */

	    /* clear struct for new port settings */
	    memset(&newtio,0, sizeof(newtio)); 
	    newtio.c_ispeed = BaudRate;
	    newtio.c_ospeed = BaudRate;
	    /*
	     * Control flag
	     *
	     * CRTSCTS : output hardware flow control (only used if 
	     * the cable has all necessary lines. See sect. 7 of Serial-HOWTO)
	     * CS8     : 8n1 (8bit,no parity,1 stopbit)
	     * CLOCAL  : local connection, no modem contol
	     * CREAD   : enable receiving characters
	     */
	    newtio.c_cflag = CS8 | CLOCAL | CREAD;

	    /*
	     * Input modes.
	     * IGNPAR  : ignore bytes with parity errors
	     * ICRNL   : map CR to NL (otherwise a CR input on the other 
	     *           computer will not terminate input)
	     *         otherwise make device raw (no other input processing)
	     */
	    newtio.c_iflag = IGNPAR | ICRNL | IGNBRK;
	    /*
	     * Output modes
	     * Raw output.
	     */
	    newtio.c_oflag = 0;
	    /*
	     * ICANON  : enable canonical input
	     *           disable all echo functionality, and don't 
	     *           send signals to the calling program.
	     */
	    newtio.c_lflag = ~ICANON;

	    /*
	     * Initialize all control characters
	     * default values can be found in /usr/include/termios.h, 
	     * and are given in the comments, but we don't need them here
	     */
	    newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */
	    newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
	    newtio.c_cc[VERASE]   = 0;     /* del */
	    newtio.c_cc[VKILL]    = 0;     /* @ */
	    newtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
	    newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
	    newtio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
	    //    newtio.c_cc[VSWTC]    = 0;     /* '\0' */
	    newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */
	    newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
	    newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
	    newtio.c_cc[VEOL]     = 0;     /* '\0' */
	    newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
	    newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
	    newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
	    newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
	    newtio.c_cc[VEOL2]    = 0;     /* '\0' */

	    /*
	     * now clean the modem line and activate the settings for the port
	     */
	    tcflush(rc, TCIFLUSH);
	    tcsetattr(rc, TCSANOW, &newtio);
	    /* 
	     * this makes the read() function wait until it has stuff to 
	     * read before reading 
	     */
	    fcntl( rc, F_SETFL, 0);
	}
    }
    return rc;
}
/**
 ******************************************************************
 *
 * Function Name :
 *
 * Description :
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */

