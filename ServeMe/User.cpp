/**
 ******************************************************************
 *
 * Module Name : User.cpp
 *
 * Author/Date : C.B. Lirakis / 15-Jan-02 
 *
 * Description : root analysis
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
 * $RCSfile: User.cpp,v $
 * $Author: clirakis $
 * $Date: 2003/08/08 17:24:47 $
 * $Locker:  $
 * $Name:  $
 * $Revision: 1.1 $
 *
 * $Log: User.cpp,v $
 * Revision 1.1  2003/08/08 17:24:47  clirakis
 * Initial revision
 *
 *
 *
 *******************************************************************
 */

#ifndef lint
/** RCS Information */
static char rcsid[]="$Header: /home/clirakis/code/lassen/RCS/User.cpp,v 1.1 2003/08/08 17:24:47 clirakis Exp $";
#endif

/* System includes. */
#include <iostream>
using namespace std;


/** Local Includes. */
#include "User.hh"
#include "geographic.h"
#include "ProcessTime.hh"
#include "tools.h"

/* Root Stuff. */
#include "TROOT.h"
#include "TFile.h"
#include "TNtupleD.h"
#include "TH1.h"
#include "TH2.h"
#include "TEnv.h"
#include "TSystem.h"
#include "TMath.h"

#include "debug.h"
#include "filename.h"


const char *SensorName="GPS";     // Sensor name. 

/**
 ******************************************************************
 *
 * Function Name : LoadPreferences
 *
 * Description :
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 28-June-08
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void User::LoadPreferences()
{
    SET_DEBUG_STACK;
    char     Path[256], *p;
    Double_t Lat, Lon;

    sprintf(Path, "$(HOME)%s.lassenrc", "/");
    p = gSystem->ExpandPathName(Path);
    fPrefs = new TEnv(p);
    Lat = fPrefs->GetValue("GPS.HomeLatitude" , (Double_t)  41.5);
    Lon = fPrefs->GetValue("GPS.HomeLongitude", (Double_t) -71.3);

    fGeo    = new Geographic(41.5, -71.3);
    Lat    *= TMath::RadToDeg();
    Lon    *= TMath::RadToDeg();
    fMyXY   = fGeo->ToXY(Lat, Lon);
    fMyXY.z = fPrefs->GetValue("GPS.HomeZ", (Double_t) 37.0);
    fSerialPortName = StrDup( fPrefs->GetValue("Serial.Port","/dev/ttyS0"));
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : SavePreferences()
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
void User::SavePreferences()
{
    SET_DEBUG_STACK;
    fPrefs->SetValue("GPS.HomeLatitude" , fAvgLat->Get());
    fPrefs->SetValue("GPS.HomeLongitude", fAvgLon->Get());
    fPrefs->SetValue("GPS.HomeZ"        , fAvgH->Get());
    fPrefs->SetValue("Serial.Port", fSerialPortName);
    fPrefs->SaveLevel(kEnvLocal);
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : SetupPlots()
 *
 * Description : Create the ntuple here. Allocate the array value filling it.
 *
 * Inputs : None
 *
 * Returns : None
 *
 * Error Conditions : None
 * 
 * Unit Tested on: 28-June-08
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void User::SetupPlots()
{
    SET_DEBUG_STACK;
    Int_t NValues = 0;
    /*
     * Setup NTUPLE
     * These are the names of the variables that we want to monitor.
     */
    char *raw_names = "Time:Lat:Lon:X:Y:Z:NSV:PDOP:HDOP:VDOP:TDOP:DX:DY:DZ:GPSSEC:LastSet:Delta:AvgDelta:SysTime";
    SET_DEBUG_STACK;
    /*
     * Create Ntuple.
     */
    fNtuple = new TNtupleD("GPS","lassen ntuple",raw_names);

    /*
     * Create a vector to pass data in on. 
     * determine how many : there are in the raw_names string and add 1.
     */
    NValues = fNtuple->GetNvar();
    fValues = new Double_t[NValues];
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : User Constructor
 *
 * Description : Create the ntuple, and connect it to a file. 
 *
 * Inputs : None
 *
 * Returns : None
 *
 * Error Conditions : None
 *
 *******************************************************************
 */
User::User()
{
    SET_DEBUG_STACK;
    char *name;

    /*
     * Initialize Root package.
     * We don't really need to track the return pointer. 
     * We just need to initialize it. 
     */
    ::new TROOT("LASSEN","Lassen Data analysis");

    // Load user preferences. 
    LoadPreferences();

    /*
     * Create a filename class. 
     */
    fn = new FileName(SensorName, "root", One_Day);

    /* Connect to a file. */
    name = (char *) fn->GetUniqueName();
    SET_DEBUG_STACK;
    fHfile = new TFile( name,"RECREATE","GPS data analysis");
    SetupPlots();
    fTimer  = new PreciseTime();
    fAvgLat = new Average(100);
    fAvgLon = new Average(100);
    fAvgH   = new Average(100);
}

