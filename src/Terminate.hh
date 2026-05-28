/**
 ******************************************************************
 *
 * Module Name : Terminate.hh
 *
 * Author/Date : C.B. Lirakis / 06-Mar-19
 *
 * Description : Access the terminate function from anywhere in
 * the module. 
 *
 * Restrictions/Limitations : none
 *
 * Change Descriptions :
 *
 * Classification : Unclassified
 *
 * References :
 *
 *
 *******************************************************************
 */
#ifndef __TERMINATE_hh_
#define __TERMINATE_hh_
/**
 * Terminate - this function is used by the module and is linked to most of
 * the signals associated with the overall module. 
 */
void Terminate (int sig);
#endif
