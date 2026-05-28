 /*
  ******************************************************************
  *
  * Module Name : OncoreDlg.cpp
  *
  * Author/Date : C.B. Lirakis / 12-Sep-10
  *
  * Description : Display status of Oncore GPS by connecting via TCP/IP
  * to the server
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
#include <csignal>
#include <stdio.h>
#include <time.h>
#include <fstream>
#include <unistd.h>
#include <stdlib.h>

// Qt includes, 
#include <QApplication>
#include <QDialog>
#include <QWidget>
#include <QTabWidget>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>


/// Local Includes.
#include "debug.h"
#include "OncoreDlg.hh"
#include "Position.hh"
#include "DataThread.hh"
#include "Oncore.hh"

const  short  PortNumber  = 65001;
const  double Version     = 1.0;
static int    Verbose     =   0;
/**
 ******************************************************************
 *
 * Function Name : Terminate
 *
 * Description : Deal with errors in a clean way!
 *               ALL, and I mean ALL exits are brought 
 *               through here!
 *
 * Inputs : Signal causing termination. 
 *
 * Returns : none
 *
 * Error Conditions : Well, we got an error to get here. 
 *
 *******************************************************************
 */ 
static void Terminate (int sig) 
{
    static int i=0;
    time_t now;

    time(&now);
    // prevent this from getting into a infinite loop of 
    // deaths. 
    i++;
    if (i>1)
    { 
        _exit(-1);
    }
    cout << "Program ends: " << ctime(&now);
    // Don't tell the user something is wrong when we have a signal of 0.
    if (sig != 0)
    {
        cout << "Death occured at: " << LastFile << " " << LastLine << endl;
    }

    // Add the apprpriate message. 
    switch (sig)
    {
    case -1: 
      cout << "User abnormal termination" << endl;
      break;
    case 0:                    // Normal termination
        cout << "Normal program termination." << endl;
        break;
    case SIGHUP:
        cout << " Hangup" << endl;
        break;
    case SIGINT:               // CTRL+C signal 

        cout << " SIGINT " << endl;
        break;
    case SIGQUIT:               //QUIT 
        cout << " SIGQUIT " << endl;
        break;
    case SIGILL:               // Illegal instruction 
        cout << " SIGILL " << endl;
        break;
    case SIGABRT:              // Abnormal termination 
        cout << " SIGABRT " << endl;
        break;
    case SIGBUS:               //Bus Error! 
        cout << " SIGBUS " << endl;
        break;
    case SIGFPE:               // Floating-point error 
        cout << " SIGFPE " << endl;
        break;
    case SIGKILL:               // Kill!!!! 
        cout << " SIGKILL" << endl;
        break;
    case SIGSEGV:              // Illegal storage access 
        cout << " SIGSEGV " << endl;
        break;
    case SIGTERM:              // Termination request 
        cout << " SIGTERM " << endl;
        break;
    case SIGSTKFLT:               // Stack fault
        cout << " SIGSTKFLT " << endl;
        break;
    case SIGTSTP:               // 
        cout << " SIGTSTP" << endl;
        break;
    case SIGXCPU:               // 
        cout << " SIGXCPU" << endl;
        break;
    case SIGXFSZ:               // 
        cout << " SIGXFSZ" << endl;
        break;
    case SIGSTOP:               // 
        cout << " SIGSTOP " << endl;
        break;
    case SIGPWR:               // 
        cout << " SIGPWR " << endl;
        break;
    case SIGSYS:               // 
        cout << " SIGSYS " << endl;
        break;
    default:
        cout << " Uknown signal type: "  << sig <<  endl;
        break;
    }

    // Put any user termination routines here. 
    //

    if (sig == 0)
    {
        _exit (0);
    }
    else
    {
        _exit (-1);
    }
}

