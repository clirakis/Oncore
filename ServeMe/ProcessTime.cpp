/**
 ******************************************************************
 *
 * Module Name : time_thread.cpp
 *
 * Author/Date : C.B. Lirakis / 10-Oct-03
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
 * $RCSfile: time_thread.cpp,v $
 * $Author: clirakis $
 * $Date: 2004/01/04 18:34:53 $
 * $Locker: clirakis $
 * $Name:  $
 * $Revision: 1.2 $
 *
 * $Log: time_thread.cpp,v $
 * Revision 1.2  2004/01/04 18:34:53  clirakis
 * This now has the library struct timespec usage for distributing the
 * time between GPS and local.
 *
 * Revision 1.1  2003/12/31 14:39:57  clirakis
 * Initial revision
 *
 *
 *
 *******************************************************************
 */

#ifndef lint
/// RCS Information
static char rcsid[]="$Header: /home/clirakis/code/lassen/RCS/time_thread.cpp,v 1.2 2004/01/04 18:34:53 clirakis Exp clirakis $";
#endif

// System includes.
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <sys/time.h>
using namespace std;

/// Local Includes.
#include "debug.h"
#include "ProcessTime.hh"
#include "Average.hh"

extern ofstream logFile;

const double MaxDeltaSeconds = 3.0 * 86000.0;
#define DEBUG       0
#define AVG_WINDOW 30
#define SET_THRESH 0.05

/**
 ******************************************************************
 *
 * Function Name : ProcessTime constructor
 *
 * Description : Create a SM segment to store the difference
 * between the GPS time and the current clock time. Also, 
 * make this a little bigger so other elements can communicate
 * between this and other processes. 
 *
 * Inputs : none
 *
 * Returns :
 *
 * Error Conditions :
 *
 *******************************************************************
 */
