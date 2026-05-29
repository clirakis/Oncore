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
#  include <sstream> 
#  include <stdint.h>
#  include "CObject.hh" // Base class with all kinds of intermediate
#  include "NMEA_GPS.hh"  // Class definitions for NMEA GPS objects.
#  include "H5Logger.hh"
#  include "filename.hh"
#  include "Oncore.hh"
#  include "SerialIO.h"

class ProcessTime;
class User;
class SharedMem2;
class GGA;

/// Motorola Oncore GPS class
class GPS : public CObject, Oncore
{
public:
    /** 
     * Build on CObject error codes. 
     */
    enum {ENO_FILE=1, ECONFIG_READ_FAIL, ECONFIG_WRITE_FAIL};

    /// Default Constructor
    GPS(void);
    /// Default destructor
    ~GPS();
    /// Do the deed!
    void Update(void);


    /**
     * Tell the program to stop. 
     */
    void Stop(void) {fRun=false;};

    /*!
     * Has the display been requested?
     */
    inline bool DisplayOn(void) const {return fDisplay;};

    /*!
     * Write the data to the HDF5 logger if open and the IPC if it
     * exists. 
     */
    void LogData(void);

    /*! 
     * Ask the program to change filenames. 
     */
    void UpdateFileName(void);

    inline const char* Filespec(void) {return fn->GetCurrentFilespec();};


    /*! Get the this pointer. */
    static GPS* GetThis(void) {return fGPS;};
    /**
     * Control bits - control verbosity of output
     */
    static const unsigned int kVerboseBasic    = 0x0001;
    static const unsigned int kVerboseMSG      = 0x0002;
    static const unsigned int kVerboseFrame    = 0x0010;
    static const unsigned int kVerboseMax      = 0x8000;

private:
    // Fill the buffer one byte at a time. 
    bool         ReadByByte(void); 
    ProcessTime *fProcessTime;
    bool         fRun;

    /*!
     * Tool to manage the file name. 
     */
    FileName*     fn;          /*! File nameing utilities. hdf5 log */

    /*!
     * Logging tool, log data to HDF5 file.  
     */
    H5Logger     *f5Logger;
    SerialIO     *fSerial;

    /*!
     * Serial port name. 
     */
    std::string  fSerialPortName;
    /*!
     * Projection details. 
     */
    PJ     *fP;            /* Pointer to projection */

    double fLatitude;      /*! Starting Latitude  */
    double fLongitude;     /*! Starting Longitude */
    double fAltitude;      /*! Starting Altitude  */
    bool   fReset;         /*! Reset requested.   */
    double fGeoLatitude;   /*! Geodetic Latitude if needed for projections. */
    double fGeoLongitude;  /*! Geodetic Longitude if needed for projections.*/
    double fX0, fY0;       /*! Geodetic center XY */
    bool   fDisplay;       /*! Turn curses display on. */
    bool   fLogging;       /*! Turn logging on. */

    /**
     * Shared memory for position data. GPGGA
     * This is the outbound data. 
     */
    SharedMem2   *pSM_PositionData;
    GGA          *fGGA;

    /*!
     * Open the data logger. 
     */
    bool OpenLogFile(void);
    /*!
     * Read the configuration file. 
     */
    bool ReadConfiguration(void);
    /*!
     * Write the configuration file. 
     */
    bool WriteConfiguration(void);


    static GPS*  fGPS;
};
#endif