/**
 ******************************************************************
 *
 * Function Name : OncoreDlg Constructor
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
OncoreDlg::OncoreDlg( QWidget *parent) : QDialog( parent)
{
    SET_DEBUG_STACK;
    time_t now;
    time(&now);
    // see http://doc.qt.nokia.com/4.0/dialogs-tabdialog.html
    // to see how to make a tab dialog.
    fTab        = new QTabWidget;
    SET_DEBUG_STACK;
    fPosition = new Position();
    fTab->addTab( fPosition,  tr("Position"));
    SET_DEBUG_STACK;

    QPushButton *okButton = new QPushButton(tr("OK"));
    connect(okButton,     SIGNAL(clicked()), this, SLOT(accept()));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(fTab);
    mainLayout->addWidget(okButton);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
    setWindowTitle(tr("Oncore Status"));
    SET_DEBUG_STACK;

    // Initlize connection with oncore process.
    fLogFile = new ofstream("consumer.log");
    *fLogFile << "# Version " << Version << " Program Begins: " << ctime(&now);
//    *fLogFile << "# verbose: " << fVerbose << endl;

    SET_DEBUG_STACK;
    fData = new DataThread(this, fLogFile);
    fData->start();
    // source then target. 
    QObject::connect(fData, SIGNAL(DataReady(void *)), this, 
                     SLOT(DataReady(void *)));
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
void OncoreDlg::exit()
{
    SET_DEBUG_STACK;
    printf("Exit\n");
    Terminate(0);
}
/**
 ******************************************************************
 *
 * Function Name : OncoreDlg Verbosity
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
void OncoreDlg::Verbose(int i)
{
    fVerbose = i;
}
/**
 ******************************************************************
 *
 * Function Name : OncoreDlg Data Ready
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
void OncoreDlg::DataReady(void *ptr)
{
    Oncore *f = (Oncore* ) ptr;
    cout << "Data Ready." << endl;
    fPosition->Update(f);
    f->TimeProcessed();
    //cout << *f << endl;
}

/**
 ******************************************************************
 *
 * Function Name : OncoreDlg
 *
 * Description : Close logging file if open and detached from shared memory.
 *               also detach from bc620 driver. 
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : none
 * 
 * Unit Tested on: 26-Feb-06
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
OncoreDlg::~OncoreDlg()
{
    SET_DEBUG_STACK;
    cout << "OncoreDlg destructor closed." << endl;
    fData->exit(0);
    delete fData;
    if (fLogFile)
    {
        fLogFile->close();
    }
}
/**
 ******************************************************************
 *
 * Function Name : Initialize
 *
 * Description : Initialze the process
 *               - Setup traceback utility
 *               - Connect all signals to route through the terminate 
 *                 method
 *               - Perform any user initialization
 *
 * Inputs : none
 *
 * Returns : true on success. 
 *
 * Error Conditions : depends mostly on user code
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
static bool Initialize(void)
{

    LastFile = (char *) __FILE__;
    LastLine = __LINE__;
    time_t now;

    time(&now);
    cout << "Program Starts: " << ctime(&now);

    signal (SIGHUP , Terminate);   // Hangup.
    signal (SIGINT , Terminate);   // CTRL+C signal 
    signal (SIGKILL, Terminate);   // 
    signal (SIGQUIT, Terminate);   // 
    signal (SIGILL , Terminate);   // Illegal instruction 
    signal (SIGABRT, Terminate);   // Abnormal termination 
    signal (SIGIOT , Terminate);   // 
    signal (SIGBUS , Terminate);   // 
    signal (SIGFPE , Terminate);   // 
    signal (SIGSEGV, Terminate);   // Illegal storage access 
    signal (SIGTERM, Terminate);   // Termination request 
    signal (SIGSTKFLT, Terminate); // 
    signal (SIGSTOP, Terminate);   // 
    signal (SIGPWR, Terminate);    // 
    signal (SIGSYS, Terminate);    // 
    // User initialization goes here. 


    return true;
}
/**
 ******************************************************************
 *
 * Function Name : Help
 *
 * Description : provides user with help if needed.
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : none
 *
 *******************************************************************
 */
static void Help(void)
{
    SET_DEBUG_STACK;
    cout << "********************************************" << endl;
    cout << "* Test file for text Logging.              *" << endl;
    cout << "* Built on "<< __DATE__ << " " << __TIME__ << "*" << endl;
    cout << "* Available options are :                  *" << endl;
    cout << "*  v n : set verbose level to n.           *" << endl;
    cout << "********************************************" << endl;
}
/**
 ******************************************************************
 *
 * Function Name :  ProcessCommandLineArgs
 *
 * Description : Loop over all command line arguments
 *               and parse them into useful data.
 *
 * Inputs : command line arguments. 
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
static void
ProcessCommandLineArgs(int argc, char **argv)
{
    int option;
    SET_DEBUG_STACK;
    do
    {
        option = getopt( argc, argv, "Hhv:V:");
        switch(option)
        {
        case 'h':
        case 'H':
            Help();
        Terminate(0);
        break;
        case 'v':
        case 'V':
            Verbose = atoi(optarg);
            break;
        }
    } while(option != -1);
}

/**
 ******************************************************************
 *
 * Function Name : main
 *
 * Description : It all starts here:
 *               - Process any command line arguments
 *               - Do any necessary initialization as a result of that
 *               - Do the operations
 *               - Terminate and cleanup
 *
 * Inputs : command line arguments
 *
 * Returns : exit code
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
int main(int argc, char **argv)
{
    QApplication a( argc, argv );

    ProcessCommandLineArgs(argc, argv);

    if (Initialize())
    {
        OncoreDlg w;
        w.Verbose(Verbose);
        w.show();

        return a.exec();
    }
    Terminate(0);
}