ProcessTime::ProcessTime()
{
    SET_DEBUG_STACK;
    TimeSM   = 0;
    GPSDelta = 0;
    memset(&oldgpstime,0, sizeof(t_GPS_Time));
    errStr   = NULL;
    error    = 0;
    verbose  = 0;

    /*
     * This is the data containing the GPS delta information.
     * This information is used by the BC60 UI
     *
     * Open shared memory to share with the world our data. 
     * The data is organized as follows. 
     * The int data, primary pointer, is the current average count
     *     float data is the average delta
     *     double data is the current delta. 
     */
    GPSDelta = new  SharedMem('H', sizeof(int), "gpstime");
    if (GPSDelta->GetError() != SharedMem::NO_ERROR)
    {
	errStr = "Error creating GPS shared memory.";
	delete GPSDelta;
	GPSDelta = 0;
	error = -1;
	SET_DEBUG_STACK;
	return;
    }

    /*
     * If our timemon process is running, it should have
     * created the shared memory segment, we are attaching to it. 
     */
    TimeSM =  new SharedMem( 'H', "timemon");
    if (TimeSM->GetError() != SharedMem::NO_ERROR)
    {
	errStr = "Error connecting to timemon SM.";
	delete TimeSM;
	TimeSM = 0;
	error  = -2;
	SET_DEBUG_STACK;
	return;
    }

    OldTimeDelta   = LastTimeSet = 0;
    NumberTimesSet = 0;
    time(&LastTimeSet);

    data     = new Average(AVG_WINDOW);
    TimeSetWindow = MaxDeltaSeconds;

    if (logFile)
	logFile <<"Time initialize complete."<<endl;
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : ProcessDelta
 *
 * Description : Take the delta and look at it. If it is huge, 
 * but within a day bounds wise, force an immediate set. Otherwise
 * hang out and wait for the average smoothed delta to exceed its
 * value
 *
 * Inputs : delta to monitor
 *
 * Returns : none
 *
 * Error Conditions : none
 * 
 * Unit Tested on:  26-Feb-06
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void ProcessTime::ProcessDelta()
{
    SET_DEBUG_STACK;
    double        avg;
    time_t        now;
    unsigned long dnow;

    /* Used for logging */
    time (&now);

    /*
     * By the time we get here we know that the time difference
     * between the PC clock and the GPS clock is within MaxDeltaSeconds
     * seconds. Now we want to make sure that threre isn't an
     * aberent spike in the delta betweeen the two, otherwise the
     * system starts going into oscillation. 
     *
     * Right off the bat, we should tolerate huge deltas, but once
     * the first set occurs, then clamp down. 
     */
    if (abs(TimeDelta) < TimeSetWindow )
    {
	/*
	 * This should smooth the data some. 
         * So add the time delta to the running average.
	 */
	data->AddElement(TimeDelta);
	avg = data->Get();
#if DEBUG
	logFile << __FILE__ << " " << __LINE__ << " "
	        << dec 
		<< " CurrentDelta: " << TimeDelta
		<< " AVG. Delta: " <<  avg
		<< endl;
#endif
	/* Don't go any further until the averge has smoothed. */
	if (!data->GetFirstFill())
	{
	    return;
	}

	if (fabs(avg)>SET_THRESH)
	{
	    NumberTimesSet++;
	    dnow = now - LastTimeSet;
	    time(&LastTimeSet);

            /*
             * Regardless of what the time set window was before
             * clamp it down.  If we set often enough, 
             * we shouldn't expect hideous values. 
             * Make sure that initially we allow some oscillations. 
             */
            if (NumberTimesSet < 2)
            {
                TimeSetWindow = 30.0;
            }
            else if (NumberTimesSet < 3)
            {
                TimeSetWindow = 2.0;
            }
            else
            {
                TimeSetWindow = 1.0;
            }
#if DEBUG
	    logFile << "SET DATA Delta: "
		    << avg
		    << " Number times set: " << NumberTimesSet
		    << " time between set: " << dnow
		    << " avg: " << avg
		    << endl;
#endif
            /*
             * If the time shared memory exists, publish the
             * delta so the daemon can do the set. 
             */
	    if (TimeSM)
	    {
		TimeSM->PutData(avg);
	    }
	    data->VReset();
	}
    }
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : ProcessTime
 *
 * Description :
 *
 * Inputs : gpstime - the time from the gps message
 *          pctime  - pc time at the hack of the recipt of the gps time message
 *
 * Returns :
 *
 * Error Conditions :
 *
 *******************************************************************
 */
bool ProcessTime::DoIt( t_GPS_Time gpstime, const PreciseTime &pctime)
{
    SET_DEBUG_STACK;
    static int      navg;
    PreciseTime     pt_gps;
    double          sub;
    unsigned int    udelta;
    void*           ptr;
    struct timeval  ttv;
    struct timezone ttz;

    gettimeofday( &ttv, &ttz);

    udelta = (int) (gpstime.TimeOfWeek - oldgpstime.TimeOfWeek);
    /*
     * The gpstime is not updated on a regular 1s interval.
     * Checking udelta (unsigned delta) to be non-zero
     * makes sure that the time set/check logic only gets called 
     * when there is a new GPS time availale. 
     */
    if (udelta != 0L)
    {
	pt_gps = DateFromGPSTime(gpstime.TimeOfWeek, 
				 gpstime.ExtendedWeek, 
				 gpstime.UTC_Delta);
        /*
         * Logging depending on verbosity. 
         */
	if(verbose > 2)
	{
	    logFile << "GPS " << pt_gps 
		    << " PC " << pctime
		    << " UDelta: "  << udelta
		    << endl;
	}

        /*
         * if this is negative, this represents a bad decode in the 
         * lassen GPS library.
         */
	if (pt_gps.GetSeconds() < 0)
	{
	    logFile <<" Bad GPS time conversion." << pctime << " "
		    << gpstime.TimeOfWeek << " " 
		    << gpstime.ExtendedWeek << " "
		    << gpstime.UTC_Delta
		    << endl;
	    return false;
	}
        /* 
         * Take the difference. 
         * pt_gps is the GPS time
         * pctime is the time of the PC clock at the moment the
         *   pt_gps was decoded.
         */
        TimeDelta  = pt_gps.GetNanoSeconds();
        TimeDelta /= 1.0E9;
        TimeDelta +=  pt_gps.GetSeconds();
        sub    = pctime.GetNanoSeconds();
        sub   /= 1.0E9;
        sub   += pctime.GetSeconds();
        /* 
         * This is the difference between the PC and the gps
         * at the time of the GPS time decode.
         */
        TimeDelta -= sub;
        /*
         * Correct for daylight savings if set. 
        */
       if (ttz.tz_dsttime != 0)
       {
           TimeDelta -= 3600;
       }
       /*
        * Put in an upper bound time delta processing. Occasionally, 
        * the time decoded from the receiver is bad. 
        * Check to see that we are within say, a few days in our bounds.
        */
        double compare = fabs(TimeDelta-OldTimeDelta);
        OldTimeDelta = TimeDelta;
        if (compare > MaxDeltaSeconds)
        {
	    logFile << " Unmanageable time delta: " << TimeDelta
		    << endl;
	    return false;
        }
       /*
        * Process the average and send it to timemon. 
        * Timemon is a process that is run in the superuser
        * state so it has the permission to set the system clock.
        */
       ProcessDelta( );

        /*
         * Send the data to the BC620 UI through shared memory.
         */
        if (GPSDelta)
        {
            navg = data->GetCurrentPointer();
            ptr  = GPSDelta->PutData( TimeDelta, (float) data->Get(), 
                (void *)&navg);
        }
        else
        {
            return false;
        }
        oldgpstime = gpstime;
    }
    SET_DEBUG_STACK;
    return true;
}
/**
 ******************************************************************
 *
 * Function Name : ProcessTime desctructor
 *
 * Description : Cleans up after time thread. 
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : none
 * 
 * Unit Tested on: 8-Oct-07
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
ProcessTime::~ProcessTime()
{
    SET_DEBUG_STACK;
    delete TimeSM;
    delete GPSDelta;
    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name : DumpData
 *
 * Description : If the logfile is open, prints out how the process
 * is running
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : none
 * 
 * Unit Tested on: 28-June-08
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void ProcessTime::DumpData()
{
}
