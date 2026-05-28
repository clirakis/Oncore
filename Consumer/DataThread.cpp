 /*
  ******************************************************************
  *
  * Module Name : DataThread.cpp
  *
  * Author/Date : C.B. Lirakis / 15-Sep-10
  *
  * Description : Connects to Oncore server and decodes data. 
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

// System includes.
#include <iostream>
using namespace std;
#include <string>
#include <cmath>
#include <fstream>
#include <unistd.h>
#include <stdlib.h>

// Qt includes, 

/// Local Includes.
#include "debug.h"
#include "DataThread.hh"
#include "Oncore.hh"
#include "client.hh"

const  short   PortNumber = 65001;


/**
 ******************************************************************
 *
 * Function Name : DataThread Constructor
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
DataThread::DataThread( QObject *parent,std::ofstream *l) : QThread(parent)
{
    SET_DEBUG_STACK;

    fAbort   = true;
    fLogFile = l;
    fClient  = new Client(PortNumber);
    if (fClient->Error() == Client::NO_ERROR)
    {
        *fLogFile << "# Client established connection with server." << endl;
        fOncore = new Oncore();
        fOncore->Logstream(fLogFile);
        fOncore->Errstream(fLogFile);
        fAbort = false;
    }
    else
    {
        *fLogFile << "# Did not connect to server." << endl;
        fOncore = 0;
    }
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : DataThread
 *
 * Description :
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : none
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
DataThread::~DataThread()
{
    SET_DEBUG_STACK;
    fMutex.lock();
    cout << "DataThread destructor closed." << endl;
    if (fOncore)
    {
        delete fOncore;
    }
    fAbort = true;
    fCondition.wakeOne();
    fMutex.unlock();
    wait();
    delete fClient;
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : DataThread
 *
 * Description :
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : none
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void DataThread::run()
{
    static struct timespec sleeptime = {0L, 50000000};
    unsigned char c[4];
    SET_DEBUG_STACK;

    while(1)
    {
        SET_DEBUG_STACK;
        if (fClient->Connected())
        {
            fMutex.lock();
            if (fClient->GetCharacter(c))
            {
                fOncore->Parse(c[0]);
            }
            if (fOncore->TimeValid())
            {
                emit DataReady(fOncore);
            }
            fMutex.unlock();
        }
        if (fAbort)
        {
            cout << "Abort" << endl;
            SET_DEBUG_STACK;
             return;
        }
        nanosleep( &sleeptime, NULL);
        SET_DEBUG_STACK;
     }
    cout << "Exit called." << endl;
    SET_DEBUG_STACK;
}
 /**
 ******************************************************************
 *
 * Function Name : DataThread
 *
 * Description :
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : none
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void DataThread::Verbose(int v)
{
    SET_DEBUG_STACK;
    fVerbose = v;
    fOncore->SetVerbose(fVerbose);
}
/**
 ******************************************************************
 *
 * Function Name : DataThread:: Time Ready
 *
 * Description :
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : none
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void DataThread::TimeReady()
{
    cout << "Time thread data ready." << endl;
}
