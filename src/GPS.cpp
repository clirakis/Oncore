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
 * 28-May-26  Move all the logging in here, Use HDF5 logging format. 
 *            use libconfig++ for configuration 
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
#include <libconfig.h++>
using namespace libconfig;

// Local Includes.
#include "debug.h"
#include "CLogger.hh"
#include "GPS.hh"
#include "UserSignals.hh"
#include "ProcessTime.hh"
#include "SharedMem2.hh"
#include "NMEA_GPS.hh"
#include "Geodetic.hh"
#include "OncoreDisp.hh"

const  char LF = 0x0A;
const  char CR = 0x0D;

GPS* GPS::fGPS;

/*
 * The USER program does logging and provides HTML support. 
 */
#define USE_USER 1

static char kConfigFileName[] = "Oncore.cfg";
static const uint32_t kNVar = 18;
/**
 * thread control for the display if selected. 
 */
static pthread_t d_thread;


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
GPS::GPS (void) : CObject(), Oncore()
{
    SET_DEBUG_STACK;
    char msg[128];
    CLogger* plogger = CLogger::GetThis();

    SetName("Oncore");
    SetError(); // No error.

    fGPS       = this;
    fRun       = true;
    fSerial    = NULL;
    fGeodetic  = NULL;
    fGeoCenter = NULL;
    f5Logger   = NULL;
    fPDisp     = NULL;
    fSerialPortName = string("/dev/ttyUSB0");  // A default. 

    if(!ReadConfiguration())
    {
	SetError(ECONFIG_READ_FAIL,__LINE__);
	return;
    }


    fSerial = new SerialIO(fSerialPortName.c_str(), 
			   B9600, 
			   SerialIO::NONE,  
			   SerialIO::ModeRaw, 
			   2, 2);

    if (fSerial->CheckError())
    {
	sprintf(msg, "Error opening serial port: %s\n", 
		fSerialPortName.c_str());
	plogger->LogError(__FILE__,__LINE__,'F',msg);
	SetError(SerialIO::BadOpen);
    }
    else
    {
	plogger->LogTime("Serial port: %s open.\n", fSerialPortName.c_str());
    }

    fProcessTime = new ProcessTime(plogger->GetVerbose());


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

    if (fLogging)
    {
	fn = new FileName("Oncore", "h5", One_Day);
	OpenLogFile();
    }
    if(fDisplay)
    {
	/* If the user has requested the display feature, start it now. */
	/* create the display. */
	fPDisp = new Oncore_Display();
	if( pthread_create(&d_thread, NULL, DisplayThread, NULL) == 0)
	{
	    plogger->Log("# Display Thread successfully created.\n");
	}
	else
	{
	    SET_DEBUG_STACK;
	    /* It is not the end of the world if this fails. */
	    plogger->Log("# Dispaly Thread failed.\n");
	}
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
    CLogger *pLog = CLogger::GetThis();
    pLog->LogTime("# Oncore Clean up.\n");

    // Kill the display thread.
    fPDisp->Stop();
    delete fPDisp;
    fPDisp = NULL;

    // Do some other stuff as well. 
    if(!WriteConfiguration())
    {
	SetError(ECONFIG_WRITE_FAIL,__LINE__);
	pLog->LogError(__FILE__,__LINE__, 'W', 
		       "Failed to write config file.\n");
    }

    /* Close serial port */
    delete fSerial;

    /* Shut down logging. */
    pLog->LogTime("stop logging\n");
    delete f5Logger;
    f5Logger = NULL;

    delete fProcessTime;
    delete pSM_PositionData;
    delete fGGA;
    delete fGeoCenter;
    delete fGeodetic;

    pLog->LogTime(" Oncore closed.\n");
}
/**
 ******************************************************************
 *
 * Function Name : OpenLogFile
 *
 * Description : Open and manage the HDF5 log file
 *
 * Inputs : none
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
bool GPS::OpenLogFile(void)
{
    SET_DEBUG_STACK;
    const char *Names = "Time:Lat:Lon:Z:NSV:DOP:TDOP:SPEED:HEAD:TS:DT:SDAY:IDAY:EVCount:X:Y:DX:DY";
    /*
     *
     *  0) Time - Seconds since unix epoch from GGA message
     *  1) Lat  - degrees
     *  2) Lon  - degrees
     *  3) Z    - meters
     *  4) NSV  - Number of satelites in view
     *  5) DOP  - ASSUME HDOP
     *  6) TDOP - Time DOP
     *  7) SPEED - speed (units?)
     *  8) HEAD - Not sure if this is true or magnetic
     *  9) TS   - Time Solution
     * 10) DT 
     * 11) SDAY
     * 12) IDAY
     * 13) EVCount - event count
     * 14) X
     * 15  Y
     * 16) DX
     * 17) DY
     */
    CLogger *pLogger  = CLogger::GetThis();
    /* Give me a file name.  */
    const char* name  = fn->GetUniqueName();
    fn->NewUpdateTime();
    SET_DEBUG_STACK;

    f5Logger = new H5Logger(name,"Oncore GPS Dataset", kNVar, false);
    if (f5Logger->CheckError())
    {
	pLogger->Log("# Failed to open H5 log file: %s\n", name);
	delete f5Logger;
	f5Logger = NULL;
	return false;
    }
    f5Logger->WriteDataTags(Names);

    /* Log that this was done in the local text log file. */
    time_t now;
    char   msg[64];
    SET_DEBUG_STACK;
    time(&now);
    strftime (msg, sizeof(msg), "%m-%d-%y %H:%M:%S", gmtime(&now));
    pLogger->Log("# changed file name %s at %s\n", name, msg);

    return true;
}
/**
 ******************************************************************
 *
 * Function Name : UpdateFileName
 *
 * Description : Flush and close current log file, update the name, 
 *               and reopen.
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
void GPS::UpdateFileName(void)
{
    SET_DEBUG_STACK;
    /*
     * flush and close existing file
     * get a new unique filename
     * reset the timer
     * and go!
     *
     * Check to see that logging is enabled. 
     */
    if(f5Logger)
    {
	// This will close and flush the existing logfile. 
	delete f5Logger;
	f5Logger = NULL;
	// Now reopen
	OpenLogFile();
    }
    SET_DEBUG_STACK;
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
	if(fSerial->Read(&val))
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
 * Function Name : LogData
 *
 * Description : Receive and decode the data from the GPS
 * receiver. MAIN LOOP
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
void GPS::LogData(void)
{
    SET_DEBUG_STACK;
    CLogger*           plogger = CLogger::GetThis();
    Oncore_Display*    pDisp   = Oncore_Display::GetThis();
    PositionStatus*    pPS   = GetPS();
    RAIM*              pRAIM = GetRAIM();
    VisibleSatellites* pVS   = GetVS();
    //
    unsigned char*  command;
    time_t          epoch;
    double          SDay;
    uint32_t        Day;
    struct tm       *tm_val;
    unsigned char   *data;
    int             n,m;
    struct timespec tstime;
    time_t          fixtime;
    double          milli;
    uint32_t        count = 0;
    Point           XY;

    // Startup, send RAIM setup message.
    pRAIM->MessageRate(15); // every 15 seconds
    pRAIM->RAIMOnOff(1);    // Turn it on.
    pRAIM->PPSMode(1);      // Continious
    pRAIM->Alarm(20);
    data = pRAIM->Message(false);
    command = MakeCommand("En", data, 15);
    n = CommandSize();
    m = fSerial->Write(command, n);
    if (n != m)
    {
	plogger->LogTime("# ERROR writing RAIM command.\n");
    }
            
    plogger->LogTime("Sent RAIM request.\n");

    // Loop while program is enabled. 
    while (fRun) 
    {
	if(fLogging)
	{
	    /* Check to see if the logging interval has rolled over. */
	    if (fn->ChangeNames())
	    {
		UpdateFileName();
	    }
	}

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
		/* Time crap */
//		tstime  = PCTime();
//		pcmilli = 1.0e-9 * (double) tstime.tv_nsec + (double) tstime.tv_sec;
		fixtime = GetPS()->Time().tv_sec;
		// DeBUG the fixtime is indeed correct
		//plogger->LogTime("fixtime %ld\n", fixtime);
		milli   = GetPS()->Time().tv_nsec;
		milli   = milli * 1.0e-9;

//		dt = (fixtime+milli) - pcmilli;
		epoch  = pPS->Time().tv_sec; 
		tm_val = localtime(&epoch);
		Day    = tm_val->tm_yday;
		SDay   = (tm_val->tm_hour + 60.0 * tm_val->tm_min)*60.0 
		    + tm_val->tm_sec;

		/* Position data **************************** */

		fLatitude  = pPS->Latitude();  // In Degrees
		fLongitude = pPS->Longitude(); //In Degrees

		// Make the forward projection. 
		XY = fGeodetic->ToXY(fLongitude, fLatitude, 0.0);

		/* 
		 * 28-Apr-24 CBL, new GGA message. 
		 */
		if (fGGA && pSM_PositionData)
		{
		    // Populate GGA message structure. 
		    fGGA->SetPCTime(tstime);
		    fGGA->SetLatitude(fLatitude);
		    fGGA->SetLongitude(fLongitude);
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

		TimeProcessed();  // reset the PositionStatus class
		if (fLogging)
		{
		    // Write to HDF5 file. 
		    f5Logger->FillInternalVector((double)fixtime+milli, 0);
		    f5Logger->FillInternalVector(fLatitude, 1);
		    f5Logger->FillInternalVector(fLongitude, 2);
		    f5Logger->FillInternalVector(pPS->Altitude(), 3);
		    f5Logger->FillInternalVector(pPS->NSAT(), 4);
		    f5Logger->FillInternalVector(pPS->DOP(), 5);
		    f5Logger->FillInternalVector(pPS->TDOP(), 6);
		    f5Logger->FillInternalVector(pPS->Velocity(), 7);
		    f5Logger->FillInternalVector(pPS->Heading(), 8);
		    f5Logger->FillInternalVector(pRAIM->TimeSolution(), 9);
		    f5Logger->FillInternalVector(pPS->GetDelta(),10);
		    f5Logger->FillInternalVector(SDay,11);
		    f5Logger->FillInternalVector(Day, 12);
		    f5Logger->FillInternalVector(count, 13);
		    f5Logger->FillInternalVector(XY.X(),14);
		    f5Logger->FillInternalVector(XY.Y(), 15);
		    f5Logger->FillInternalVector(XY.X()-fGeodetic->XY0().X(), 16);
		    f5Logger->FillInternalVector(XY.Y() - fGeodetic->XY0().Y(), 17);

		    f5Logger->Fill();
		}
		count++;
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
		if(pDisp)
		{
		    // A display is available. 
		    pDisp->Update(pPS, pRAIM, pVS);
		}
	    }
	}
    } 
    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name : ReadConfiguration
 *
 * Description : Open read the configuration file. 
 *
 * Inputs : none
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
bool GPS::ReadConfiguration(void)
{
    SET_DEBUG_STACK;
    CLogger *Logger = CLogger::GetThis();
    ClearError(__LINE__);
    Config *pCFG = new Config();

    Logger->Log("# Opening configuration file: %s\n", kConfigFileName);

    /*
     * Open the configuragtion file. 
     */
    try{
	pCFG->readFile(kConfigFileName);
    }
    catch( const FileIOException &fioex)
    {
	Logger->LogError(__FILE__,__LINE__,'F',
			 "I/O error while reading configuration file.\n");
	return false;
    }
    catch (const ParseException &pex)
    {
	Logger->Log("# Parse error at: %s : %d - %s\n",
		    pex.getFile(), pex.getLine(), pex.getError());
	return false;
    }

    /*
     * Start at the top. 
     */
    const Setting& root = pCFG->getRoot();

    // Output a list of all books in the inventory.
    try
    {
	/*
	 * index into group GPS
	 */
	const Setting &GPS = root["GPS"];
	string Port;
	int    Debug;

	GPS.lookupValue("Port",       fSerialPortName);
	GPS.lookupValue("Reset",      fReset);
	GPS.lookupValue("Debug",      Debug);
	GPS.lookupValue("Display",    fDisplay);
	GPS.lookupValue("Logging",    fLogging);

	SetDebug(Debug);

	if (fReset)
	{
	    Logger->Log("# Reset is called for.\n");
	}
    }
    catch(const SettingNotFoundException &nfex)
    {
	// Ignore.
    }

    // Output a list of all movies in the inventory.
    try
    {
	double Lat, Lon;
	/*
	 * index into group Geodetic
	 */
	const Setting &GroupGeodetic = root["Geodetic"];

	GroupGeodetic.lookupValue("Latitude0",  Lat);
	GroupGeodetic.lookupValue("Longitude0", Lon);
	// Make sure that there is no fGeoCenter
	delete fGeoCenter;
	delete fGeodetic;
	fGeoCenter = new Point( Lon, Lat);
        fGeodetic  = new Geodetic(fGeoCenter->Y(), fGeoCenter->X());
	Logger->Log("# Geodetic center set! %f %f %f %f\n", 
		    Lat, Lon, fGeodetic->XY0().X(), fGeodetic->XY0().Y());
    }
    catch(const SettingNotFoundException &nfex)
    {
	// Ignore.
    }
    delete pCFG;
    pCFG = 0;
    SET_DEBUG_STACK;
    return true;
}