/**
 ******************************************************************
 *
 * Function Name : Do
 *
 * Description : Parse the values. Use the lassen data and the 
 * projection setup in the constructor to project lat lon to XY.
 * store the data in the ntuple.
 *
 * Inputs : Lassen *LP - the lassen class with all the data in it.
 *
 * Returns : NONE
 *
 * Error Conditions : NONE
 *
 *******************************************************************
 */
void User::Do(const Lassen *LP, const ProcessTime* TM)
{
    SET_DEBUG_STACK;
    SolutionStatus *SolStat; 
    Position       *pos;
    FPoint         delta;
    char           *name;

    if (fn->ChangeNames())
    {
	// Save all objects in this file
	fHfile->Write();
	
	// Close the file. Note that this is automatically done when you leave
	// the application.
	fHfile->Close();
	delete fHfile;
	/* Connect to a file. */
        name = (char *) fn->GetUniqueName();
	fn->NewUpdateTime();
	SET_DEBUG_STACK;

	fHfile = new TFile( name,"RECREATE","GPS data analysis");
	// And we are ready to go again. 
	SET_DEBUG_STACK;
        SetupPlots();
    }

    pos     = LP->GetPositionData();
    SolStat = LP->LastStatus();

    //cout << pos->Latitude*TMath::RadToDeg() << endl;
    if ((pos->Latitude>30.0*TMath::DegToRad()) && (pos->Latitude<50.0*TMath::DegToRad()))
    {
	fAvgLat->AddElement(pos->Latitude * TMath::RadToDeg());
    }

    if ((pos->Longitude>-80.0*TMath::DegToRad()) && (pos->Longitude<-60.0*TMath::DegToRad()))
    {
	fAvgLon->AddElement(pos->Longitude * TMath::RadToDeg());
    }
    if (fabs(pos->Altitude)<200.0)
    {
	fAvgH->AddElement(pos->Altitude);
    }
#if 0
    cout << pos->Latitude * 180.0/M_PI << " "
	 << pos->Longitude * 180.0/M_PI
	 << endl;
#endif
    FPoint XYZP(fGeo->ToXY(pos->Latitude,pos->Longitude));
    delta = XYZP - fMyXY;
    fTimer->Now();

    fValues[0]  = pos->Sec;                  // Verified
    fValues[1]  = (float) pos->Latitude;     // Verified
    fValues[2]  = (float) pos->Longitude;    // Verified
    fValues[3]  = XYZP.x;                    // Verified
    fValues[4]  = XYZP.y;                    // Verified
    fValues[5]  = (float) pos->Altitude;     // Verified
    fValues[6]  = SolStat->NSV;              // Verified
    fValues[7]  = SolStat->PDOP;             // Verified
    fValues[8]  = SolStat->HDOP;             // Verified
    fValues[9]  = SolStat->VDOP;             // Verified
    fValues[10] = SolStat->TDOP;             // Verified
    fValues[11] = delta.x;                   // Verified
    fValues[12] = delta.y;                   // Verified
    fValues[13] = pos->Altitude - fMyXY.z;   // Verified
    fValues[14] = LP->Time().TimeOfWeek;    // GPSSEC 
    fValues[15] = TM->GetLastSet();          // Verified
    fValues[16] = TM->GetDelta();            // Verified OK
    fValues[17] = TM->GetAverage();          // Not good
    fValues[18] = fTimer->GetDouble();       // Verified
    SET_DEBUG_STACK;

    fNtuple->Fill(fValues);

    SET_DEBUG_STACK;

}
/**
 ******************************************************************
 *
 * Function Name : User destructor
 *
 * Description : write the final data out on the ntuple, close the 
 * file and delete all the stuff we allocated
 *
 * Inputs : None
 *
 * Returns : None
 *
 * Error Conditions : None
 *
 *******************************************************************
 */
User::~User()
{
    SET_DEBUG_STACK;
    SavePreferences();
    SET_DEBUG_STACK;

    // Save all objects in this file
    fHfile->Write();

    // Close the file. Note that this is automatically done when you leave
    // the application.
    fHfile->Close();
    SET_DEBUG_STACK;

    delete fAvgLat;
    delete fAvgLon;
    delete fAvgH;
    delete fHfile;
    delete fValues;
    delete fTimer;
    delete fGeo;
    delete fn;
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : DumpValues
 *
 * Description : Dump the data going into the ntuple.
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
void User::DumpValues()
{
    for (int i=0;i<fNtuple->GetNvar();i++)
    {
	cout << i << " " << fValues[i] << endl;
    }
}
