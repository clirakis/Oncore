/**
 ******************************************************************
 *
 * Module Name : Average.hh
 *
 * Author/Date : C.B. Lirakis / 28-June-08
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
 * $RCSfile$
 * $Author$
 * $Date$
 * $Locker$
 * $Name$
 * $Revision$
 *
 * $Log: $
 *
 *******************************************************************
 */
#ifndef __AVERAGE_hh_
#define __AVERAGE_hh_
// Root includes
# include "TArrayD.h"

class Average : public TArrayD
{
public:
    Average(int Nele);
    void   AddElement(double val);
    double GetSigma();
    double Get();
    void   VReset();
    inline bool GetFirstFill() {return FirstFill;};
    inline int  GetCurrentPointer() {return CurrentPointer;};

private:
    int    CurrentPointer, RejectCount;
    bool   FirstFill;
};

#endif
