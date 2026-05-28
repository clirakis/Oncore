/**
 ******************************************************************
 *
 * Module Name : USER.hh
 *
 * Author/Date : C.B. Lirakis / 13-Jun-02
 *
 * Description : Function prototypes for user input. 
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
 * $RCSfile: User.h,v $
 * $Author: clirakis $
 * $Date: 2003/08/08 17:26:28 $
 * $Locker: clirakis $
 * $Name:  $
 * $Revision: 1.1 $
 *
 * $Log: User.h,v $
 * Revision 1.1  2003/08/08 17:26:28  clirakis
 * Initial revision
 *
 *
 *******************************************************************
 */
#ifndef __USER_hh_
#define __USER_hh_
# include "lassen.hh"
# include "fpoint.h"
# include "TNtupleD.h"

class TFile;
class TH2D;
class TEnv;
class Geographic;
class FileName;
class PreciseTime;
class ProcessTime;
class Average;

class User {
public:
    User();
    ~User();
    void Do(const Lassen *LP, const ProcessTime* TM);
    inline const char* GetSerialPortName() {return fSerialPortName;};

private:
    void LoadPreferences();
    void SavePreferences();
    void SetupPlots();
    void DumpValues();

    /* root variables. */
    TFile*       fHfile;    // File open to dump buffers to.
    TNtupleD*    fNtuple;   // The actual ntuple.
    TH2D*        fPosition; // Keep a 2D position histogram
    FPoint       fMyXY;     // Starting Projection value in meters. 
    TEnv*        fPrefs;
    Geographic*  fGeo;
    FileName*    fn;        // File nameing utilities.
    PreciseTime* fTimer;
    char *       fSerialPortName;
    Average      *fAvgLat, *fAvgLon, *fAvgH;
    Double_t*    fValues;
};
#endif
