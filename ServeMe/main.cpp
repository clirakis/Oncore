/**
 ******************************************************************
 *
 * Module Name : main.cpp
 *
 * Author/Date : C.B. Lirakis / 02-Feb-01
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
 * $RCSfile: main.cpp,v $
 * $Author: clirakis $
 * $Date: 2003/12/31 14:39:57 $
 * $Locker:  $
 * $Name:  $
 * $Revision: 1.2 $
 *
 * $Log: main.cpp,v $
 * Revision 1.2  2003/12/31 14:39:57  clirakis
 * Check in prior to making a change chag[3~ changing
 * shared memory time structure from timeval to timespec.
 *
 * Revision 1.1  2003/08/08 17:24:47  clirakis
 * Initial revision
 *
 *******************************************************************
 */  
    

#ifndef lint
/// RCS Information
static char rcsid[] = "$Header: /home/clirakis/code/lassen/RCS/main.cpp,v 1.2 2003/12/31 14:39:57 clirakis Exp $";

#endif  /*  */

    
// System includes.
#include <iostream>
using namespace std;

#include <string>
#include <cmath>
#include <csignal>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/resource.h>
#include <errno.h>
#include <fstream>

/// Local Includes.
#include "serial.hh"
#include "lassen.hh"
#include "tools.h"
#include "User.hh"
#include "debug.h"
#include "sharedmem.hh"    // class definition for shared segment. 
#include "ProcessTime.hh"   // A thread for doing time keeping. 

static int       VerboseLevel = 0;
static bool      LoggingOn = false;
static struct tm now;
static pthread_t rx_thread;

#define MAX_BUFFER 128
#define N_BUFFERS  32

struct ThreadControl_t {
    Lassen*   gps;
    int       serial_fd;
    bool      Run;
    bool      IsRunning;
    Buffered* Buffers[N_BUFFERS];
    unsigned  WriteBuffer, ReadBuffer;
    sem_t     BufferReady;
};

static struct ThreadControl_t Rx;
static SharedMem   *PositionData;
static SharedMem   *SolutionData;
static SharedMem   *TrackingData;
static SharedMem   *ProcessMon;
static int         DLE_Count = 0;
static ProcessTime *timeProc;
static User        *user;

/*
 * Setup a global variable for logging rather than to stdout. 
 */
ofstream logFile;

// Turn on if you want signals to be registered
#define USE_SIGNALS 1
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
static void Terminate (int sig) 
{
    static int i=0;
    int    count;
    time_t now;

    time(&now);

    i++;
    if (sig != 0)
    {
	logFile << asctime(gmtime(&now))  
		<< " Death occured at: " 
		<< LastFile << " " << LastLine 
		<< endl;
	cerr << asctime(gmtime(&now))  
	     << " Death occured at: " 
	     << LastFile << " " << LastLine 
	     << endl;

    }
    switch (sig)
    {
    case 0:                    // Normal termination
        cerr << "Normal program termination." << endl;
        break;
    case SIGHUP:
        cerr << " Hangup" << endl;
        break;
    case SIGINT:               // CTRL+C signal 
        cerr << " SIGINT " << endl;
        break;
    case SIGQUIT:               //QUIT 
        cerr << " SIGQUIT " << endl;
        break;
    case SIGILL:               // Illegal instruction 
        cerr << " SIGILL " << endl;
        break;
    case SIGABRT:              // Abnormal termination 
        cerr << " SIGABRT " << endl;
        break;
    case SIGBUS:               //Bus Error! 
        cerr << " SIGBUS " << endl;
        break;
    case SIGFPE:               // Floating-point error 
        cerr << " SIGFPE " << endl;
        break;
    case SIGKILL:               // Kill!!!! 
        cerr << " SIGKILL" << endl;
        break;
    case SIGSEGV:              // Illegal storage access 
        cerr << " SIGSEGV " << endl;
        break;
    case SIGTERM:              // Termination request 
        cerr << " SIGTERM " << endl;
        break;
    case SIGSTKFLT:               // Stack fault
        cerr << " SIGSTKFLT " << endl;
        break;
    case SIGTSTP:               // 
        cerr << " SIGTSTP" << endl;
        break;
    case SIGXCPU:               // 
        cerr << " SIGXCPU" << endl;
        break;
    case SIGXFSZ:               // 
        cerr << " SIGXFSZ" << endl;
        break;
    case SIGSTOP:               // 
        cerr << " SIGSTOP " << endl;
        break;
    case SIGPWR:               // 
        cerr << " SIGPWR " << endl;
        break;
    case SIGSYS:               // 
        cerr << " SIGSYS " << endl;
        break;
    default:
        cerr << " Uknown signal type: "  << sig <<  endl;
        break;
    }
    if (i>1) 
    {
	logFile << asctime(gmtime(&now))  
		<< " Multiple terminates called!" 
		<< LastFile << " " << LastLine 
		<< endl;
	cerr << asctime(gmtime(&now))  
             << " Multiple terminates called!" 
	     << LastFile << " " << LastLine 
             << endl;
	exit(-1);
    }
    Rx.Run = false;
    count = 0;
    do {
	sleep(1);
	count++;
	logFile << "Waiting for RX to end." << count << endl;
    } while ((count<10) && Rx.IsRunning);
    logFile << __LINE__ << endl;

    delete user;
    delete timeProc;
    delete Rx.gps;
    logFile << __LINE__ << endl;

    if (Rx.serial_fd > -1)
    {
	close(Rx.serial_fd);
    }
    for (i=0;i<N_BUFFERS;i++)
    {
	delete Rx.Buffers[i];
    }
    logFile << __LINE__ << endl;

    sem_destroy( &Rx.BufferReady);
    delete PositionData;
    delete SolutionData;
    delete TrackingData;
    delete ProcessMon;
    logFile.close();
    logFile << __LINE__ << endl;

    if (sig == 0)
    {
	exit (0);
    }
    else
    {
	exit (-1);
    }
}

