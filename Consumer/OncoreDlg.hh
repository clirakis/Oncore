/**
 ******************************************************************
 *
 * Module Name : OncoreDlg.hh
 *
 * Author/Date : C.B. Lirakis / 12-Sep-10
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
#ifndef __ONCOREDLG_h_
#define __ONCOREDLG_h_
# include <QDialog>
# include <iostream>
# include <fstream>
class QMenu;
class QWidget;
class QTabWidget;
class QSpinBox;
class QLabel;
class QCheckBox;
class DataThread;
class Position;

class OncoreDlg : public QDialog
{
    Q_OBJECT
public:
    OncoreDlg( QWidget *parent=0);
    virtual ~OncoreDlg();
    inline int  Verbose() const {return fVerbose;};
    void Verbose(int v);

public slots:
    void DataReady(void* );

private slots:
    void  exit(); 

protected:       
    // event handlers

private:
    QTabWidget*    fTab;
    QMenu          *fmenu, *fpmenu;
    int            fVerbose;
    DataThread*    fData;
    Position*      fPosition;

    std::ofstream* fLogFile;
};


#endif
