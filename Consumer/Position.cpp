/********************************************************************
 *
 * Module Name : Position.cpp
 *
 * Author/Date : C.B. Lirakis / 12-Sep-10
 *
 * Description : Position tab element class
 *
 * Restrictions/Limitations :
 *
 * Change Descriptions :
 *
 * Classification : Unclassified
 *
 * References :
 *
 ********************************************************************/
// System includes.
#include <iostream>
using namespace std;
#include <string>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

// Qt includes
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QTimerEvent>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QPalette>
#include <QDateTime>
#include <QFileDialog>
#include <QStringList>

// Local Includes.
#include "debug.h"
#include "Position.hh"
#include "Oncore.hh"

/**
 ******************************************************************
 *
 * Function Name : module constructor
 *
 * Description : Construct the first tab on the Oncore UI. 
 *
 * Inputs : parent widget
 *
 * Returns : fully constructed tab1
 *
 * Error Conditions :
 * 
 * Unit Tested on: 12-Sep-10
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
Position::Position(QWidget *parent) : QWidget(parent)
{
    SET_DEBUG_STACK;
    QVBoxLayout *VBox       = new QVBoxLayout; 
    QHBoxLayout *HBox       = new QHBoxLayout;
    QGroupBox   *Group      = new QGroupBox;
    QGroupBox   *Lights     = new QGroupBox;
    QGroupBox   *Status     = new QGroupBox;
    QGridLayout *Grid       = new QGridLayout;
    QGridLayout *StatusGrid = new QGridLayout;
    QLabel      *ql;
    int         row;

    fLogfile = 0;
    row      = 0;


    Lights->setTitle("Receiver Status");
    fAlmanac = new QLabel("Almanac");
    fAlmanac->setFrameStyle(QFrame::Panel | QFrame::Raised);
    fAlmanac->setAutoFillBackground( true );
    fAlmanac->setPalette(QPalette(Qt::green));
    HBox->addWidget(fAlmanac);

    fVisibleSatellites = new QLabel("Visible Satellites");
    fVisibleSatellites->setFrameStyle(QFrame::Panel | QFrame::Raised);
    fVisibleSatellites->setAutoFillBackground( true );
    fVisibleSatellites->setPalette(QPalette(Qt::green));
    HBox->addWidget(fVisibleSatellites);


    fGeometry = new QLabel("Geometry");
    fGeometry->setFrameStyle(QFrame::Panel | QFrame::Raised);
    fGeometry->setAutoFillBackground( true );
    fGeometry->setPalette(QPalette(Qt::green));
    HBox->addWidget(fGeometry);

    fFix = new QLabel("Fix");
    fFix->setFrameStyle(QFrame::Panel | QFrame::Raised);
    fFix->setAutoFillBackground( true );
    fFix->setPalette(QPalette(Qt::green));
    HBox->addWidget(fFix);

    Lights->setLayout(HBox);
    VBox->addWidget(Lights);
    // =================================================
    Status->setTitle("Fix Status");
    row = 0;
    ql = new QLabel ("Antenna:");
    StatusGrid->addWidget(ql, row, 0);
    fAntennaStatus = new QLabel("OK");
    fAntennaStatus->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    StatusGrid->addWidget(fAntennaStatus, row, 1);
    row++;

    ql = new QLabel ("Visible:");
    StatusGrid->addWidget(ql, row, 0);
    fVisibleSatellite = new QLabel("0");
    fVisibleSatellite->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    StatusGrid->addWidget(fVisibleSatellite, row, 1);
    row++;

    ql = new QLabel ("Tracked:");
    StatusGrid->addWidget(ql, row, 0);
    fTrackedSatellite = new QLabel("0");
    fTrackedSatellite->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    StatusGrid->addWidget(fTrackedSatellite, row, 1);
    row++;


    Status->setLayout(StatusGrid);
    VBox->addWidget(Status);
    // =================================================
    Group->setTitle("Position");

    ql = new QLabel(tr("Time:"));
    Grid->addWidget( ql, row,0);
    fTime = new QLabel(tr("00:00:00"));
    fTime->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    Grid->addWidget( fTime, row, 1);
    row++;

    ql = new QLabel(tr("Latitude:"));
    Grid->addWidget( ql, row,0);
    fLatitude = new QLabel(tr("00 00 00"));
    fLatitude->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    Grid->addWidget( fLatitude, row, 1);
    row++;

    ql = new QLabel(tr("Longitude:"));
    Grid->addWidget( ql, row,0);
    fLongitude = new QLabel(tr("00:00:00"));
    fLongitude->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    Grid->addWidget( fLongitude, row, 1);
    row++;

    ql = new QLabel(tr("Height:"));
    Grid->addWidget( ql, row,0);
    fHeight = new QLabel(tr("0"));
    fHeight->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    Grid->addWidget( fHeight, row, 1);
    row++;

    ql = new QLabel(tr("Heading:"));
    Grid->addWidget( ql, row,0);
    fHeading = new QLabel(tr("0"));
    fHeading->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    Grid->addWidget( fHeading, row, 1);
    row++;

    ql = new QLabel(tr("Velocity:"));
    Grid->addWidget( ql, row,0);
    fVelocity = new QLabel(tr("0"));
    fVelocity->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    Grid->addWidget( fVelocity, row, 1);
    row++;

    ql = new QLabel("DOP:");
    Grid->addWidget(ql, row, 0);
    fDOP = new QLabel("0");
    fDOP->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    Grid->addWidget(fDOP, row, 1);
    row++;

    ql = new QLabel("DOP Fix:");
    Grid->addWidget(ql, row, 0);
    fHDOP2or3 = new QLabel("UNKNOWN");
    fHDOP2or3->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    Grid->addWidget(fHDOP2or3, row, 1);
    row++;

    Group->setLayout(Grid);
    VBox->addWidget(Group);
   
    VBox->addStretch(1);
    setLayout(VBox);
    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name : module destructor
 *
 * Description : clean up after ourselves, specifically deregister 
 * the timer timeout
 *
 * Inputs : none
 *
 * Returns : nonw
 *
 * Error Conditions :
 * 
 * Unit Tested on:  25-Jul-10
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
Position::~Position ()
{
    SET_DEBUG_STACK;
    if (fLogfile)
    {
        fLogfile->close();
        delete fLogfile;
    }
    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name : UpdateTime
 *
 * Description : Update the dialog display with current TFP time
 * as well as computer time
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : Failure to read TFP
 * 
 * Unit Tested on: 25-Jul-10
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void Position::Update(void *f)
{
    SET_DEBUG_STACK;
    struct timespec host_now;
    unsigned char   status = 0;
    char            msg[32];
    struct timeval  ttv;
    struct timezone ttz;
    int             OffsetSeconds;

    Oncore *oncore = (Oncore *) f;
    struct timespec gps = oncore->GPSTime();
    QDateTime now;
    now.setTime_t(gps.tv_sec);
    fTime->setText(now.toString());


    gettimeofday( &ttv, &ttz);
    OffsetSeconds = ttz.tz_minuteswest*60;

    clock_gettime(CLOCK_REALTIME, &host_now);   // Host time
    // Add in time to host clock to make it UTC. 
    host_now.tv_sec += OffsetSeconds;

    status = 0;
    if ((status&0x01) == 0)
    {
        fAlmanac->setPalette(QPalette(Qt::green));   
    }
    else
    {
        fAlmanac->setPalette(QPalette(Qt::red));   
    }
    if ((status&0x02) == 0)
    {
        fVisibleSatellites->setPalette(QPalette(Qt::green));   
    }
    else
    {
        fVisibleSatellites->setPalette(QPalette(Qt::red));   
    }
    if ((status&0x04) == 0)
    {
        fGeometry->setPalette(QPalette(Qt::green));   
    }
    else
    {
        fGeometry->setPalette(QPalette(Qt::red));   
    }
    if ((status&0x04) == 0)
    {
        fFix->setPalette(QPalette(Qt::green));   
    }
    else
    {
        fFix->setPalette(QPalette(Qt::red));   
    }

#if 0
    if (fLogfile!=NULL)
    {
        char msg[32], msg2[32];
        strftime( msg, sizeof(msg), "%T", gmtime(&host_now.tv_sec));
        strftime( msg2, sizeof(msg2), "%T", gmtime(&bcnow.tv_sec));
        *fLogfile << msg
                  << " " << host_now.tv_nsec
                  << " " << msg2
                  << " " << bcnow.tv_nsec
                  << " " << delta
                  << endl;
    }
#endif
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name :
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
void Position::SaveData()
{
    SET_DEBUG_STACK;
    if(fEnableSave->checkState() == Qt::Checked)
    {
        // Open modal dialog
        QFileDialog *fd = new QFileDialog(this, "Save to:");
        fd->setFilter( "data (*.dat *.txt)" );
        fd->setFileMode( QFileDialog::AnyFile );
        fd->show();
        if ( fd->exec() == QDialog::Accepted )
        {
            time_t now;
            time(&now);
            QStringList afile = fd->selectedFiles();
            fLogfile = new ofstream(afile.at(0).toLocal8Bit().constData());
            *fLogfile << "* File opened at: " << ctime(&now);
            fFilename->setText(afile.at(0));
        }
        else
        {
            fEnableSave->setCheckState(Qt::Unchecked);
        }
    }
    else
    {
        fLogfile->close();
        delete fLogfile;
        fLogfile = 0;
        fFilename->setText(tr("NONE"));
    }
    SET_DEBUG_STACK;
}