/**
 ******************************************************************
 *
 * Function Name : SetSignals
 *
 * Description : Route termination signals through exit method. 
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : None
 * 
 * Unit Tested on: 23-Feb-08
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
static void SetSignals()
{
    /*
     * Setup a signal handler.      
     */
    signal (SIGINT , Terminate);   // CTRL+C signal 
    signal (SIGKILL, Terminate);   // 
    signal (SIGQUIT, Terminate);   // 
    signal (SIGILL , Terminate);   // Illegal instruction 
    signal (SIGABRT, Terminate);   // Abnormal termination 
    signal (SIGIOT , Terminate);   // 
    signal (SIGBUS , Terminate);   // 
    signal (SIGFPE , Terminate);   // 
    signal (SIGSEGV, Terminate);   // Illegal storage access 
    signal (SIGTERM, Terminate);   // Termination request 
    signal (SIGSTKFLT, Terminate); // 
    signal (SIGSTOP, Terminate);   // 
    signal (SIGPWR, Terminate);    // 
    signal (SIGSYS, Terminate);    // 
}
#define USER 1
/**
 ******************************************************************
 *
 * Function Name : AllocateBuffers
 *
 * Description : At startup, allocate the memory necessary for the
 *               sequence of buffers. 
 *
 * Inputs : Rx - receive buffer structure. 
 *
 * Returns : true on success
 *
 * Error Conditions : returns false on memory allocation fail. 
 * 
 * Unit Tested on:  13-Apr-08
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
static bool AllocateBuffers(struct ThreadControl_t *Rx)
{
    SET_DEBUG_STACK;
    int i;

    for (i=0;i<N_BUFFERS;i++)
    {
	Rx->Buffers[i] = new Buffered(MAX_BUFFER);
    }
    SET_DEBUG_STACK;
    return true;
}

/**
 ******************************************************************
 *
 * Function Name : RxThread
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
void* RxThread(void* arg)
{
    SET_DEBUG_STACK;
    time_t        now;
    int           ThreadPriority;
    unsigned char inchar, lastchar;
    unsigned long status;
    struct        ThreadControl_t *Rx = (struct ThreadControl_t*) arg;

    /* Seconds and microseconds timeout at 100ms */
    struct timeval timeout = {0L, 100000L};
    fd_set         readfds;
    int            rv;
    int            TimeoutCount = 0;
    int            PacketCount;
    int            LoopCount    = 0;
    unsigned long  OverallCount = 0;
    char           text[256];
#if (USE_SIGNALS == 1)
    SetSignals();
