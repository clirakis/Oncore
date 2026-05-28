/**
 ******************************************************************
 *
 * Module Name : Position.hh
 *
 * Author/Date : C.B. Lirakis / 12-Sep-10
 *
 * Description : Position data from Oncore device.
 *
 * Restrictions/Limitations :
 *
 * Change Descriptions :
 *
 * Classification : Unclassified
 *
 * References :
 *
 *******************************************************************
 */
#ifndef __POSITION_hh_
#define __POSITION_hh_
#  include <iostream>
#  include <fstream>
#  include <QWidget>

class QLabel;
class QTimerEvent;
class QCheckBox;

///  documentation here. 
class Position : public QWidget
{
    Q_OBJECT
public:
    /// Default Constructor
    Position(QWidget *parent = 0);
    /// Default destructor
    ~Position();
    void Update(void *);
protected:

private slots:
    /*
     * Save data to file.
     */
    void  SaveData();

private:

    QLabel     *fLatitude, *fLongitude, *fHeight;
    QLabel     *fVelocity, *fHeading, *fDOP;
    QLabel     *fTime;
    // Based on the data in TDOP (DOP type) show status
    QLabel     *fAntennaStatus, *fHDOP2or3;
    QLabel     *fVisibleSatellite, *fTrackedSatellite;
    // Receiver status lights.
    QLabel     *fAlmanac, *fVisibleSatellites, *fGeometry, *fFix;
    // Fix is acquiring satellites, Differential, 2D or 3D

    QLabel     *fFilename;
    QCheckBox  *fEnableSave;

    std::ofstream   *fLogfile;
};
#endif
