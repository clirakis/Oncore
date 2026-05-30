/********************************************************************
 *
 * Module Name : OncoreDisp.cpp
 *
 * Author/Date : CBL/26-Dec-15
 *
 * Description : Display data TSIP interface
 *
 * Restrictions/Limitations :
 *
 * Change Descriptions :
 * 19-Feb-22 CBL  Updated to class structure
 * 07-Feb-26 CBL  Updated to enable user enabled file change
 *
 * Classification : Unclassified
 *
 * References :
 *
 *
 ********************************************************************/
// System Includes
// System includes.
#include <iostream>
using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <ncurses.h>
#include <time.h>
#include <math.h>

// Local Includes
#include "tools.h"
#include "debug.h"
#include "OncoreDisp.hh"
#include "GPS.hh"
#include "CLogger.hh"
#include "Oncore.hh"

Oncore_Display* Oncore_Display::fOncore_Display;


static int CommandCol   = 2;
static const char *main_frame_window[] =
{
    "+----------------------------------------------------------------------------+",
    "|            Oncore  (h)ome, (r)efresh                                        ",
    "+----------------------------------------------------------------------------+",
    "|                                                                            |",
    "|                                                                            |",
    "|                                                                            |",
    "|                                                                            |",
    "|                                                                            |",
    "|                                                                            |",
    "|                                                                            |",
    "|                                                                            |",
    "|                                                                            |",
    "|                                                                            |",
    "|                                                                            |",
    "|                                                                            |",
    "|  Messages -----------------------------------------------------------------+",
    "|                                                                            |",
    "|                                                                            |",
    "|                                                                            |",
    "|                                                                            |",
    "|                                                                            |",
    "|  Command  -----------------------------------------------------------------+",
    "|                                                                            |",
    "+----------------------------------------------------------------------------+"
};

#define MAINFRAMEWINSIZ sizeof(main_frame_window)/sizeof(main_frame_window[0])

static const char *pos_strings[] = {
             "   Position Data                               ",
             "       Latitude :                Mode:         ",
             "      Longitude :                 NSV:         ",
             "        Altitude:                 PRN:         ",
             "           Speed:                 DOP:         ",
             "         Heading:                TDOP:         ",
             "                               Status:         ",
             "                                               ",
	     "                                               ",
             "                                               ",
             "                                               ",
};

#define POS_STR_SIZE sizeof(pos_strings) / sizeof(pos_strings[0])


static const char *help_strings[] = {
             "                          Help                 ",
             "                                               ",
             "  d - toggle display data (Hex codes rec'd)    ",
             "                                               ",
             "  f - filename change, advance number          ",
             "  h - home                                     ",
             "                                               ",
             "  q - quit                                     ",
             "  r - refresh                                  ",
	     "                                               ",
             "  ? - help                                     ",
             "                                               ",
             "                                               ",
};

#define HELP_STR_SIZE sizeof(help_strings) / sizeof(help_strings[0])

