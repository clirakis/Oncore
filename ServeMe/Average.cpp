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
 ********************************************************************/

#ifndef lint
static char rcsid[]="$Header$";
#endif

// System includes.
#include <iostream>
#include <string>
#include <cmath>
#include <fstream>
using namespace std;

// Local Includes.
#include "Average.hh"

extern ofstream logFile;

/**
 ******************************************************************
 *
 * Function Name : Average Constructor
 *
 * Description : Expands on root TArrayD
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
Average::Average(int Nele) : TArrayD(Nele)
{
    CurrentPointer = 0;
    FirstFill      = false;
    RejectCount    = 0;
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
    // Protect against temporary fliers.
    double sig = GetSigma();
    double avg = Get();
    double diff = fabs(val)-fabs(avg);
    if ((fabs(diff) < 3.0*sig) || (RejectCount > 5) || (!FirstFill))
    {
	AddAt(val,CurrentPointer);
	CurrentPointer = (CurrentPointer+1)%GetSize();
	if (CurrentPointer == 0)
	{
	    FirstFill = true;
	}
	RejectCount = 0;
    }
    else
    {
	RejectCount++;
#if 0
	logFile.precision(4);
	logFile << scientific
		<< "Val: "       << val
		<< " Sigma : "   << sig
		<< " diff: "     << diff 
		<< " Reject: "   << RejectCount
		<< " Fill: "     << (int) FirstFill
		<< endl;
#endif
    }

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
double Average::GetSigma()
{
    int i;
    double sum, sum2, rc;

    rc = 0.0;
    sum = sum2 = 0.0;
    sum = GetSum();
    sum = sum * sum;

    if (GetSize() > 1)
    {
	for(i=0;i<GetSize();i++)
	{
	    sum2 += (GetAt(i) * GetAt(i));
	}
	rc = sqrt(fabs(sum - sum2))/((double) GetSize());
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
double Average::Get()
{
    double rc = 0.0;
    if (FirstFill)
    {
	rc = GetSum()/((double)GetSize());
    }
    else if(CurrentPointer > 0)
    {
	rc = GetSum()/((double)CurrentPointer);
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
    CurrentPointer = 0;
    FirstFill      = false;
    Reset();     // Fill vector with zeros.
}
