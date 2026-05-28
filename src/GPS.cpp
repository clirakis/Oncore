/********************************************************************
 *
 * Module Name : GPS.cpp
 *
 * Author/Date : C.B. Lirakis / 01-Feb-11
 *
 * Description : Generic GPS
 *
 * Restrictions/Limitations :
 *
 * Change Descriptions :
 * 18-Feb-24  Major upgrade to how the sentances are recieved over the
 *            serial line and subsequently processed. 
 *
 *            Took out simple buffer and used utility Buffered. 
 *            This allows me to drain the information off the buffer
 *            with conversion utilties. This is in the Oncore Library. 
 *
 * Classification : Unclassified
 *
 * References :
 * https://synergy-gps.com/wp-content/uploads/2018/11/ut-engg-notes.pdf
 *
 ********************************************************************/
// System includes.

#include <iostream>
using namespace std;
#include <string>
#include <cmath>

// Local Includes.
#include "debug.h"
#include "CLogger.hh"
#include "GPS.hh"
#include "UserSignals.hh"
#include "ProcessTime.hh"
#include "User.hh"
#include "SharedMem2.hh"
#include "NMEA_GPS.hh"

const  char LF = 0x0A;
const  char CR = 0x0D;

GPS* GPS::fGPS;

/*
 * The USER program does logging and provides HTML support. 
 */
#define USE_USER 1

/**
 ******************************************************************
 *
 * Function Name : GPS constructor
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
GPS::GPS (const char *SerialPortName) : Oncore(), SerialIO(SerialPortName, 
							   B9600, 
							   SerialIO::NONE,  
							   SerialIO::ModeRaw, 
							   2, 2)
{
    SET_DEBUG_STACK;
    char msg[128];
    CLogger* plogger = CLogger::GetThis();

    SetName("Oncore");
    SetError(); // No error.

    fGPS  = this;
    fUser = NULL;
    fRun  = true;

    if (SerialIO::CheckError())
    {
	sprintf(msg, "Error opening serial port: %s\n", SerialPortName);
	plogger->LogError(__FILE__,__LINE__,'F',msg);
	SetError(SerialIO::BadOpen);
    }
    else
    {
	plogger->LogTime("Serial port: %s open.\n", SerialPortName);
    }

    fProcessTime = new ProcessTime(plogger->GetVerbose());

#if USE_USER==1
    fUser = new User();
#endif

    fGGA = NULL;
    // 28-Apr-24 CBL, make shared memory to hold 'GGA' data. 
    pSM_PositionData = new SharedMem2("GGA", 
				      GGA::DataSize(), true);
    if (pSM_PositionData->CheckError())
    {
	plogger->LogError(__FILE__, __LINE__, 'W',
			 "GGA data SM failed.");
	delete pSM_PositionData;
	pSM_PositionData = 0;
	SET_DEBUG_STACK;
	SetError(-1);
    }
    else
    {
	plogger->Log("# GGA SM successfully created.\n");
	fGGA = new GGA();
    }
    

    plogger->LogTime("Oncore initalization complete.\n");
    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name : GPS destructor
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
GPS::~GPS (void)
{
    SET_DEBUG_STACK;
    delete fProcessTime;
    delete fUser;
    delete pSM_PositionData;
    delete fGGA;

}

/**
 ******************************************************************
 *
 * Function Name : ReadByByte
 *
 * Description : Read a byte at a time and make a full buffer. 
 *
 * Inputs : NONE
 *
 * Returns : true on succeessfully filling buffer, false on EWOULDBLOCK
 *
 * Error Conditions : read error
 * 
 * Unit Tested on: 18-Feb-24
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
bool GPS::ReadByByte(void)
{
    SET_DEBUG_STACK;
    const struct timespec sleeptime = {0L, 100000000L};
    const unsigned char match[4]    = {0x0d, 0x0a, 0x40, 0x40};

    unsigned char val, *ptr;
    int32_t       rv;
    bool          run = true;
    uint32_t      BigCount = 0;
    bool          rc = true;

    /* reset input buffer to zero. */
    fBuffer->Reset();

    do
    {
	if(Read(&val))
	{
	    BigCount = 0;   // Reset timeout on read. 

	    // Put the value into the buffer and increment the counter. 
	    if(fBuffer->Put(val) > 4)
	    {
		ptr = fBuffer->GetData();
		ptr += (fBuffer->GetFillIndex() - 4); // Backup a bit. 
		rv = memcmp(ptr, match, sizeof(match));
		if (rv==0)
		{
		    // end of old data, start of new. 
		    run = false;
		    rc  = true;
		}
	    }
	}
	else
	{
	    // nothing available, wait a moment. 
	    nanosleep( &sleeptime, NULL);
	    BigCount++; 
	    run = (BigCount<20);
	    rc  = run;
	}
    } while (!fBuffer->IsFull() && run);

    // when first starting the @@ prefix is still attached. 
    // deal with it downstream. 

    SET_DEBUG_STACK;
    return rc;
}

