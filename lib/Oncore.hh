/**
 ******************************************************************
 *
 * Module Name : Oncore.hh
 *
 * Author/Date : C.B. Lirakis / 11-Jan-01
 *
 * Description : 
 *
 * Restrictions/Limitations :
 *
 * Change Descriptions :
 * 09-Mar-19   CBL  Updated, it has been awhile. Version 1.6
 * 17-Feb-24   Getting a lot of errors, debugging and adding in 
 *             more comments. 
 * 18-Feb-24   process full sentance at a time. Remove state machine. 
 *             CObject provided by serial class, removed. 
 *             Added in Buffered class for handling serial IO stream. 
 *             deals with conversions. 
 *
 * Classification : Unclassified
 *
 * References :
 *
 *******************************************************************
 */
#ifndef __ONCORE_hh_
#  define __ONCORE_hh_
#  include <stdint.h>

#  include "Buffered.hh"

const int NRXDATA = 8;
const int VERMAJ=1;
const int VERMIN=6;
const int MAX_SATELLITE=12;

/*!
 * Class that keeps track of the visible data, eg: satellite 
 * elevation, doppler etc. 
 */
class VisibleData
{
    friend ostream& operator<<(ostream& output, const VisibleData &n); 
public:
    /*! Constructor */
    VisibleData(void) {ID=Health=0; Doppler=Elevation=Azimuth=0;};
    /*! Reset all variables that we maintain in this class. */
    inline void Clear(void) { ID=0; Doppler=Elevation=Azimuth=0; Health=0;};
    /*! Create equality between two VisibleData classes. */
    inline VisibleData& operator= (const VisibleData& rhs) {ID=rhs.ID; 
	Doppler=rhs.Doppler; Elevation=rhs.Elevation;Azimuth=rhs.Azimuth;
	Health=rhs.Health; return *this;};

    unsigned char ID;           /*! Satellite ID */
    int           Doppler;      /*! Doppler      */
    int           Elevation;    /*! Elevation, degrees. */
    int           Azimuth;      /*! Azimuth,   degrees */
    unsigned char Health;       /*! Bit packed string for health. */
};

/*!
 * How many visible satellites are there? Bb message
 */
class VisibleSatellites
{
    /*!
     * Friend class to print out the data maintained in the class. 
     */
    friend ostream& operator<<(ostream& output, const VisibleSatellites &n); 
public:
    /* Constructor */
    VisibleSatellites(void);
    /* Destructor  */
    ~VisibleSatellites(void);
    /*! Update the satellite data */
    uint32_t  Update(Buffered *buf);

    void Clear(void);

    inline unsigned char Checksum(void) const {return fCKSUM;};
    inline bool Filled(void) const {return fFilled;};
    inline int  Number(void) const {return fNSat;};
    inline VisibleData& Data(int i) const {if (i<MAX_SATELLITE) 
	    return fData[i];
	else 
	    return fData[MAX_SATELLITE-1];};

private:
    uint32_t      fNSat;   // {1:32} at time of writing, proably more. 
    VisibleData*  fData;
    unsigned char fCKSUM;
    bool          fFilled;
};
class RxData 
{
    friend ostream& operator<<(ostream& output, const RxData &n); 
public:
    RxData() {ID=Mode=SNR=Status=0;};
    inline void Clear() {ID=Mode=SNR=Status = 0;};

    unsigned char ID;
    unsigned char Mode;
    unsigned char SNR;
    unsigned char Status;
};

/*
 * Oncore message Ea
 */ 
class PositionStatus 
{
    friend ostream& operator<<(ostream& output, const PositionStatus &n); 
public:
    PositionStatus(void);
    int Update(Buffered *buf);
    ~PositionStatus(void);
    void Clear(void);

