/**
 ******************************************************************
 *
 * Module Name : GPS.hh
 *
 * Author/Date : C.B. Lirakis / 09-Mar-19
 *
 * Description :  Encapulate all the Motorola Oncore setup and control. 
 *
 * Restrictions/Limitations : NONE
 *
 * Change Descriptions : NONE
 *
 * Classification : Unclassified
 *
 * References :
 *
 *******************************************************************
 */
#ifndef __GPS_hh_
#define __GPS_hh_
#  include "Oncore.hh"
#  include "SerialIO.h"

class ProcessTime;
class User;
class SharedMem2;
class GGA;

/// Motorola Oncore GPS class
class GPS : public Oncore, public SerialIO
{
public:
    /// Default Constructor
    GPS(const char *SerialPortName="/dev/ttyUSB0");
    /// Default destructor
    ~GPS();
    /// Do the deed!
    void Update(void);

    /// Stop the run. 
    inline void Stop(void) {fRun = false;};

    /*! Get the this pointer. */
    static GPS* GetThis(void) {return fGPS;};

private:

    // Fill the buffer one byte at a time. 

    bool         ReadByByte(void); 
    ProcessTime *fProcessTime;
    bool         fRun;
    User        *fUser;
    /**
     * Shared memory for position data. GPGGA
     * This is the outbound data. 
     */
    SharedMem2   *pSM_PositionData;
    GGA          *fGGA;

    static GPS*  fGPS;
};
#endif
