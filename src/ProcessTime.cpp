/**
 ******************************************************************
 *
 * Module Name : ProcessTime.cpp
 *
 * Author/Date : C.B. Lirakis / 10-Oct-03
 *
 * Description :
 *
 * Restrictions/Limitations :
 *
 * Change Descriptions : 06-Aug-10 in support of Oncore machine
 * 21-Sep-20    CBL    Updated to use SharedMem2 class
 *
 * Classification : Unclassified
 *
 * References :
 *
 *******************************************************************
 */
// System includes.
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <cstdint>
#include <sys/time.h>
using namespace std;
#include <string.h>

/// Local Includes.
#include "debug.h"
#include "CLogger.hh"
#include "ProcessTime.hh"
#include "Average.hh"

// 3 Days
const double MaxDeltaSeconds = 3.0 * 86000.0;
const int    WindowSamples   = 30;
const double SetPC_Threshold = 0.1; // Number of seconds to force set.


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
ProcessTime::ProcessTime(bool verbose)
{
    SET_DEBUG_STACK;
    CLogger* logger = CLogger::GetThis();
    fTimeSM   = NULL;
    fGPSDelta = NULL;
    memset(&fOldTime, 0, sizeof(struct timespec));
    fError    = 0;
    fVerbose  = verbose;

    logger->Log("# Process Time data format:\n");
    logger->Log("# ---------------------------------------------------\n");
    logger->Log("# message type: 1 - delta seconds, 2 - window        \n");
    logger->Log("# 3 -                                                \n");
    logger->Log("#    Number of seconds since epoch on host computer. \n");
    logger->Log("#    Formatted host time. m d y H M S                \n");
    logger->Log("#    current average delta between host and GPS.     \n");
    logger->Log("#    Number of times set has occured.                \n");
    logger->Log("#    number of seconds elapsed since last time set.  \n");
    logger->Log("# ---------------------------------------------------\n");


    /*
     * This is the data containing the GPS delta information.
     * Open shared memory to share with the world our data. 
     * The data is organized as follows. 
     * The long data, primary pointer, is the GPS seconds
     *     long data, GPS nanoseconds
     *     double data is the current delta. 
     * set this up as a server of data. 
     */

    fGPSDelta = new  SharedMem2("GPSDelta", sizeof(struct timespec), true);
    if (fGPSDelta->CheckError())
    {
	logger->LogError(__FILE__, __LINE__, 'W',
			 "Error attaching gpstime memory.");
        delete fGPSDelta;
        fGPSDelta =  NULL;
        fError    = -1;
        SET_DEBUG_STACK;
    }
    else
    {
	logger->LogTime("Attached to gpstime memory.\n");
    }

    /*
     * If our timemon process is running, it should have
     * created the shared memory segment, we are attaching to it. 
     * This is in a client capacity. 
     *
     * This may be a difficult one to connect to. 
     */
    fTimeSM =  new SharedMem2( "timemon", sizeof(float), false, 0);
    if (fTimeSM->CheckError())
    {
	logger->LogError(__FILE__,__LINE__, 'W',
			 "Error attaching timemon memory.");
        delete fTimeSM;
        fTimeSM     =  NULL;
        fError      = -2;
        SET_DEBUG_STACK;
    }
    else
    {
        logger->LogTime("Attached to timemon memory.\n");
    }

    fWindowHigh     = MaxDeltaSeconds; 
    fWindowLow      = SetPC_Threshold;
    fNumberTimesSet = 0;
    time(&fLastTimeSet);

    fData = new Average(WindowSamples);
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
void ProcessTime::ProcessDelta(double delta)
{
    SET_DEBUG_STACK;
    CLogger* logger = CLogger::GetThis();
    double        avg;
    time_t        now;

    time(&now);
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

    // Always get the average for publication.
    avg = fData->Get();
    if (fVerbose)
    {
        logger->LogTime("1 %f\n", delta);
    }

    if (abs(delta) < fWindowHigh)
    {
        /*
         * This should smooth the data some. 
         * So add the time delta to the running average.
         */
        fData->AddElement(delta);
        // Should we get this far, get the new averate to see if it is
        // worth using to set the PC clock.
        avg = fData->Get();
        /* Don't go any further until the averge has smoothed. */
        if (fData->FirstFill())
        {
            if (fabs(avg)>fWindowLow)
            {
                fNumberTimesSet++;
                time(&fLastTimeSet);
                /*
                 * Regardless of what the time set window was before
                 * clamp it down.  If we set often enough, 
                 * we shouldn't expect hideous values. 
                 * Make sure that initially we allow some oscillations. 
                 */
                if (fNumberTimesSet < 2)
                {
                    fWindowHigh = 30.0;
                }
                else if (fNumberTimesSet < 3)
                {
                    fWindowHigh = 2.0;
                }
                else
                {
                    fWindowHigh = 1.0;
                }

                logger->LogTime("2 %f\n",fWindowHigh);
                 /*
		  * If the time shared memory exists, publish the
		  * delta so the daemon can do the set. 
		  */
		if (fTimeSM)
		{
		    fTimeSM->PutData(avg);
		    fData->VReset();
		}
		logger->LogTime("3 %f %f %d \n", delta, avg, fNumberTimesSet);
            }
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
bool ProcessTime::Update(const struct timespec &gps, double delta)
{
    SET_DEBUG_STACK;
    static uint32_t count = 0;
    CLogger* logger = CLogger::GetThis();
    /* 
     * replace timezone with a converstion from time_t to 
     * gmtime which gives struct tm. 
     * Inside struct tm is tm_isdst. This should fix the blinking
     * issue.
     */

    /*
     * Process the average and send it to timemon. 
     * Timemon is a process that is run in the superuser
     * state so it has the permission to set the system clock.
     */
    //ProcessDelta( delta);

    /*
     * Send the data to the BC635 UI through shared memory.
     */
    if (fGPSDelta)
    {
	// Double is fDelta
        //     - float data is average
	//     - Data is structure of actual time
        fGPSDelta->PutData((void *)&gps);
	fGPSDelta->PutData(delta);
	/*
	 * Not sure why, but for some reason on compilation delta seems 
	 * to subtract 3600 unless this code is here. 
	 * suspect some sort of memory overwrite. 
	 */
	if(count%(5*60) == 0)
	{
	    logger->LogTime("GPS Delta: %f\n", delta);
	}
	count++;
	//cout << "PROCESSTIME: " << delta << endl;

	if (fVerbose)
	{
	    char s[40];
	    strftime( s, sizeof(s), "%F %T", localtime(&gps.tv_sec));
	    logger->LogTime("%ld %ld %f %s\n", gps.tv_sec, gps.tv_nsec,
			    delta,s);
	}
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
ProcessTime::~ProcessTime(void)
{
    SET_DEBUG_STACK;
    delete fTimeSM;
    delete fGPSDelta;
    SET_DEBUG_STACK;
}

