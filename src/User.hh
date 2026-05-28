/**
 ******************************************************************
 *
 * Module Name : USER.hh
 *
 * Author/Date : C.B. Lirakis / 10-Mar-19
 *
 * Description : User code for Oncore GPS. 
 *
 * Restrictions/Limitations : NONE
 *
 * Change Descriptions : 
 * 19-Feb-24   Added time delta into tree. 
 *
 * Classification : Unclassified
 *
 * References : Code/Oncore/Oncore/Manual/ch6.pdf
 *
 *
 *******************************************************************
 */
#ifndef __USER_hh_
#define __USER_hh_
#include <proj.h>
# include "TROOT.h"

class TTree;
class TFile;
class TH1D;
class TH2D;
class TEnv;
class Geodetic;
class FileName;
class PreciseTime;
class TMemFile;
class THttpServer;
class TCanvas;
class TMarker;

class User {
public:
    User();
    ~User();
    void Update(Double_t dt);
    void ChangeFile(void) {fChangeFile = kTRUE;};

private:
    void LoadPreferences(void);
    void SavePreferences(void);
    void SetupPlots(void);
    void DumpValues(void);

    /* root variables. */
    TFile*       fHfile;    /*! Disk File open to dump buffers to. */
    TTree*       fTree;     /*! The actual ntuple. */
    TH2D*        fPosition; /*! Keep a 2D position histogram */
    TEnv*        fPrefs;    /*! preferences  */
    FileName*    fn;        /*! File nameing utilities. */
    PreciseTime* fTimer;    /*! */
    Bool_t       fChangeFile; /*! Tell the system to change the file name. */

    // Tree elements
    ULong_t      fTime;
    Double_t     fLat, fLon, fX, fY, fZ;
    Double_t     fDOP, fTDOP, fDX, fDY;
    UChar_t      fNSV;
    Double_t     fVel;     /* Velocity */
    Double_t     fHeading; 
    UInt_t       fNUpdate;
    UInt_t       fTimeSolution;
    Double_t     fDelta;     /*! PC - GPS time */
    Double_t     fSDay;      /*! Seconds into a given day. */
    UInt_t       fDay;       /*! Day into year. */

    // Make a canvas for the next two histograms. 
    TCanvas*     fCanvas1;
    // Histograms that are worth making
    TH2D*        fDYDX;
    TH1D*        f1Z;
    TH1D*        f1T; // Time solution status

    // Create a marker to show the last point and the current average. 
    TMarker      *fCurrent, *fAverage;

    // Proj Variables
    PJ           *fP;
    Double_t     fLat0, fLon0;
    Double_t     fX0, fY0;

    // http server
    THttpServer* fServ;
};
#endif
