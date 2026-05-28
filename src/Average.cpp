/********************************************************************
 *
 * Module Name : Average.cpp
 *
 * Author/Date : C.B. Lirakis / 28-June-08
 *
 * Description : Generic module
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
#include <cmath>
#include <string.h>
using namespace std;

// Local Includes.
#include "debug.h"
#include "Average.hh"

/**
 ******************************************************************
 *
 * Function Name : Average Constructor
 *
 * Description : 
 *
 * Inputs : Number of elements to allocate for system. 
 *
 * Returns : constructed class
 *
 * Error Conditions : NONE
 * 
 * Unit Tested on: 28-June-08
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
Average::Average(size_t Nele)
{
    SET_DEBUG_STACK;
    fCurrentPointer = 0;
    fFirstFill      = false;
    fRejectCount    = 0;
    fData           = 0;
    if (Nele > 0)
    {
        fData       = new double[Nele];
    }
    fSize           = Nele;
}
/**
 ******************************************************************
 *
 * Function Name : AddElement
 *
 * Description : Add value, but not without first doing some
 * flier rejection. It has to be within 3 sigma or rejected more
 * than 5 times, or within the FirstFill (First NElements)
 *
 * Inputs : value to put in array
 *
 * Returns : NONE
 *
 * Error Conditions : NONE
 * 
 * Unit Tested on:  28-June-08
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void Average::AddElement( double val)
{
    SET_DEBUG_STACK;
    if (fFirstFill)
    {
        // Protect against temporary fliers.
        double sig  = GetSigma();
        double avg  = Get();
        double diff = fabs(val)-fabs(avg);
        if ((fabs(diff) < 3.0*sig) || (fRejectCount > 5))
        {
            Add(val);
            fRejectCount = 0;
        }
        else
        {
            fRejectCount++;
        }
    }
    else
    {
        Add(val);
    }
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : Add
 *
 * Description : Add raw element into array
 *
 * Inputs : value to put in array
 *
 * Returns : NONE
 *
 * Error Conditions : NONE
 * 
 * Unit Tested on:  06-Aug-10
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void Average::Add( double val)
{
    SET_DEBUG_STACK;
    fData[fCurrentPointer] = val;
    //
    // roll the pointer as necessary, kind of a windowed low pass filter
    // without weight. 
    //
    fCurrentPointer        = (fCurrentPointer+1)%fSize;
    fNEntries++;
    if (fCurrentPointer == 0)
    {
        fFirstFill = true;
    }
    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name : GetSigma
 *
 * Description : return the sigma of the array.
 *
 * Inputs : NONE
 *
 * Returns : Sigma
 *
 * Error Conditions : NONE
 * 
 * Unit Tested on: 28-June-08
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
double Average::GetSigma(double *avg)
{
    size_t i;
    double sum, sum2, rc, x;

    rc  = 0.0;
    sum = sum2 = 0.0;

    if (fFirstFill)
    {
        for(i=0;i<fSize;i++)
        {
            x     = fData[i];
            sum  += x;
            sum2 += x*x;
        }
        if (avg != NULL)
        {
            *avg = sum/((double) fSize);
        }
        rc = sqrt(fabs(pow(sum,2.0) - sum2))/((double) fSize);
    }
    else
    {
        if (avg != NULL)
        {
            *avg = 9999.99;
        }
    }
    return rc;
}
/**
 ******************************************************************
 *
 * Function Name : Get
 *
 * Description : Return average of array
 *               Conditions: 
 *               If number of entries > size full array is used.
 *               Else only entries are used
 *               If no entries have been made return 0
 *
 * Inputs : NONE
 *
 * Returns : Avergage
 *
 * Error Conditions : NONE
 * 
 * Unit Tested on: 28-June-08
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
double Average::Get(void)
{
    double rc = 9999.99;
    if (fFirstFill)
    {
        double sum = 0;
        for (size_t i=0;i<fSize;i++)
        {
            sum += fData[i];
        }
        rc = sum/((double)fSize);
    }
    return rc;
}

/**
 ******************************************************************
 *
 * Function Name :VReset
 *
 * Description : Reset the current system.
 *
 * Inputs : NONE
 *
 * Returns : NONE
 *
 * Error Conditions : NONE
 * 
 * Unit Tested on: 28-June-08
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void Average::VReset()
{
    fCurrentPointer = 0;
    fFirstFill      = false;
    memset( fData, 0, fSize*sizeof(double));
}
ostream& operator<<(ostream& output, const Average &n)
{
    output << "Average Data "
           << " Current Pointer: " << n.fCurrentPointer
           << " Reject Count: "    << n.fRejectCount
           << " First Fill: "      << n.fFirstFill
           << " Total Entries: "   << n.fNEntries
           << endl;
    return output;
}