#endif

    Rx->IsRunning     = true;
    Rx->Run           = true;
    Rx->WriteBuffer   = 0;
    Rx->ReadBuffer    = 0;
    lastchar          = 0;
    PacketCount       = 0;
    lastchar          = 0;

    ThreadPriority = getpriority(PRIO_PROCESS,0);
    ThreadPriority += 2;
    if(setpriority(PRIO_PROCESS, 0, ThreadPriority) <0)
    {
	printf("Error setting Thread Priority\n");
	/* Not fatal. */
    } 

    while( Rx->Run)
    {
	OverallCount++;
#if 0
	if (OverallCount%2000 == 0)
	    logFile << OverallCount << endl;
#endif
	/*
	 * Hang out until we actually have something on the serial port. 
	 * Actually we want to use a non-blocking read so we can 
	 * exit properly. So we check on timeout as necessary.
	 */
	FD_ZERO(&readfds);
	FD_SET (Rx->serial_fd, &readfds);
	timeout.tv_sec  = 0L;
	timeout.tv_usec = 100000L;
	status = 0L;
	//rv = select( Rx->serial_fd, &readfds, NULL, NULL, &timeout);
	rv = select( Rx->serial_fd+1, &readfds, NULL, NULL, &timeout);
        LoopCount = (LoopCount + 1)%16;

	/* What does our return code say to do? */
	switch( rv)
	{
	case -1:
	    //perror("select");
	    logFile << __FILE__ << " " << __LINE__
		    << " Error in select." << endl;
	    status |= 0x00000001;
	    break;
	case 0:
	    // Timeout condition. 
            TimeoutCount++;
            if (TimeoutCount%100 == 0)
            {
		/* 
		 * This many timeouts may indicate a hung serial line
		 * or other hardware problem. 
		 */
                logFile << __FILE__ << " "  << __LINE__
			<< "Rx Timeout!"
			<< endl; 
            }
	    status |= 0x00000002;
	    break;
	default:
	    /* By default we want to process the data */

	    status |= 0x00000004;
            TimeoutCount = 0;
	    // Something is available. Put it in the variable inchar
	    inchar = 0;
	    rv = read ( Rx->serial_fd, &inchar,  1);
	    if ((rv > 0) && ((VerboseLevel*0x00000010)>0))
	    {
		logFile << "Data Rx: 0x" 
			<< hex << (int) inchar
			<< endl;
	    }
	    /***********************************************************
	     * A TSIP packet is composed of
	     * DLE ID Data DLE ETX
	     *
	     * Here we determine if the packet length, is ended etc.
	     * Always fill the buffer, just use the switching to determine
	     * if the buffer pointers should switch.
	     ***********************************************************
	     */
	    Rx->Buffers[Rx->WriteBuffer]->Put(inchar);
	    SET_DEBUG_STACK;
	    PacketCount++;

	    switch(inchar)
	    {
	    case DLE:
		DLE_Count++;
		/**
		 *  It could be data a begin or end byte.
		 * 2 consequitive DLE indicate start of packet. 
		 */
		if (DLE_Count <2)
		{
		    // It is probably data.
		    status |= 0x00000008;   
		}
		else
		{
#if 0
		    logFile << "Processing Packet" << Rx->WriteBuffer << endl;
#endif
		    PacketCount = 0;
		    status |= 0x00000010;   // Processing packet
		}
		break;
	    case ETX:
		/* 
		 * ETX indicates the end of the buffer. 
		 * But may be part of the data stream. 
		 */
		DLE_Count = 0;
		status |= 0x00000020;
		// If preceeded by a DLE then it is an end!!!
		if (lastchar == DLE)
		{
#if 0
		    logFile << "Packet Ends " << Rx->WriteBuffer 
			    << " Size " << PacketCount
			    << " Command " << hex
			    << (int) Rx->Buffers[Rx->WriteBuffer].Char()
			    << dec << endl;
#endif
		    // Clear Busy flag for this buffer. 
		    Rx->Buffers[Rx->WriteBuffer]->ClearBusy();
		    PacketCount = 0;
		    // Increment buffer count in a Ring way.
		    Rx->WriteBuffer = (Rx->WriteBuffer+1) % N_BUFFERS;
		    if (Rx->Buffers[Rx->WriteBuffer]->Busy())
		    {
			time(&now);
			strftime( text, sizeof(text), "%F %T", gmtime(&now));
			logFile << text
				<< " This buffer is busy." 
				<< Rx->WriteBuffer
				<< endl;
		    }
		    else
		    {
			// Set busy flag
			Rx->Buffers[Rx->WriteBuffer]->SetBusy();
		    }
		    sem_post(&Rx->BufferReady);
		    status |= 0x00000040;
		}
		break;
	    default:
		DLE_Count = 0;
		break;
	    }
	    lastchar = inchar;
	    break;
	}
	//logFile << " STATUS " << hex << status << dec << endl;
	if (ProcessMon)
	{
	    status |= (	(Rx->Buffers[Rx->WriteBuffer]->GetFill() << 16) +
			(Rx->WriteBuffer<<20) +
			(LoopCount<<24));
	    ProcessMon->CopyAndPutUserData( &status);
	}
	else
	{
	    logFile << __FILE__ << " " << __LINE__
		    << " Process mon is null " << endl;
	}
    }
    status = 0xFFFFFFFF;
    ProcessMon->CopyAndPutUserData( &status);

    Rx->IsRunning = false;
    logFile << " RX Thread terminated. " << endl;
    SET_DEBUG_STACK;
    return 0;
}

