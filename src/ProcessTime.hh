/**
 ******************************************************************
 *
 * Module Name : time_thread.h
 *
 * Author/Date : C.B. Lirakis / 11-Jan-01
 *
 * Description : 
 *
 * Restrictions/Limitations :
 *
 * Change Descriptions :
 * 21-Sep-20    CBL    Updated to use SharedMem2. 
 *
 * Classification : Unclassified
 *
 * References :
 *
 *******************************************************************
 */
#ifndef  __PROCESS_TIME_h_
# define __PROCESS_TIME_h_
# include <iostream>
# include "time.h"
# include "SharedMem2.hh"    // class definition for shared segment.
# include "Average.hh"

class ProcessTime {
 public:
    ProcessTime(bool verbose=false);
    ~ProcessTime(void);
    bool          Update(const struct timespec &gps, double delta);
    inline void   Verbose(bool v)        {fVerbose = v;};
    inline int    Error(void)      const {return fError;};
    inline double GetAverage(void) const {return fData->Get();};
//    inline double GetDelta(void)   const {return fDelta;};
    inline time_t GetLastSet(void) const {return fLastTimeSet;};

 private:
    void            ProcessDelta(double delta);
    SharedMem2*     fTimeSM;     // Communicate with time set daemon.
    SharedMem2*     fGPSDelta;   // Communicate our info with other programs.
    struct timespec fOldTime;
    int             fError;
    bool            fVerbose;
    Average*        fData;

    // Parameters on time setting.
    int             fNumberTimesSet;
    time_t          fLastTimeSet;
    double          fWindowHigh, fWindowLow;
};
#endif
