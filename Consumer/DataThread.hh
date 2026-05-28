/**
 ******************************************************************
 *
 * Module Name : DataThread.hh
 *
 * Author/Date : C.B. Lirakis / 15-Sep-10
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
*******************************************************************
 */
#ifndef __DATATHREAD_h_
#  define __DATATHREAD_h_
#  include <iostream>
#  include <fstream>
#  include <QThread>
#  include <QMutex>
#  include <QWaitCondition>

class Oncore;
class Client;

class DataThread : public QThread
{
    Q_OBJECT
public:

    DataThread( QObject *parent=0, std::ofstream *l=0);
    ~DataThread();
    inline int  Verbose() const {return fVerbose;};
    void Verbose(int v);

public slots:
    void TimeReady();

signals:
    void DataReady(void *);       

protected:       
    // event handlers
    void run();

private:
    QMutex          fMutex;
    QWaitCondition  fCondition;
    bool            fAbort;
    Oncore*         fOncore;
    Client*         fClient;
    std::ofstream*  fLogFile;
    int             fVerbose;
};
#endif