/**
 ******************************************************************
 *
 * Function Name : Update
 *
 * Description : Receive and decode the data from the GPS
 * receiver. 
 *
 * Inputs : NONE
 *
 * Returns : NONE
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
void GPS::Update(void)
{
    SET_DEBUG_STACK;
    CLogger*        plogger = CLogger::GetThis();
    unsigned char*  command;
#if 0
    unsigned char   commanddata[4];
    uint32_t        count = 0;
    PositionStatus *pPS;
#endif
    unsigned char  *data;
    int             n,m;
    struct timespec tstime;
    time_t          fixtime;
    float           milli;


    // Startup, send RAIM setup message.
    GetRAIM()->MessageRate(15); // every 15 seconds
    GetRAIM()->RAIMOnOff(1);    // Turn it on.
    GetRAIM()->PPSMode(1);      // Continious
    GetRAIM()->Alarm(20);
    data = GetRAIM()->Message(false);
    command = MakeCommand("En", data, 15);
    n = CommandSize();
    m = Write(command, n);
    if (n != m)
    {
	plogger->LogTime("# ERROR writing RAIM command.\n");
    }
            
    plogger->LogTime("Sent RAIM request.\n");

    // Loop while program is enabled. 
    while (fRun) 
    {
	if (ReadByByte())
	{
	    /*
	     *  we have a valid sentance of some type. 
	     *  parse what is in the buffer. 
	     */
	    Parse();

	    if (TimeValid())
	    {
		if (plogger->GetVerbose()>0)
		{
		    plogger->LogTime("Time Processed.\n");
		}
		/* 
		 * 28-Apr-24 CBL, new GGA message. 
		 */

		if (fGGA && pSM_PositionData)
		{
		    // Populate GGA message structure. 
		    tstime = PCTime();
		    fGGA->SetPCTime(tstime);
		    fGGA->SetLatitude(GetPS()->Latitude()*M_PI/180.0);
		    fGGA->SetLongitude(GetPS()->Longitude()*M_PI/180.0);
		    fixtime = GetPS()->Time().tv_sec;
		    milli   = GetPS()->Time().tv_nsec;
		    milli   = milli * 1.0e-9;
		    fGGA->SetTime(fixtime);   
		    fGGA->SetUTC(GetPS()->GetUTC());
		    fGGA->SetMilliseconds( milli); 
		    fGGA->SetGeoidHeight(0.0);   // FIXME
		    fGGA->SetAltitude(GetPS()->Altitude());
		    fGGA->SetFix(GetPS()->Status());   //FIXME -- need to translate
		    fGGA->SetNSat(GetPS()->NSAT());
		    fGGA->SetHDOP(GetPS()->DOP());      // is this HDOP?
		    pSM_PositionData->PutData(fGGA->DataPointer());
		}

		/*
		 * Update the time shared memory segments. 
		 * Process the data as necessary. 
		 * GPSTime() and GetDelta() are both part
		 * of the Oncore class 
		 * Both are filled as part of PositionStatus. 
		 */
		fProcessTime->Update(GPSTime(), GetDelta());

		// Do any user stuff here. 
		if (fUser)
		{
		    fUser->Update(GetDelta());
		}
		TimeProcessed();  // reset the PositionStatus class

#if 0
		// Setup to get, not sure what....
		count++;
		if (count == 10)
		{
		    commanddata[0] = 0xff;
		    command = MakeCommand("AP", commanddata, 1);
		    n = CommandSize();
		    m = Write(command, n);
		    if (n != m)
		    {
			plogger->LogError(__FILE__,__LINE__,'W',
					 "ERROR writing command.");
		    }
		
		    if (plogger->GetVerbose()>0)
		    {
			plogger->LogTime("Sent request.\n");
		    }
		    count = 0;
		}
#endif
	    }
	}
    } 
    SET_DEBUG_STACK;
}

