/********************************************************************
 *
 * Module Name : Oncore.cpp
 *
 * Author/Date : C.B. Lirakis / 30-Jul-10
 *
 * Description : Library functions to decode and access the Oncore 
 * GPS time module. This primarily uses the Motorola message format. 
 *
 * Restrictions/Limitations :
 *
 * Change Descriptions :
 * 18-Feb-24  Major Overhaul. process one full sentance at a time. 
 *            Took out simple buffer and used utility Buffered. 
 *            This allows me to drain the information off the buffer
 *
 * 20-Mar-24  Redo DST flag. 
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
#include <iomanip>
#include <cstring>

// Local Includes.
#include "Oncore.hh"
#include "CLogger.hh"
#include "Buffered.hh"

const double MSarcPerDeg = 3600000.0;
const double DegPerMSarc = 1.0/MSarcPerDeg;
const char   LF = 0x0A;
const char   CR = 0x0D;

/**
 ******************************************************************
 *
 * Function Name : Oncore constructor
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
Oncore::Oncore (void) 
{
    SET_DEBUG_STACK;

    fBuffer        = new Buffered(kBufferSize);
    fPS            = new PositionStatus();
    fVS            = new VisibleSatellites();
    fRAIM          = new RAIM();
    fCKSUM         = 0;
    CLogger::GetThis()->Log("# Oncore class initialized. \n");
    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name : Oncore destructor
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
Oncore::~Oncore (void)
{
    SET_DEBUG_STACK;
    delete fBuffer;

    if (fPS)
    {
        delete fPS;
    }
    if (fVS)
    {
        delete fVS;
    }
    if (fRAIM)
    {
        delete fRAIM;
    }
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : CompiledOn
 *
 * Description : Return character string about when last compilied.
 *
 * Inputs : none
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
char *CompiledOn(void)
{
    static char rv[256];
    SET_DEBUG_STACK;
    sprintf(rv,"Version %d.%d Compiled on %s %s", VERMAJ, VERMIN, 
	    __DATE__, __TIME__ );
    SET_DEBUG_STACK;
    return rv;
}
/**
 ******************************************************************
 *
 * Function Name : Parse
 *
 * Description : A full sentance has been received, decode it. 
 *               The upstream call will guarantee that the 
 *               buffer is not empty. 
 *
 * Inputs : NONE
 *
 * Returns : true on success. 
 *
 * Error Conditions :
 *     Bad Parse
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
bool Oncore::Parse(void)
{
    SET_DEBUG_STACK;
    CLogger       *Log  = CLogger::GetThis();
    bool          rc    = true;
    unsigned char cksum = 0;
    unsigned char command, subcommand;

    if (Log->GetVerbose()>0)
    {
	*Log->LogPtr() << *fBuffer;
    }

    /* 
     * remove the prefix @@ if there. 
     * Get the first character.
     */
    command = fBuffer->GetChar();
    if (command == '@')   command = fBuffer->GetChar(); // Advance the buffer
    if (command == '@')   command = fBuffer->GetChar(); // Advance the buffer

    // Get the subcomand too. 
    subcommand = fBuffer->GetChar();

    // FIXME!!!
    // Not sure how well this will work given how I've strutured the buffer.