/**
 ******************************************************************
 *
 * Function Name : Help
 *
 * Description : provides user with help if needed.
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : none
 *
 *******************************************************************
 */
static void Help(void)
{
    SET_DEBUG_STACK;
    cout << "********************************************" << endl;
    cout << "* Test file for lassen.              *" << endl;
    cout << "* Built on "<< __DATE__ << " " << __TIME__ << "*" << endl;
    cout << "* RCSinfo = " << rcsid << endl;
    cout << "* Available options are :                  *" << endl;
    cout << "*  -l  turn loggin on                      *" << endl;
    cout << "*  -v  set verbosity level                 *" << endl;
    cout << "*  -r  Cause a software reset and exit.    *" << endl;
    cout << "********************************************" << endl;
}
/**
 ******************************************************************
 *
 * Function Name : InitializeProgram
 *
 * Description : Setup signals, and do any user initialization here
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : none
 *
 *******************************************************************
 */
static bool InitializeProgram()
{
    time_t t_now;
    int nbytes;

    SET_DEBUG_STACK;
    Rx.gps = NULL;

#if (USE_SIGNALS == 1)
    SetSignals();
#endif
    // Open log file for program run. Mostly for debugging.
    logFile.open("log.txt");
    if (logFile.fail())
    {
	cerr << "Failed to open log file." << endl;
	return false;
    }
    
    time(&t_now);
    localtime_r(&t_now,&now);
    logFile << "Program starts: " << asctime(&now)  << endl;
    timeProc = new ProcessTime( );
    if (timeProc->Error() != 0)
    {
	cerr <<"Could not initialize time SM." << endl;
    }
    timeProc->SetVerbose(VerboseLevel);

    // If selected, initialize the user I/O.
    user = new User();

    // Setup the GPS access. 
    Rx.gps = new Lassen ();
    Rx.gps->SetVerbosity(VerboseLevel);
    if (LoggingOn)
    {
	Rx.gps->ToggleLogData();
    }
    Rx.gps->SetErrorLog(&logFile);
    
    // Open the serial port.
    if (( Rx.serial_fd = SerialOpen( user->GetSerialPortName(), B9600))<0)
    {
	return false;
    }
    else
    {
        logFile << "Opened serial port: " << user->GetSerialPortName() << endl;
    }

    /*
     * Allocate a buffer for the input data. 
     */
    if (!AllocateBuffers(&Rx))
    {
	return false;
    }
    sem_init( &Rx.BufferReady, 0, 0);

    if( pthread_create(&rx_thread, NULL, RxThread, (void *) &Rx) == 0)
    {
	logFile << "RX Thread successfully created." << endl;
    }
    else
    {
	SET_DEBUG_STACK;
	return false;
    }
    /*
     * If we get this far, also open a shared memory segment
     * for the gps data.
     */
#if 1
    PositionData = new SharedMem('P', sizeof(Position), "gpsdata");
    if (PositionData->GetError() != SharedMem::NO_ERROR)
    {
	delete PositionData;
	PositionData = 0;
	SET_DEBUG_STACK;
	return false;
    }
    SolutionData = new SharedMem('S', sizeof(SolutionStatus), "gpsdata");
    if (SolutionData->GetError() != SharedMem::NO_ERROR)
    {
	delete SolutionData;
	SolutionData = 0;
	SET_DEBUG_STACK;
	return false;
    }

    nbytes = 8*sizeof(struct PackedAzEl);
#if 0
    cout <<"Size of TData " << nbytes << " " 
	 << nbytes 
	 << endl;
#endif
    TrackingData = new SharedMem('T', nbytes, "gpsdata");
    if (TrackingData->GetError() != SharedMem::NO_ERROR)
    {
	delete TrackingData;
	TrackingData = 0;
	SET_DEBUG_STACK;
	return false;
    }
#else
    PositionData = 0;
    SolutionData = 0;
    TrackingData = 0;
#endif
    ProcessMon = new SharedMem('O', sizeof(unsigned long), "processmon");
    if (ProcessMon->GetError() != SharedMem::NO_ERROR)
    {
	delete ProcessMon;
	ProcessMon = 0;
    }
    else
    {
	logFile << __FILE__ << " " << __LINE__
		<< " Process monitor shared memory attached."
		<< endl;
    }
    
    SET_DEBUG_STACK;
    return true;
}
/********************************************************************
 *
 * Function Name : ProcessCommandLineArgs
 *
 * Description :
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 *
 ********************************************************************/
