/********************************************************************
 *
 * Module Name : Terminate.cpp
 *
 * Author/Date : C.B. Lirakis / 05-Mar-19
 *
 * Description : Terminate to be called from anywhere to do proper clean up
 *
 * Restrictions/Limitations :
 *
 * Change Descriptions :
 *
 * Classification : Unclassified
 *
 * References :
 *
 ********************************************************************/
// System includes.

#include <iostream>
using namespace std;
#include <string>
#include <cmath>
#include <cstring>
#include <unistd.h>
#include <csignal>


// Local Includes.
#include "debug.h"
#include "CLogger.hh"
#include "Terminate.hh"
#include "GPS.hh"


/**
 ******************************************************************
 *
 * Function Name : Terminate
 *
 * Description : Deal with errors in a clean way!
 *               ALL, and I mean ALL exits are brought 
 *               through here!
 * 
 * Inputs : Signal causing termination. 
 *
 * Returns : none
 *
 * Error Conditions : Well, we got an error to get here. 
 *
 *******************************************************************
 */ 
void Terminate (int sig) 
{
    static int i=0;
    CLogger *logger = CLogger::GetThis();
    char msg[256], tmp[128];
    time_t now;
    time(&now);
    //struct tm *tm_now = localtime(&now);

 
    i++;
    if (i>1) 
    {
        _exit(-1);
    }

    switch (sig)
    {
    case -1: 
      sprintf( msg, "User abnormal termination");
      break;
    case 0:                    // Normal termination
        sprintf( msg, "Normal program termination.");
        break;
    case SIGHUP:
        sprintf( msg, " Hangup");
        break;
    case SIGINT:               // CTRL+C signal 
        sprintf( msg, " SIGINT ");
        break;
    case SIGQUIT:               //QUIT 
        sprintf( msg, " SIGQUIT ");
        break;
    case SIGILL:               // Illegal instruction 
        sprintf( msg, " SIGILL ");
        break;
    case SIGABRT:              // Abnormal termination 
        sprintf( msg, " SIGABRT ");
        break;
    case SIGBUS:               //Bus Error! 
        sprintf( msg, " SIGBUS ");
        break;
    case SIGFPE:               // Floating-point error 
        sprintf( msg, " SIGFPE ");
        break;
    case SIGKILL:               // Kill!!!! 
        sprintf( msg, " SIGKILL");
        break;
    case SIGSEGV:              // Illegal storage access 
        sprintf( msg, " SIGSEGV ");
        break;
    case SIGTERM:              // Termination request 
        sprintf( msg, " SIGTERM ");
        break;
    case SIGTSTP:               // 
        sprintf( msg, " SIGTSTP");
        break;
    case SIGXCPU:               // 
        sprintf( msg, " SIGXCPU");
        break;
    case SIGXFSZ:               // 
        sprintf( msg, " SIGXFSZ");
        break;
    case SIGSTOP:               // 
        sprintf( msg, " SIGSTOP ");
        break;
    case SIGSYS:               // 
        sprintf( msg, " SIGSYS ");
        break;
#ifndef MAC
     case SIGPWR:               // 
        sprintf( msg, " SIGPWR ");
        break;
    case SIGSTKFLT:               // Stack fault
        sprintf( msg, " SIGSTKFLT ");
        break;
#endif
   default:
        sprintf( msg, " Uknown signal type: %d", sig);
        break;
    }
    if (sig!=0)
    {
        sprintf ( tmp, "%s %s %d", asctime(localtime(&now)),
					   LastFile, LastLine);
        strncat ( msg, tmp, sizeof(msg));
	logger->Log("# %s\n",msg);
    }

    // User termination here
    GPS *p = GPS::GetThis();
    delete p;

    delete logger;

    if (sig == 0)
    {
        _exit (0);
    }
    else
    {
        _exit (-1);
    }
}