//    cksum = CKSUM(fBuffer[index], fBufferPointer);

    /* Decide how to decode based on first letter. */
    switch (command)
    {
    case 'A':
        switch(subcommand)
        {
	    /* Message AP - which pulse mode are we in. */
        case 'P':
            DecodeAP(fBuffer);
            if (Log->GetVerbose()>0)
            {
		Log->Log("# 1PPS Mode: %d\n",(int) fPPSMode);
            }
            break;
        default:
	    Log->Log("# ERROR, FILE: %s, LINE: %d, Unknown A command: %c\n", 
		     (char) subcommand );
            break;
        }
        break;

    case 'B':
        switch(subcommand)
        {
	    /* Case Bb - Visible satellites message */
        case 'b': // Satellites
            fVS->Clear();   // clear out old data

	    // Decode. 
            if (fVS->Update(fBuffer) > 0)
            {
		Log->Log("# ERROR, FILE: %s, LINE: %d, Buffer size mismatch . command: Bb, Expected: 89  Got: %d\n",
			 __FILE__, __LINE__, fBuffer->GetFill());
		*Log->LogPtr() << *fBuffer; 
                rc = false;
                break;
            }
            fCKSUM = fVS->Checksum();
            if (Log->GetVerbose()>0)
            {
                *Log->LogPtr() << *fVS;
            }
            rc = true;
            break;
        default:
	    Log->Log("# ERROR, FILE: %s, LINE: %d, Unknown subcode B: %c\n",
				    __FILE__, __LINE__ , subcommand);
	    rc = false; 
            break;
        }
        break;
    case 'E':
        switch (subcommand)
        {
	    /* Case Ea - Position status message */
        case 'a':
	    // Position status
            fPS->Clear();
            if (fPS->Update(fBuffer)>0)
            {
		Log->Log("# ERROR, FILE: %s, LINE: %d, Position, Buffer size mismatch. command: Ea, Expected: 76 Got: %d\n",
			 __FILE__, __LINE__ , fBuffer->GetFill());
                rc = false;
                break;
            }
            fCKSUM = fPS->Checksum();
            if (Log->GetVerbose()>0)
            {
                *Log->LogPtr() << *fPS;
            }
            rc = true;
            break;
	    /* Case En - RAIM status */
        case 'n':
            // RAIM Status;
            fRAIM->Clear();
            if (fRAIM->Update(fBuffer)>0)
            {
		Log->Log("# ERROR, FILE: %s, LINE: %d, RAIM, Buffer size mismatch. command: En, Expected: 69 Got: %d\n",

			 __FILE__, __LINE__ , fBuffer->GetFill());

		*Log->LogPtr() << *fBuffer; 
                rc = false;
                break;
            }
            fCKSUM = fRAIM->Checksum();
            if (Log->GetVerbose()>0)
            {
                *Log->LogPtr() << *fRAIM;
            }
            rc = true;
            break;
        default:
	    Log->Log("# ERROR, FILE: %s, LINE: %d, Unknown %c %c\n",
		     __FILE__, __LINE__ , command, subcommand); 
	    rc = false;
            break;
        }
        break;
    default:
	Log->Log("# ERROR, FILE: %s, LINE: %d, Unknown code %c\n",
		 __FILE__,__LINE__, command);
	rc = false;
        break;
    }

#if 0        // FIXME
    if (rc)
    {
        if (cksum != fCKSUM)
        {
	    Log->Log("# ERROR, FILE: %s, LINE: %d, Checksum failed. Mine %d, Buffer %d, Buffersize: %d\n"
		     __FILE__, __LINE__, cksum, fCKSUM, 
		     (int) fBuffer->GetFill());
	    *Log->LogPtr() << *fBuffer; 
            rc = false;
        }
    }
