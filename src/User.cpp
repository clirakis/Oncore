/**
 ******************************************************************
 *
 * Module Name : User.cpp
 *
 * Author/Date : C.B. Lirakis / 15-Jan-02 
 *
 * Description : This is a cern-root based user interface to the 
 * Motorola Oncore timekeeping GPS. 
 *
 * Restrictions/Limitations : NONE
 *
 * Change Descriptions : 
 * 09-Mar-19 CBL Updated
 * 30-Oct-22 CBL Removed Geodtic references and replaced with proj8
 *
 * Classification : Unclassified
 *
 * References : https://root.cern/doc/v616/classes.html
 * https://github.com/root-project/jsroot/blob/master/docs/JSROOT.md
 * https://root.cern.ch/root/htmldoc/guides/JSROOT/JSROOT.html
 * https://proj.org/development/reference/cpp/index.html
 *
 *******************************************************************
 */
/* System includes. */
#include <iostream>
using namespace std;

// Proj 8 include
#include <proj.h>

/** Local Includes. */
#include "User.hh"
#include "tools.h"

/* Root Stuff. */
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TEnv.h"
#include "TSystem.h"
#include "TCanvas.h"
#include "TFrame.h"
#include "TMarker.h"
// #include "TMemFile.h"
#include "THttpServer.h"

/* Local includes */
#include "debug.h"
#include "filename.hh"
#include "CLogger.hh"
#include "GPS.hh"

const char *SensorName="Oncore";     // Sensor name. 

#define USE_HTTP 1

/**
 ******************************************************************
 *
 * Function Name : LoadPreferences
 *
 * Description : Load user preferences from the TEnv in the
 * class initialization. saves to "~/.oncorerc
 *
 * Inputs : none
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 10-Mar-19
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void User::LoadPreferences(void)
{
    SET_DEBUG_STACK;
    char     Path[256], *p;

    sprintf(Path, "$(HOME)%c.oncorerc", '/');
    p = gSystem->ExpandPathName(Path);

    CLogger::GetThis()->Log("# Trying to load preferences from %s\n", 
			    Path);
    fPrefs = new TEnv(p);
    fLon0 = fPrefs->GetValue("Geodetic.center.Longitude", -73.8930);
    fLat0 = fPrefs->GetValue("Geodetic.center.Latitude" ,  41.3083);

    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : SavePreferences()
 *
 * Description : Upon exit, update any persistant variables. 
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
void User::SavePreferences(void)
{
    SET_DEBUG_STACK;
    CLogger::GetThis()->Log("# Saving Preferences. \n");
    fPrefs->SetValue("Geodetic.center.Longitude", fLon0);
    fPrefs->SetValue("Geodetic.center.Latitude" , fLat0);
    fPrefs->SaveLevel(kEnvLocal);
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : SetupPlots
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
void User::SetupPlots(void)
{
    SET_DEBUG_STACK;
    /*
     * Create tree
     */
    fTree = new TTree( "GPS", "A tree of GPS data.");
    fTree->Branch("Time", &fTime,        "Time/D");
    fTree->Branch("Lat",  &fLat,         "Lat/D");
    fTree->Branch("Lon",  &fLon,         "Lon/D");
    fTree->Branch("X",    &fX,           "X/D");
    fTree->Branch("Y",    &fY,           "Y/D");
    fTree->Branch("Z",    &fZ,           "Z/D");
    fTree->Branch("NSV",  &fNSV,         "NSV/b");
    fTree->Branch("DOP",  &fDOP,         "DOP/D");
    fTree->Branch("TDOP", &fTDOP,        "TDOP/D");
    fTree->Branch("DX",   &fDX,          "DX/D");
    fTree->Branch("DY",   &fDY,          "DY/D");
    fTree->Branch("V",    &fVel,         "V/D");
    fTree->Branch("H",    &fHeading,     "H/D");
    fTree->Branch("TS",   &fTimeSolution,"TS/I");
    fTree->Branch("DT",   &fDelta,       "DT/D");
    fTree->Branch("SDAY", &fSDay,        "SDay/D");
    fTree->Branch("SDAY", &fDay,         "Day/I");