    // Data access methods
    inline struct timespec Time(void)   const {return fTime;};
    inline struct timespec PCTime(void) const {return fPCTime;};
    inline double Latitude(void)        const {return fLatitude;};
    inline double Longitude(void)       const {return fLongitude;};
    inline double Altitude(void)        const {return fAltitude;};
    inline double Velocity(void)        const {return fVelocity;};
    inline double Heading(void)         const {return fHeading;};
    inline double DOP(void)             const {return fDOP;};
    inline int    TDOP(void)            const {return ftDOP;};
    inline int    NSAT(void)            const {return fNum;};
    inline int    NSAT_Tracked(void)    const {return fTNum;};
    inline int    Status(void)          const {return fStatus;};
    inline bool   Filled(void)          const {return fFilled;};
    inline void   Empty(void)           {fFilled = false;};
    inline unsigned char Checksum(void) const {return fCKSUM;};
    inline double GetDelta(void)        const {return fDelta;};
    inline time_t GetUTC(void)          const {return fUTC;};

private:
    struct  timespec fTime, fPCTime;
    time_t           fUTC;
    double           fLatitude, fLongitude, fAltitude;
    double           fVelocity, fHeading;
    double           fDOP;                    // DOP
    int              ftDOP;                   // DOP type
    int              fNum;                    // Number of visible satellites.
    int              fTNum;                   // Number of satellites tracked.
    int              fStatus;
    bool             fFilled;
    RxData*          fSat;
    unsigned char    fCKSUM;
    // Seconds from PC clock.
    double           fDelta;     // Added this in to get at decode.
};

class SatTime
{
public:
    SatTime(void);
    void Update( unsigned char ID, unsigned int Time);
    friend ostream& operator<<(ostream& output, const SatTime &n); 

private:
    unsigned char fID;
    unsigned int  fTimeEst;  // nanoseconds
};

/*!
 * RAIM message class
 *  Message length 22 bytes.
 *  Decoding a response goes like:
 * @@EnotaapxxxxxxxxxpysreensffffsffffsffffsffffsffffsffffsffffsffffC<CR><LF>
 *
 * o - output message rate {0:255} 
 * t - Time RAIM algorithm on/off 0=off, 1 = on
 * aa - Time RAIM alarm limit in 100s of nanoseconds {3:65535}
 * P  - 1PPS control mode:
 *      0 - 1PPS output pulse is always off
 *      1 - 1PPS is always on
 *      2 - pulse active only when traccking at least one satellite
 *      3 - pulse active only when Time RAIM algorithm confirms
 *          solution error is within the user defined limit. 
 * xxxxxxxxx - not used
 * P - Pulse status 0 - off, 1 - on
 * y - 1PPS pulse sync - 0=pulse referneced to UTC, 1 = pulse 
 *     referenced to GPS time
 *
 * s - Time RAIM solution status 
 *     0 = OK: solution within alarm limits
 *     1 = ALARM: user specified limit exceeded
 *     2 = UNKNOWN: due to ...
 *         a) alarm threshold set too low
 *         b) Time RAIM turned off
 *         c) insufficient satellites being tracked
 *
 * Time RAIM Status
 *     0 = detection and isolation possible. 
 *     1 = detection only possible
 *     2 = neither possible. 
 *
 * ee - time solution one sigma {0:65535}
 *     accuracy estimate in nanoseconds
 *
 * n - negative sawtooth {-128:127}
 *     time error of next 1PPS pulse in nanoseconds
 *
 * For each of 8 receiver channels:
 * s - satellite ID {0 .. 37}
 *      ffff fractional GPS local {0:999999999}
 *      time estimate of satellite in nanoseconds
 * C - checksum
 * <CR> <LF>
 */
class RAIM {
    friend ostream& operator<<(ostream& output, const RAIM &n); 
public:
    RAIM(void);
    int Update(Buffered *buf);
    ~RAIM(void);
    void Clear(void);

