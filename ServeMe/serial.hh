/**
 ******************************************************************
 *
 * Module Name : serial.hh
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
#ifndef __SERIAL_hh_
#define __SERIAL_hh_
#include <termios.h>
int SerialOpen( const char *port, speed_t BaudRate);

#endif
