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
 *******************************************************************
 */
#ifndef  __AVERAGE_hh_
# define __AVERAGE_hh_

class Average
{

    friend ostream& operator<<(ostream& output, const Average &n); 
public:
    Average(size_t Nele);
    void   AddElement(double val);
    double GetSigma(double *avg=NULL);
    double Get(void);
    void   VReset();
    inline bool FirstFill() {return fFirstFill;};
    inline int  CurrentPointer() {return fCurrentPointer;};

private:
    void    Add(double val);

    size_t  fNEntries;
    size_t  fCurrentPointer, fRejectCount;
    bool    fFirstFill;
    double* fData;
    size_t  fSize;
};

#endif