    inline unsigned char  MessageRate(void)   const {return fMessageRate;};
    inline unsigned char  RAIMOnOff(void)     const {return fRAIMOnOff;};
    inline unsigned short Alarm(void)         const {return fAlarm;};
    inline unsigned char  PPSMode(void)       const {return fPPSMode;};
    inline unsigned char  PulseStatus(void)   const {return fPulseStatus;};
    inline unsigned char  PulseSync(void)     const {return fPulseSync;};
    inline unsigned char  SolutionStatus(void) const {return fRAIMSolutionStatus;};
    inline unsigned char  RAIMStatus(void)    const {return fRAIMStatus;};
    inline unsigned short TimeSolution(void)  const {return fTimeSolution;};
    inline char           Sawtooth(void)      const {return fSawtooth;};

    inline bool Filled(void)        const {return fFilled;};

    // Fill the values and make a message from it. 
    inline void MessageRate(unsigned char v) {fMessageRate = v;};
    inline void RAIMOnOff  (bool o)  {if (o) fRAIMOnOff = 1; else fRAIMOnOff = 0;};
    inline void Alarm      (short r) { fAlarm = r;};
    inline void PPSMode    (unsigned char v) {fPPSMode = v;};
    inline unsigned char Checksum(void) const {return fCKSUM;};
    unsigned char* Message(bool q); // q = true for query.
    
private:
    unsigned char  fMessageRate;  // Seconds between updates.
    unsigned char  fRAIMOnOff;    // 1 on 
    unsigned short fAlarm;        // RAIM alarm limit. 0 - off 1-65535 100ns steps
    unsigned char  fPPSMode;      // 0 pulse off. 1 1PPS on all the time.
                                  // 2 pulse active only when tracking sat.
                                  // 3 Pulse active when RAIM good. 
    // Response only messages
    unsigned char  fPulseStatus;  // 0 detection and isolation, 1 detecton only
    unsigned char  fPulseSync;    // 0 UTC, 1 GPS
    unsigned char  fRAIMSolutionStatus; 
    unsigned char  fRAIMStatus;
    unsigned short fTimeSolution; // nanoseconds
    char           fSawtooth;     
    SatTime        *fSat;

    unsigned char  fCKSUM;
    bool           fFilled;
};

class Oncore 
{
public:
    /// Default Constructor
    Oncore(void);
    /// Default destructor
    ~Oncore(void);

    /* 
     * constants used elsewhere in the program. 
     */
    const int kBufferSize = 128;  // size of recieve buffer. 



    unsigned char* MakeCommand(const char* Command, 
                               const unsigned char *data = NULL,
                               int DataSize = 0);

    inline bool   TimeValid(void)      const {return fPS->Filled();};
    inline bool   VisibleValid(void)   const { return fVS->Filled();};
    inline bool   RAIMValid(void)      const {return fRAIM->Filled();};
    inline void   TimeProcessed (void)       {fPS->Empty();};
    inline int    CommandSize(void)    const {return fCommandSize;};
    inline double GetDelta(void)       const {return fPS->GetDelta();};
    inline RAIM*  GetRAIM(void)              {return fRAIM;};
    inline PositionStatus*    GetPS(void)    {return fPS;};
    inline VisibleSatellites* GetVS(void)    {return fVS;};
    inline struct timespec    GPSTime(void) const {return fPS->Time();};
    inline struct timespec    PCTime(void)  const {return fPS->PCTime();};
    char*  CompiledOn(void) const;

protected:
    /* A full sentance is in the input buffer, parse it. */
    bool                  Parse(void);

    Buffered              *fBuffer;

private:
    unsigned char         CKSUM(unsigned char *buffer, int buffersize);
    uint32_t              DecodeAP(Buffered *buf);

    unsigned char         fCKSUM;

    // For assembling commands
    unsigned char         fCommandBuffer[64];
    int                   fCommandSize;

    // Data from various parsed messages. 
    PositionStatus*       fPS;
    VisibleSatellites*    fVS;
    RAIM*                 fRAIM;

    // 1PPS mode
    int                   fPPSMode;
};
#endif