static void
ProcessCommandLineArgs(int argc, char **argv)
{
    SET_DEBUG_STACK;
    int   option;
	
    do
    {
        option = getopt( argc, argv, ":hHlrv:");
        switch(option)
	{
	case 'h':
	case 'H':
	    Help();
	    Terminate(0);
	    break;
	case 'l':
	    LoggingOn = true;
	    break;
	case 'v':
	    VerboseLevel = atoi(optarg);
	    cout << "Verbose Level = " << VerboseLevel << endl;
	    break;
	case 'r':
	    cout << "Performing soft reset and exiting!" << endl;
	    //gps->SoftReset();
	    Terminate(0);
	    break;
	}
    } while(option != -1);
    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name :  ProcessFrame
 *
 * Description : 
 *
 * Inputs : 
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
static void ProcessFrame()
{
    SET_DEBUG_STACK;
    static unsigned long FrameCount = 0;
    static time_t        lasttime, LastUpdate;
    time_t               now;
    Position*            pos;
    SolutionStatus*      status; 
    RawTracking*         pRaw;
    struct tm*           tnow;
    char                 str[64];
    int                  i,nraw;
    size_t               NBytes;
    // Tracking data to display info about satellites. 
    struct PackedAzEl    tdata[MAXPRNCOUNT];

    pos    = Rx.gps->GetPositionData();
    status = Rx.gps->LastStatus();
    pRaw   = Rx.gps->GetRawTrackingData();
    nraw   = Rx.gps->GetRawCount();

    time(&now);
    if (pos)
    {
	if ((VerboseLevel&0x00000004) > 0)
	{
	    logFile 
		<< __FILE__ << " " << __LINE__ 
		<< " Time: " << pos->Sec
		<< " Position:  " << 180.0/M_PI * pos->Latitude
		<< " " << 180.0/M_PI * pos->Longitude
		<< " " << pos->Altitude
		<< endl;
	}
	if (PositionData)
	{
	    PositionData->CopyAndPutUserData( pos);
	}
    }
    if (SolutionData && status)
    {
	SolutionData->CopyAndPutUserData(status);
    }

    if(pRaw && TrackingData && (nraw>0))
    {	
#if 0
	if (pRaw[0].DT() < 2.0)
	{
	    logFile << "Tracking: " << Rx.gps->GetRawCount ();
	    for (i=0;i<MAXPRNCOUNT; i++)
		logFile << " " << (int) pRaw[i].PRN << ":" 
			<< pRaw[i].SignalLevel << " ";
	    logFile << endl;
	}
#endif
	if ((now - LastUpdate) > 5)
	{
	    for(i=0;i<nraw;i++)
	    {
		tdata[i] = pRaw[i].CompactAzEl();
	    }
	    TrackingData->CopyAndPutUserData( tdata);
	    LastUpdate = now;
	}
    }

    if ((now - lasttime) > 15)
    {
#if DEBUG
	logFile << "Make request." << endl;
#endif
	Rx.gps->RequestTrackingStatus();
	const unsigned char *CommandBuffer=Rx.gps->GetCommandBuffer();
	NBytes = Rx.gps->GetCommandSize();
	write ( Rx.serial_fd , CommandBuffer, NBytes);
	lasttime = now;
    }
    FrameCount++;
    if (FrameCount%20 == 0)
    {
	tnow = gmtime( &now);
	strftime ( str, sizeof(str), "%F %T", tnow);
	logFile << __FILE__ << " " << __LINE__ << " " << str 
		<< " Process Frame: " << FrameCount << endl;
    }

    timeProc->DoIt( Rx.gps->Time(), Rx.gps->PCTime());
    if (user)
    {
	user->Do(Rx.gps, timeProc);
    }
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : main
 *
 * Description : main steering routine for the GPS interface. 
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 *
 *******************************************************************
 */ 
int main (int argc, char **argv) 
{
    const  int      waitvalue = 200; /* Milliseconds */
    struct timespec sleeptime = {0L,150000000};
    struct timespec waittime  = {1L,200000000};
    int           rv, i, nbuf, bzcount;
    int           NOfix_count, NOFix_test;
    time_t now;

    time(&now);
 
    waittime.tv_nsec = waitvalue * 1000000;
    NOFix_test       = (int) floor( 5.0/((double)waitvalue/1000.0));

    ProcessCommandLineArgs(argc, argv);
    if(InitializeProgram())
    {
	NOfix_count = 0;
        Rx.IsRunning = true;
	while (Rx.IsRunning)
	{
	    rv = sem_timedwait( &Rx.BufferReady, &waittime);
	    nanosleep( &sleeptime, NULL);
	    if ( rv==0 )
	    {
		// How many buffers are ready to be processed?
		nbuf = Rx.WriteBuffer - Rx.ReadBuffer;
		// Make sure it is normalized.
		if (nbuf < 0)
		{
		    nbuf += N_BUFFERS;
		}
                if (nbuf == 0)
                {
                    bzcount++;
                    if (bzcount > 10)
		    {
                        logFile << asctime(gmtime(&now))
			        << "nbuf is zero, this is unusual." << endl;
		    }
                }
                else 
                {
                    bzcount = 0;
                }
		SET_DEBUG_STACK;
		// Loop over the number of buffers to read. 
		for (i=0;i<nbuf;i++)
		{
		    SET_DEBUG_STACK;
		    //Rx.Buffers[Rx.ReadBuffer]->HexDump(&logFile);

		    /* 
		     * All data buffers should have 2 DLE's at the 
		     * beginning. 
		     */
		    if (Rx.Buffers[Rx.ReadBuffer]->Char(1) == FIX_DATA_REPLY)
		    {
			NOfix_count = 0;
			// Frame complete!
			ProcessFrame();
		    }
		    else
		    {
			NOfix_count++;
			/*
			 * This should be based on the timeout value.
			 * It is possible to go up to 5 seconds 
			 * without a fix. 
			 */
			if (NOfix_count%NOFix_test == 0)
			{
			    logFile << __FILE__ << " " << __LINE__
				    << " Buf #: " << Rx.ReadBuffer
				    << " No fixes in: " << dec << NOfix_count
				    << " tries. " << endl;
			}
		    }
		    SET_DEBUG_STACK;
		    if ((VerboseLevel&0x00000008) > 0)
		    {
			Rx.Buffers[Rx.ReadBuffer]->HexDump(&logFile);
		    }
		    SET_DEBUG_STACK;
		    Rx.gps->DecodeMessage( Rx.Buffers[Rx.ReadBuffer]);

		    SET_DEBUG_STACK;
		    Rx.ReadBuffer = (Rx.ReadBuffer+1)%N_BUFFERS;
		    SET_DEBUG_STACK;
		} // Loop over buffers
	    }
            else
            {
                //perror("semtimedwait");
		logFile << __FILE__ << " " << __LINE__
			<< " " << strerror(errno)
			<< endl;
            }
	} // End while loop 
    }
    else
    {
	cerr << "Error opening serial port!" << endl;
    }
    Terminate(0);
}