#if USE_HTTP==1
    /* 
     * Create a canvas to encompass the drawing of the two plots
     * for the server 
     */
    fCanvas1 = new TCanvas("Canvas1","GPS changes",200,10,700,500);
    fCanvas1->SetFillColor(42);
    fCanvas1->GetFrame()->SetFillColor(21);
    fCanvas1->GetFrame()->SetBorderSize(6);
    fCanvas1->GetFrame()->SetBorderMode(-1);
    fCanvas1->Divide(2,2);
    fCanvas1->SetGrid(1);
    fCanvas1->SetGrid(2);

    /*
     * Create the markers to display the current and 
     * average positions
     */
    fCurrent = new TMarker(0.0, 0.0, 2);
    fCurrent->SetMarkerColor(kGreen);
    fAverage = new TMarker(0.0, 0.0, 28);
    fAverage->SetMarkerColor(kRed);
#else
    fCanvas1 = 0;
    fCurrent = 0;
    fAverage = 0;
#endif
    // Handy 1 and 2 D histograms. 
    fDYDX = new TH2D("dXdY","dX vs dY", 200, -25.0, 25.0, 200, -25.0, 25.0);
    fDYDX->SetXTitle("dX(meters)");
    fDYDX->SetYTitle("dY(meters)");
    f1Z   = new TH1D("Z", "Altitude", 40, -10.0, 10.0);
    f1T   = new TH1D("TS", "Tims Solution(sigma ns)", 256, 0.0, 128.0);

    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : User Constructor
 *
 * Description : Create the tree, and connect it to a file. 
 *
 * Inputs : None
 *
 * Returns : None
 *
 * Error Conditions : None
 *
 *******************************************************************
 */
