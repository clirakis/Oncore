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
 *
 * Classification : Unclassified
 *
 * References :
 *
 *
 * RCS header info.
 * ----------------
 * $RCSfile: time_thread.h,v $
 * $Author: clirakis $
 * $Date: 2004/01/04 18:34:53 $
 * $Locker: clirakis $
 * $Name:  $
 * $Revision: 1.2 $
 *
 * $Log: time_thread.h,v $
 * Revision 1.2  2004/01/04 18:34:53  clirakis
 * hmmmm, using struct timespec on time routines now.
 *
 * Revision 1.1  2003/12/31 14:39:57  clirakis
 * Initial revision
 *
 *
 *
 *******************************************************************
 */
#ifndef __PROCESS_TIME_h_
# define __PROCESS_TIME_h_

# include "lassen.hh"       // in support of t_GPS_Time
# include "sharedmem.hh"    // class definition for shared segment.
# include "Average.hh"

class ProcessTime {
 public:
    ProcessTime();
    ~ProcessTime();

    bool          DoIt( t_GPS_Time gpstime, const PreciseTime &pctime);
    inline void   SetVerbose(int v) {verbose = v;};
    inline int    Error() {return error;};
    double GetAverage() const {return data->Get();};
    inline double GetDelta()   const {return TimeDelta;};
    inline time_t GetLastSet() const {return LastTimeSet;};

 private:
    void         ProcessDelta();
    void         DumpData();

    SharedMem*   TimeSM;     // Communicate with time set daemon.
    SharedMem*   GPSDelta;   // Communicate our info with other programs.
    t_GPS_Time   oldgpstime;
    int          error, verbose;
    char*        errStr;
    Average      *data;

    // Parameters on time setting.
    int          NumberTimesSet;
    time_t       LastTimeSet;
    double       TimeDelta, OldTimeDelta;
    double       TimeSetWindow;
};
#endif