#endif
    SET_DEBUG_STACK;
    return rc;
}
/**
 ******************************************************************
 *
 * Function Name : CKSUM
 *
 * Description : make a motorola style cksum using sum of xor
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 5-Sep-10
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
unsigned char Oncore::CKSUM(unsigned char *buffer, int buffersize)
{
    SET_DEBUG_STACK;
    unsigned char CKSUM = 0;
    for (int i=0;i<buffersize;i++) 
    {
        CKSUM = CKSUM^buffer[i];
    }
    SET_DEBUG_STACK;
    return CKSUM;
}
/**
 ******************************************************************
 *
 * Function Name : MakeCommand
 *
 * Description : Construct a command string from the desired command
 *
 * Inputs : Command to issue
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 5-Sep-10
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
unsigned char* Oncore::MakeCommand(const char* Command, 
                                   const unsigned char *data,
                                   int DataSize)
{
    SET_DEBUG_STACK;
    CLogger *Log = CLogger::GetThis();
    int i = 0;
    int j;
    fCommandSize = 0;
    memset (fCommandBuffer, 0, sizeof(fCommandBuffer));
    fCommandBuffer[i] = '@'; i++;
    fCommandBuffer[i] = '@'; i++;
    fCommandBuffer[i] = Command[0]; i++;
    fCommandBuffer[i] = Command[1]; i++;
    for (j=0;j<DataSize;j++)
    {
        fCommandBuffer[i] = data[j]; i++;
    }
    fCommandBuffer[i] = CKSUM(fCommandBuffer,i); i++;
    fCommandBuffer[i] = CR; i++;
    fCommandBuffer[i] = LF; i++;

    fCommandSize = i;
    if (Log->GetVerbose()>0)
    {
        *Log->LogPtr() << "# Command: " << hex;
        for (i=0; i<(int)fCommandSize; i++)
        {
            *Log->LogPtr() << (int) fCommandBuffer[i] << " ";
        }
        *Log->LogPtr() << dec << endl;
    }
    SET_DEBUG_STACK;
    return fCommandBuffer;
}
/**
 ******************************************************************
 *
 * Function Name : PositionStatus Constructor
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
PositionStatus::PositionStatus(void)
{
    SET_DEBUG_STACK;
    fTime.tv_sec = fTime.tv_nsec = 0L;
    fLatitude = fLongitude = fAltitude = 0.0;
    fVelocity = fHeading = 0.0;
    fSat      = new RxData[NRXDATA];
    fCKSUM    = 0;
    SET_DEBUG_STACK;
}


/**
 ******************************************************************
 *
 * Function Name : DecodeAP
 *
 * Description : AP message is pulse mode. 
 *  0 - 1PPS output
 *  1 - 100PPS output
 *
 * Inputs : NONE
 *
 * Returns : number of characters decoded. 
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
uint32_t Oncore::DecodeAP(Buffered *buf)
{
    SET_DEBUG_STACK;
    fPPSMode = buf->GetChar();
    fCKSUM   = buf->GetChar();
    SET_DEBUG_STACK;
    return 8;  // 8 bytes including @@ and \r\n
}
/**
 ******************************************************************
 *
 * Function Name : Update
 *
 * Description : when the buffer is full and the response character
 * indicates position-status message, then call this to fill 
 * the data fields. 
 * Position Status prefix is: Ea
 *
 * Message looks like: 
 *    @@EamdyyhmsffffaaaaoooohhhhmmmmvvhhddtntimsdimsdimsdimsdimsdimsdimsdimsdsC<CR><LF>
 *
 * Inputs : size of buffer for check
 *
 * Returns : expected size of buffer if fails
 *
 * Error Conditions : expected buffer size != buffer size
 * 
 * Unit Tested on: 5-Sep-10
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
int PositionStatus::Update(Buffered *buf)
{
    SET_DEBUG_STACK;
    CLogger *Log = CLogger::GetThis();
    struct tm gpsnow, tmp;
    // 52 = 17 + 8*4 + 2+1
    time_t myclock;
    time_t Offset = timezone;
    tzset();

    time(&myclock);
    struct tm *local = localtime(&myclock);
    
    // daylight is set IFF this timezone has a daylight, otherwise use offset
    // raw
     if (local->tm_isdst == 1)
     {
         Offset -= 3600;
     }

    fCKSUM = 0;
    // PC time is recorded in the buffer time
//    clock_gettime( CLOCK_REALTIME, &fPCTime);

    fPCTime = buf->GetTime();   // time of first character. 

    gpsnow.tm_mon  = buf->GetChar()-1;  // this structure uses {0:11}
    gpsnow.tm_mday = buf->GetChar();
    gpsnow.tm_year = buf->GetInt();
    gpsnow.tm_year -= 1900;

    gpsnow.tm_hour = buf->GetChar();
    gpsnow.tm_min  = buf->GetChar();
    gpsnow.tm_sec  = buf->GetChar();
    fTime.tv_sec   = mktime(&gpsnow);
    fTime.tv_nsec  = buf->GetLongInt();

    // Adding in UTC time. Number of seconds in current day. 
    memset(&tmp, 0, sizeof(struct tm));
    tmp.tm_sec    = gpsnow.tm_sec;
    tmp.tm_min    = gpsnow.tm_min;
    tmp.tm_hour   = gpsnow.tm_hour;
    fUTC = tmp.tm_hour*3600 + tmp.tm_min*60 + tmp.tm_sec;
	
    // Process pc Delta.
    fDelta = (double) (fTime.tv_sec - fPCTime.tv_sec - Offset) + 
	((double) (fPCTime.tv_nsec - fTime.tv_nsec)) * 1.0e-9;


//     cout << "DAYLIGHT: " << daylight << " OFFSET: " << Offset/3600 
// 	 << " delta: " << fDelta
// 	 << endl;

    if (Log->GetVerbose()>0)
    {
	*Log->LogPtr() << "# Position Status, month; " 
		       << gpsnow.tm_mon
		       << " day: "  << gpsnow.tm_mday
		       << " year: " << gpsnow.tm_year
		       << " hour: " << gpsnow.tm_hour
		       << " min: "  << gpsnow.tm_min
		       << " sec: "  << gpsnow.tm_sec
		       << endl
		       << "# ftime - PC sec: " 
		       << (fTime.tv_sec - fPCTime.tv_sec - Offset)
		       << ", nsec: " 
		       << (fTime.tv_nsec - fPCTime.tv_nsec)
		       << endl;
    }
    fLatitude  = buf->GetLongInt()*DegPerMSarc;
    fLongitude = buf->GetLongInt()*DegPerMSarc;
    fAltitude  = buf->GetLongInt()*0.01;      // meters
    // Skip the next 4 characters
    buf->Skip(4);
    fVelocity  = buf->GetInt()*0.01;           // meters per second
    fHeading   = buf->GetInt()*0.1;            // Degrees
    fDOP       = buf->GetInt()*0.1;            // 0.1 resolution
    ftDOP      = buf->GetChar();               // dop type, bit field
    fNum       = buf->GetChar();               // Number satellites vis.
    fTNum      = buf->GetChar();               //  "   " tracked

    // Modes: 0 - search/acquire
    //        1 - code acq.
    //        2 - AGC set
    //        3 - preq acq. 
    //        4 - bit sync det.
    //        5 - message sync det.
    //        6 - satellite time available
    //        7 - ephemeris acq.
    //        8 - avail for pos.
    for (int i=0 ; i<8 ;i++)
    {
	fSat[i].ID     = buf->GetChar();
	fSat[i].Mode   = buf->GetChar();
	fSat[i].SNR    = buf->GetChar();  // 0:255 dB
	fSat[i].Status = buf->GetChar();  // bit flag
    }
    fStatus = buf->GetChar();
    fCKSUM  = buf->GetChar();
    fFilled = true;
    //cout << "DIFF: "  << dec << (int) (val-ival) << endl;

    uint32_t rv = buf->Remaining();
    // There are 4 trailing characters \r \n 0x40 0x40 
    rv -= 4;
    SET_DEBUG_STACK;
    return rv;
}

/**
 ******************************************************************
 *
 * Function Name : PositionStatus destructor
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
PositionStatus::~PositionStatus(void)
{
    SET_DEBUG_STACK;
    if (fSat)
    {
        delete[] fSat;
    }
    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name : PositionStatus ostream friend
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
ostream& operator<<(ostream& output, const PositionStatus &n)
{
    SET_DEBUG_STACK;
    time_t now = (time_t) n.Time().tv_sec;
    output << "# " << ctime(&now)
           << "# Latitude: "    << n.Latitude()
           << " Longitude: "    << n.Longitude()
           << " Altitude: "     << n.Altitude()
           << endl
           << "# Velocity: "    << n.Velocity()
           << " Heading: "      << n.Heading()
           << " DOP: "          << n.DOP() 
           << " DOP Type: "     << hex << n.TDOP()
           << endl
           << "# Sat in View: " << dec << n.NSAT()
           << " Sat. Tracked: " << n.NSAT_Tracked()
           << " Status: "       << hex << n.Status()
           << dec
           << endl
           << "#    ------------------------------------------------" 
           << endl;
    for (int i=0 ;i<8;i++)
    {
        output << "#    | " << n.fSat[i] << " |" << endl;
    }
    output << "#    ------------------------------------------------" << endl;
    SET_DEBUG_STACK;
    return output;
}

/**
 ******************************************************************
 *
 * Function Name : ostream for RxData
 *
 * Description : 
 *
 * Inputs : ostream to be filled, RxData to extract from.
 *
 * Returns : ostream filled with formatted RxData
 *
 * Error Conditions : none
 * 
 * Unit Tested on: 22-Sep-10
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
ostream& operator<<(ostream& output, const RxData &n)
{
    SET_DEBUG_STACK;
    output << "Satellite ID: " << dec << std::right << setw(2) << (int) n.ID
           << " Mode: "   << dec << std::right << setw(2) << (int) n.Mode
           << " SNR: "    << dec << std::right << setw(2) << (int) n.SNR
           << " Status: " << hex << std::right << setw(2) << (int) n.Status;
    SET_DEBUG_STACK;
    return output;
}
/**
 ******************************************************************
 *
 * Function Name :  VisibileSatellites constructor
 *
 * Description : create a structure to hold upte MAX_SATELLITE of data. 
 *
 * Inputs : if size is non-zero, means some data is available in the buffer. 
 *
 * Returns : 
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
VisibleSatellites::VisibleSatellites(void)
{
    SET_DEBUG_STACK;
    fNSat = 0;
    fData = new VisibleData[MAX_SATELLITE];

    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : Clear
 *
 * Description : Zero out the data in the VisibleSatellites class.
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : none
 * 
 * Unit Tested on: 22-Sep-10
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void VisibleSatellites::Clear(void)
{
    SET_DEBUG_STACK;
    fNSat = 0;
    if (fData)
    {
        for (int i=0;i<12;i++)
            fData[i].Clear();
    }
    fCKSUM = 0;
    fFilled = false;
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name :  VisibleSatellites destructor
 *
 * Description : delete allocated memory.
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : none
 * 
 * Unit Tested on: 22-Sep-10 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
VisibleSatellites::~VisibleSatellites(void)
{
    SET_DEBUG_STACK;
    delete[] fData;
}
/**
 ******************************************************************
 *
 * Function Name : PositionStatus Clear Method
 *
 * Description : Clear all variables.
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
void PositionStatus::Clear(void)
{
    SET_DEBUG_STACK;
    memset( &fTime, 0, sizeof(struct  timespec));
    memset( &fPCTime, 0, sizeof(struct  timespec));

    fLatitude = fLongitude = fAltitude = 0.0;
    fVelocity = fHeading   = fDOP    = 0.0;
    ftDOP     = fNum       = fTNum   = fStatus = 0;
    if (fSat)
    {
        for (int i=0;i<NRXDATA;i++)
            fSat[i].Clear();
    }
    fCKSUM  = 0;
    fFilled = false;
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : Visible Satellites 
 *
 * Description : Update Method, this is parsing the Bb message. 
 *               Total length of message is 92 bytes including @@ and \r\n
 *
 * message format: 
 *     @@BbniddeaasiddeaasiddeaasiddeaasiddeaasiddeaasiddeaasiddeaasiddeaasiddeaasiddeaasiddeaasC<CR><LF>
 *
 * Inputs :  index into buffer, length of buffer
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
uint32_t VisibleSatellites::Update(Buffered *buf)
{
    SET_DEBUG_STACK;

    /*
     *  niddeaas iddeaas repeated upto 12 (satellite) times. 
     * 89 = 4 + 1 + 12*7
     * The prefix and suffix are stripped, so should only be
     * 84 total
     */
    fCKSUM = 0;
    fNSat  = buf->GetChar();
    for (int i=0;i<12;i++)
    {
	fData[i].ID        = buf->GetChar();   // Satellite ID, byte
	fData[i].Doppler   = buf->GetInt();    // Doppler, short
	fData[i].Elevation = buf->GetChar();   // Elevation in deg, byte
	fData[i].Azimuth   = buf->GetInt();    // Azimuth in deg, short
	fData[i].Health    = buf->GetChar();   // Health info, byte
    }
    fCKSUM  = buf->GetChar();
    fFilled = true;

    SET_DEBUG_STACK;
    return (buf->Remaining()-4);
}
/**
 ******************************************************************
 *
 * Function Name : PositionStatus destructor
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
ostream& operator<<(ostream& output, const VisibleData &n)
{
    SET_DEBUG_STACK;
    output << std::right << std::setw(2) << "Satellite ID: "<< (int) n.ID
           << std::right << std::setw(6) << " Doppler: "    << n.Doppler
           << std::right << std::setw(3) << " Azimuth: "    << n.Azimuth
           << std::right << std::setw(2) << " Elevation: "  << n.Elevation
           << std::right << std::setw(2) << " Health: "     << (int) n.Health; 
    SET_DEBUG_STACK;
    return output;
}

/**
 ******************************************************************
 *
 * Function Name : Visible satellites ostream friend
 *
 * Description :
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 5-Sep-10
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
ostream& operator<<(ostream& output, const VisibleSatellites &n)
{
    SET_DEBUG_STACK;
    output << "# Number Visible: " << dec << n.fNSat << endl
           << "# ----------------------------------------------" << endl;
    for (uint8_t i=0;i<n.fNSat; i++)
    {
        output << "#      " << n.fData[i] << endl;
    }
    output << "# ----------------------------------------------" << endl;
    SET_DEBUG_STACK;
    return output;
}


/**
 ******************************************************************
 *
 * Function Name : SatTime constructor
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
SatTime::SatTime()
{
    SET_DEBUG_STACK;
    fID      = 0;
    fTimeEst = 0;
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : SatTime Update
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
void SatTime::Update (unsigned char ID, unsigned int Time)
{
    SET_DEBUG_STACK;
    fID = ID;
    fTimeEst = Time;
}
/**
 ******************************************************************
 *
 * Function Name : SatTime ostream friend
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
ostream& operator<<(ostream& output, const SatTime &n)
{
    SET_DEBUG_STACK;
    output << "#     ID: " << std::right << std::setw(2) << (int) n.fID 
           << " Local Time Est: (ns) : " 
	   << std::right << std::setw(2) << (int) n.fTimeEst 
           << endl;
    return output;
}

/**
 ******************************************************************
 *
 * Function Name : RAIM constructor
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
RAIM::RAIM(void)
{
    SET_DEBUG_STACK;
    fSat = new SatTime[NRXDATA];

    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : RAIM clear
 *
 * Description : zero RAIM data
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
void RAIM::Clear()
{
    SET_DEBUG_STACK;
    fFilled      = false;
    fMessageRate = 0;
    fRAIMOnOff   = 0;
    fAlarm       = 0;
    fPPSMode     = 0;
    fCKSUM       = 0;
}
/**
 ******************************************************************
 *
 * Function Name : RAIM destructor
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
RAIM::~RAIM()
{
    SET_DEBUG_STACK;
    delete[] fSat; 
}
/**
 ******************************************************************
 *
 * Function Name : RAIM Update Method
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
int RAIM::Update(Buffered *buf)
{
    SET_DEBUG_STACK;
    unsigned char ID;
    uint32_t      nsec;

    // EnotaapxxxxxxxxxxC<CR><LF>
    // 16 = 1+1+2+1+10+1
    // @@EnotaapxxxxxxxxxxpysreensffffsffffsffffsffffsffffsffffsffffsffffC<CR><LF>
    // o - output message rate (0:255)
    // t - RAIM on/off 0:1
    // aa - Raim alarm limit 3:65535 100ns per step
    // p  - PPS control mode 0-3
    // xxxxxxxxxx - 10 char not used 
    // p - pulse status
    // y - 1pps pulse sync status
    // s - RAIM solution status 0-ok, 1-alarm, 2 unknown
    // r - RAIM time status 0-detection/isolation possible, 1 detection only
    //                      2 - neither possible
    // ee - time solution, one sigma in nanoseconds 0-65535
    // n  - negative sawtooth -128-127
    // For each receiver channel. 
    // s - satellite id
    // ffff - fractional GPS local time estimate in nanoseconds
    // C - checksum
                           
    fFilled             = false;
    fCKSUM              = 0;

    fMessageRate        = buf->GetChar();
    fRAIMOnOff          = buf->GetChar();
    fAlarm              = buf->GetInt();
    fPPSMode            = buf->GetChar();
    
    buf->Skip(10);  // Skip unused data.
    fPulseStatus        = buf->GetChar();
    fPulseSync          = buf->GetChar();
    fRAIMSolutionStatus = buf->GetChar();
    fRAIMStatus         = buf->GetChar();
    fTimeSolution       = buf->GetInt();
    fSawtooth           = buf->GetChar();

    for(int i=0;i<NRXDATA;i++)
    {
	ID   = buf->GetChar();
	nsec = buf->GetLongInt();
	fSat[i].Update(ID, nsec);

    }
    fCKSUM       = buf->GetChar();
    fFilled      = true;

    return (buf->Remaining()-4);
}
/**
 ******************************************************************
 *
 * Function Name : RAIM Message
 *
 * Description : Compose command for change. 
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
uint8_t* RAIM::Message(bool q)
{
    SET_DEBUG_STACK;
    static uint8_t Buffer[128];
    int i = 0;
    unsigned char *p;

    memset(Buffer, 0, sizeof(Buffer));
    if (q)
    {
        for (i=0;i<16;i++)
        {
            Buffer[i]=0xff;
        }
    }
    else
    {
        Buffer[i] = fMessageRate; i++;
        Buffer[i] = fRAIMOnOff;   i++;
        p = (unsigned char*)&fAlarm;
        Buffer[i] = p[1];i++; 
        Buffer[i] = p[0];i++;
        Buffer[i] = fPPSMode; i++;
    }
    return Buffer;
}
/**
 ******************************************************************
 *
 * Function Name : RAIM ostream friend
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
ostream& operator<<(ostream& output, const RAIM &n)
{
    SET_DEBUG_STACK;
    int i;
    output << "# ------------------------------------------------"  << endl
           << "# RAIM " << endl
           << dec
           << "#     Message Rate: " << (int) n.fMessageRate << endl
           << "#     RAIM on off : " << (int) n.fRAIMOnOff   << endl
           << "#     Alarm limit : " << n.fAlarm             << endl
           << "#     PPSMode     : " << (int) n.fPPSMode     << endl
           << "#     Pulse Status: " << (int) n.fPulseStatus << endl
           << "#     Pulse Sync  : " << (int) n.fPulseSync   << endl
           << "# RAIM Sol. Status: " << (int) n.fRAIMSolutionStatus << endl
           << "#     RAIM Status : " << (int) n.fRAIMStatus  << endl
           << "#Time Solution(ns): " << n.fTimeSolution      << endl
           << "#         SawTooth: " << (int) n.fSawtooth 
           << endl;
    for(i=0;i<NRXDATA;i++)
    {
        output << n.fSat[i];
    }
     output << "#    ---------------------------------------------" 
            << endl;
    return output;
}