/**
 ******************************************************************
 *
 * Function Name : WriteConfigurationFile
 *
 * Description : Write out final configuration
 *
 * Inputs : none
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
bool GPS::WriteConfiguration(void)
{
    SET_DEBUG_STACK;
    CLogger *Logger = CLogger::GetThis();
    ClearError(__LINE__);
    double Lat = 41.3084;
    double Lon = -71.2;
    Config *pCFG = new Config();

    Setting &root = pCFG->getRoot();

    // Add some settings to the configuration.
    Setting &GPS = root.add("GPS", Setting::TypeGroup);
    Setting &GroupGeodetic = root.add("Geodetic", Setting::TypeGroup);

    GPS.add("Port",      Setting::TypeString)  = fSerialPortName;

    // Reset both the request to reset and the debug parameter. 
    GPS.add("Reset",     Setting::TypeBoolean) = false;
    GPS.add("Debug",     Setting::TypeInt)     = 0;
    GPS.add("Display",   Setting::TypeBoolean) = fDisplay;
    GPS.add("Logging",   Setting::TypeBoolean) = fLogging;

    // if falls through first time
    if(fGeodetic)
    {
	Lat = fGeodetic->CenterLat();
	Lon = fGeodetic->CenterLon();
    }
    GroupGeodetic.add("Latitude0",  Setting::TypeFloat) = Lat;
    GroupGeodetic.add("Longitude0", Setting::TypeFloat) = Lon;

    // Write out the new configuration.
    try
    {
	pCFG->writeFile(kConfigFileName);
	Logger->LogTime(" New configuration successfully written to: %s\n",
			kConfigFileName);

    }
    catch(const FileIOException &fioex)
    {
	Logger->Log("# I/O error while writing file: %s \n",
		    kConfigFileName);
	delete pCFG;
	return(false);
    }
    delete pCFG;
    SET_DEBUG_STACK;
    return true;
}