/**
 ******************************************************************
 *
 * Function Name : Oncore_Display constructor
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
Oncore_Display::Oncore_Display(void)
{
    SET_DEBUG_STACK;
    /* Store the this pointer */
    fOncore_Display = this;
    fRun = true;
    fDisplayData = true;

    initscr();
    start_color();
    init_pair( 1, COLOR_YELLOW, COLOR_BLUE);
    init_pair( 2, COLOR_BLUE  , COLOR_YELLOW);
    init_pair( 3, COLOR_BLUE  , COLOR_WHITE);
    /* 
     * newwin arguments - 
     * Number lines
     * ncolumns
     * begin_y
     * begin_x
     */
    fVin = newwin(24,80,0,0);
    refresh();
    fCurrentScreen = POSITION_SCREEN;
    main_frame();

    WriteMsgToScreen("Start Display....");
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : Oncore_Display destructor
 *
 * Description : clean up all the ncurses stuff. 
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : none
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
Oncore_Display::~Oncore_Display(void)
{
    SET_DEBUG_STACK;
    WriteMsgToScreen("End Display....   ");
    delwin(fVin);
    endwin();
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : main_frame
 *
 * Description :  Paint the main frame of the system
 *
 * Inputs : NONE
 *
 * Returns : NONE
 *
 * Error Conditions : NONE
 * 
 * Unit Tested on: 22-Feb-22
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void Oncore_Display::main_frame(void)
{
    SET_DEBUG_STACK;
    uint32_t i;
    int x = 0;
    int y = 0;

    werase(fVin);

    for (i = 0; i < MAINFRAMEWINSIZ; i++ )
    {
	wmove( fVin, x+i, y);
        wprintw(fVin, "%s", main_frame_window[i]);
    }

    switch (fCurrentScreen)
    {
    case POSITION_SCREEN:
	x = 3;
	y = 2;
        for (i = 0; i < POS_STR_SIZE; i++ )
        {
            wmove( fVin, x+i, y);
	    wprintw( fVin, "%s",pos_strings[i]);
        }
        break;
    case HELP_SCREEN:
	x = 3;
	y = 2;
        for (i = 0; i < HELP_STR_SIZE; i++ )
        {
            wmove( fVin, x+i, y);
	    wprintw( fVin, "%s",help_strings[i]);
        }
	break;
    default:
	break;
    }

    /* set background color */
    wbkgd(fVin, COLOR_PAIR(1));

    /* push the output to the screen */
    wrefresh(fVin);
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : Update
 *
 * Description : This is called when an entire TSIP frame has been 
 * received and processed. This call will determine which screen is
 * active and parse the correct data. 
 *
 * Inputs : NONE
 *
 * Returns : NONE
 *
 * Error Conditions : NONE
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void Oncore_Display::Update(const PositionStatus* pPS, const RAIM* pRaim )
{
    SET_DEBUG_STACK;

    switch (fCurrentScreen)
    {
    case HELP_SCREEN:
	return;
	break;
    case POSITION_SCREEN:

	display_position(pPS->Latitude(), pPS->Longitude(), 
			 pPS->Altitude(), pPS->Velocity(), pPS->Heading());
	display_details(0, pPS->NSAT(), pPS->DOP(), pPS->TDOP(), 0);
	display_time(pPS->Time().tv_sec, pPS->GetDelta());
	break;
    }

    SET_DEBUG_STACK;
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
void Oncore_Display::display_time(time_t gpstime, double delta)
{
    SET_DEBUG_STACK;
    int row = STATUS_AREA-1;
    int col = LEFT_AREA;
    char timestr[128], tmp[32];
    strftime( timestr, sizeof(timestr), "%F %H:%M:%S ", 
	      localtime(&gpstime));
    sprintf(tmp, " %g", delta);
    strcat( timestr, tmp);
    wmove  (fVin, row, col);
    wprintw(fVin, "%s", timestr);
    row++;
    SET_DEBUG_STACK;
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
void Oncore_Display::display_details(int mode, int NSV, double dop, double Tdop, int status)

{
    SET_DEBUG_STACK;
    char sv[40], tmp[8];

    int row = STATUS_AREA;
    int col = RIGHT_AREA;
    int i;
    wmove  (fVin, row, col);
    wprintw(fVin, "%d", mode);
    row++;

    wmove  (fVin, row, col);
    wprintw(fVin, "%2d", NSV);
    row++;

    /* PLACEHOLDER */
    memset(sv, 0, sizeof(sv));
    sprintf(sv, "%2d ", 0);
#if 0
    for (i=1;i<nsvs;i++)
    {
	sprintf(tmp, "%2d ", sv_prn[i]);
	strcat(sv, tmp);
    }
#endif
    wmove  (fVin, row, col);
    wprintw(fVin, "%s", sv);
    row++;

    wmove  (fVin, row, col);
    wprintw(fVin, "%8.2f", dop);
    row++;

    wmove  (fVin, row, col);
    wprintw(fVin, "%8.2f", Tdop );
    row++;

    wmove  (fVin, row, col);
    wprintw(fVin, "%d", status);
    row++;

    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name : display_position
 *
 * Description :
 *      Display all the GGA data
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
void Oncore_Display::display_position(double lat, double lon, double alt,
				      double speed, double heading)
{
    SET_DEBUG_STACK;
    int row, col;
    char tmpstr[64];

    row = STATUS_AREA;
    col = LEFT_AREA;
    
    wmove  (fVin, row, col);
    wprintw(fVin, "%s", str_lat(lat*DegToRad, tmpstr));
    row++;
    
    wmove  (fVin, row, col);
    wprintw(fVin, "%s", str_lon(lon*DegToRad, tmpstr));
    row++;
    
    wmove  (fVin, row, col);
    wprintw(fVin, "%6.2f", alt);
    row++;

    wmove  (fVin, row, col);
    wprintw(fVin, "%6.2f", speed);
    row+=5;

    wmove  (fVin, row, col);
    wprintw(fVin, "%6.2f", heading);

    /* set background color */
    wbkgd(fVin, COLOR_PAIR(1));
    /* push the output to the screen */
    wrefresh(fVin);
    SET_DEBUG_STACK;
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
void Oncore_Display::WriteMsgToScreen(const char *s)
{
    static char msg[74];
    static int row = MESSAGE_AREA;

    SET_DEBUG_STACK;
    size_t n = strlen(s);
    if (n<1)
	return;

    if (n>sizeof(msg)) n = sizeof(msg);

    /* Clear out anything that is residual. */
    strncpy( msg, s, sizeof(msg));
    memset( &msg[n-1], 0x20, sizeof(msg)-n);

    wmove(fVin,row,2);
    row--;
    if (row == (STATUS_AREA + STATUS_HEIGHT)) row = MESSAGE_AREA;

    /* print something to the window. Standard printf like format */
    wprintw(fVin, "%s", msg);
    /* set background color */
    wbkgd(fVin, COLOR_PAIR(1));

    /* push the output to the screen */
    wrefresh(fVin);
    SET_DEBUG_STACK;
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
void Oncore_Display::display_message (const char *fmt, ...)
{
    SET_DEBUG_STACK;
    va_list p;
    char s[256], c[80], *cp;
    static int once = FALSE;

    va_start(p, fmt);
    vsprintf(s, fmt, p);
    va_end(p);
    
    if ((!once) && (fCurrentScreen == 0))
    {
	once = TRUE;
	main_frame();
    }
    sprintf(c, "%-55.55s", s);
    if ((cp = strchr(s, '\n' )) != NULL)
    {
	*cp = '\0';
    }
    WriteMsgToScreen(s);
    SET_DEBUG_STACK;
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
void Oncore_Display::ClearCommandArea(void)
{
    SET_DEBUG_STACK;
    CommandCol = 2;
    wmove( fVin, COMMAND_AREA, CommandCol);
    wprintw(fVin, "                        ");

    /* set background color */
    wbkgd(fVin, COLOR_PAIR(1));

    /* push the output to the screen */
    wrefresh(fVin);
    SET_DEBUG_STACK;
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
void Oncore_Display::DisplayCommandChar(unsigned char c)
{
    SET_DEBUG_STACK;
    wmove( fVin, COMMAND_AREA, CommandCol);    
    wprintw(fVin, "0x%X ", c);
    CommandCol += 5;

    /* set background color */
    wbkgd(fVin, COLOR_PAIR(1));

    /* push the output to the screen */
    wrefresh(fVin);
    SET_DEBUG_STACK;
} 

/**
 ******************************************************************
 *
 * Function Name : ParseHomeKeys
 *
 * Description : Parse input keys pertaining to home page. 
 *
 * Inputs : character
 *
 * Returns : none
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
void Oncore_Display::ParseHomeKeys( char c)
{
    SET_DEBUG_STACK;
    switch(c)
    {
    case 'f':
    case 'F':
	display_message("File number advance.\n");
	break;
    case 'o':
    case 'O':
    case 'p':
    case 'P':
    case 't':
    case 'T':
	display_message("Option not available.\n");
	break;
    default:
	break;
    }
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : check_keys
 *
 * Description : default screen is position status data. 
 * Changing such that the keys are checked based on which screen is active. 
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
int Oncore_Display::checkKeys(void)
{
    SET_DEBUG_STACK;
    int rc = 0;

    /* get a character from the window. */
    int c = wgetch(fVin);

    if (c != '\0')
    {
	switch (c)
	{
	case 0:
	    break;
	case 'd':
	case 'D':
	    fDisplayData = !fDisplayData;
	    break;
	case '?':
	    fCurrentScreen = HELP_SCREEN;
	    main_frame();
	    break;
	case 'f':
	case 'F':
	    // File name change requested. 
	    //On::GetThis()->UpdateFileName();
	    break;
	case 'h':
	case 'H':
	    fCurrentScreen = POSITION_SCREEN;
	    main_frame();
	    break;
	case 'q':
	case 'Q':
	    /* QUIT */
	    rc = 1;
	    break;
	case 'r':
	case 'R':
	    /* Repaint the screen */
	    main_frame();
	    break;
	default:
	    switch(fCurrentScreen) 
	    {
	    default:
		ParseHomeKeys(c);
		break;
	    }
	    break;
	}
    }
    SET_DEBUG_STACK;
    return rc;
}

/**
 ******************************************************************
 *
 * Function Name : DisplayThread
 *
 * Description : A thread to update the display and check the 
 * keys in the display as necessary. 
 *
 * Inputs : void* arg - not used. 
 *
 * Returns : NONE
 *
 * Error Conditions : NONE
 * 
 * Unit Tested on: 6-Mar-19
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void* DisplayThread(void *arg)
{
    SET_DEBUG_STACK;
    int rv;
    const struct timespec sleeptime = {1L, 200000000L};
    Oncore_Display *pDisp = Oncore_Display::GetThis();
    CLogger::GetThis()->LogTime("Display thread starts.\n");

    while (pDisp->IsRunning())
    {
	/*
	 * Check to see if the user has requested
	 * special changes in the setup
	 */
	rv = pDisp->checkKeys();
	if (rv>0)
	{
	    pDisp->WriteMsgToScreen("QUIT");
	    pDisp->Stop();
	    /* Command a graceful exit to the program. */
	    GPS::GetThis()->Stop(); 
	}
	else
	{
	    nanosleep( &sleeptime, NULL);
	    SET_DEBUG_STACK;
	    //pDisp->WriteMsgToScreen("UPDATE");
	}
    }
    CLogger::GetThis()->LogTime("Display thread stops.\n");
    SET_DEBUG_STACK;
    return NULL;
}