User::User(void)
{
    SET_DEBUG_STACK;
    char *name;
    char temp[64];
    PJ_COORD c, c_out;

    fCanvas1 = NULL;
    fDYDX    = NULL;
    f1Z      = NULL;
    f1T      = NULL;
    fCurrent = NULL;
    fAverage = NULL;
    fServ    = NULL;

    /*
     * Initialize Root package.
     * We don't really need to track the return pointer. 
     * We just need to initialize it. 
     */
    ::new TROOT("ONCORE","Oncore Data analysis");

    // Load user preferences. 
    LoadPreferences();

    /*
     * Create a filename class. 
     */
    fn = new FileName(SensorName, "root", One_Day);

    /* Connect to a file. */
    name = (char *) fn->GetUniqueName();
    SET_DEBUG_STACK;
    /* Create disk file */
    fHfile = new TFile( name,"RECREATE","GPS data analysis");

    SetupPlots();
    fChangeFile = kFALSE;

#if USE_HTTP==1
    // Setup HTTP server
    //const char *ServerEntry = "http:8080/none?top=GPS";
    const char *ServerEntry = "http:8081";
    fServ = new THttpServer(ServerEntry);

    // Specifically don't have a timeout process. 
    // We will update the page in the Update proc call. 
    fServ->SetTimer(0, kTRUE);

    // Allow dialog between server and client
    fServ->SetReadOnly(kFALSE);

    // use custom web page as default
    //fServ->SetDefaultPage("oncore.htm");

    CLogger::GetThis()->Log("# HTTP server started, %s\n",ServerEntry);
#else
    fServ = 0;
#endif
    fNUpdate = 0;


    /* Setup the projection. */

    // Calculate zone
    uint32_t zone = Zone(fLon0);
    sprintf(temp, "EPSG:326%2.2d", zone);
    CLogger::GetThis()->Log("# Creating projection from WGS84 to %s\n", temp);

    fP =  proj_create_crs_to_crs(PJ_DEFAULT_CTX,
				 "EPSG:4326",  // WGS84
				 temp, // UTM 
				 NULL);
    // Get the XY of the center in the projection 
    c = proj_coord(fLon0, fLat0, 0.0, 0.0);
    c_out = proj_trans(fP, PJ_FWD, c);
    fX0 = c_out.xy.x;
    fY0 = c_out.xy.y;
    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name : Update
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
void User::Update(Double_t dt)
{
    SET_DEBUG_STACK;
    CLogger *Logger = CLogger::GetThis();
    GPS *pGPS = GPS::GetThis();

    const PositionStatus* pos = pGPS->GetPS();
    const RAIM*          raim = pGPS->GetRAIM();
    Double_t             dX, dY;
    char                 *name;
    time_t               now;
    char                 msg[64];
    PJ_COORD             c, c_out;
    struct tm            *tm_val;

    if (fn->ChangeNames() || fChangeFile)
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
	time(&now);
	strftime (msg, sizeof(msg), "%m-%d-%y %H:%M:%S", gmtime(&now));
	Logger->Log("# changed file name on: %s to: %s\n", msg,name);
	
	fChangeFile = kFALSE;
    }

    //*CLogger::GetThis()->LogPtr() << *pos;


    /*
     * Pos in this case is in Degrees not radians. FIXME. 
     */
    time_t epoch  = pos->Time().tv_sec; 
    fTime         = epoch;
    tm_val        = localtime(&epoch);
    fDay          = tm_val->tm_yday;
    fSDay         = (tm_val->tm_hour + 60.0 * tm_val->tm_min)*60.0 
	+ tm_val->tm_sec;

    fLat          = pos->Latitude();   //*TMath::RadToDeg();
    fLon          = pos->Longitude();  //*TMath::RadToDeg();

    // Make the forward projection. 
    c = proj_coord(fLon, fLat, 0.0, 0.0); 
    c_out = proj_trans(fP, PJ_FWD, c);
    dX = c_out.xy.x - fX0;
    dY = c_out.xy.y - fY0;

    fX            = c_out.xy.x;
    fY            = c_out.xy.y;
    fZ            = pos->Altitude(); 
    fNSV          = pos->NSAT();
    fDOP          = pos->DOP();
    fTDOP         = pos->TDOP();
    fDX           = dX;
    fDY           = dY;
    fVel          = pos->Velocity();
    fHeading      = pos->Heading();
    fTimeSolution = raim->TimeSolution();
    fDelta        = dt;


    /* Fill the Tree. */
    fTree->Fill();

    /* 1 and 2 D histograms */
    fDYDX->Fill(fDY,fDX);
    f1Z->Fill(fZ);
    f1T->Fill(fTimeSolution);

    // Update the web server if available. 
    if (fServ)
    {
	 fServ->ProcessRequests();
         if (fNUpdate==0) 
	 {
	     fCanvas1->cd(1);
	     fDYDX->Draw();
	     fCurrent->SetX(fDY);
	     fCurrent->SetY(fDX);
	     fCurrent->Draw();

	     fAverage->SetX(fDYDX->GetMean(1));
	     fAverage->SetY(fDYDX->GetMean(2));
	     fAverage->Draw();

	     fCanvas1->cd(2);
	     f1Z->Draw();

	     fCanvas1->cd(3);
	     f1T->Draw();
	 }
         fCanvas1->Modified();
         fCanvas1->Update();
    }
    fNUpdate = (fNUpdate+1)%5; // Only update canvas every 5 fills. 
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
User::~User(void)
{
    SET_DEBUG_STACK;
    SavePreferences();
    SET_DEBUG_STACK;

    // Save all objects in this file
    fHfile->Write();

    // Close the file. Note that this is automatically done when you leave
    // the application.
    fHfile->Close();

    //delete fDYDX;
    //delete f1Z;
    delete fCurrent;
    delete fAverage;
    delete fCanvas1;

    SET_DEBUG_STACK;

    delete fHfile;
    delete fServ;
    delete fn;

    proj_destroy(fP);

    SET_DEBUG_STACK;
}
