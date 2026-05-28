/**
 ******************************************************************
 *
 * Module Name : client.cpp
 *
 * Author/Date : C.B. Lirakis / 09-May-09
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
/* System includes. */
#include <iostream>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>          /* hostent struct, gethostbyname() */
#include <arpa/inet.h>      /* inet_ntoa() to format IP address */
#include <netinet/in.h>     /* in_addr structure */
#include <string.h>         /* bzero */
#include <errno.h>          /* error processing. */
#include <unistd.h>
#include <sys/wait.h>       /* waitpid */
#include <time.h>

/** Local Includes. */
#include "debug.h"
#include "client.hh"

/** Global Variables. */

#define MAXLINE   256

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
bool Client::GetCharacter(unsigned char *c)
{

    static int	read_cnt = 0;
    //static char	*read_ptr;
    static char	read_buf[MAXLINE];
    if (!fConnected)
        return false;
#if 0
    if (read_cnt <= 0) 
    {
    again:
        if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) 
        {
            if (errno == EINTR)
            {
                goto again;
            }
            return(-1);
        } 
        else if (read_cnt == 0)
        {
            return(0);
        }
        read_ptr = read_buf;
    }

    read_cnt--;
    *ptr = *read_ptr++;
    return(1);
#else
again:
    read_cnt = read(fSockfd, read_buf, 1);
    if ( read_cnt < 0) 
    {
        if (errno == EINTR)
        {
            goto again;
        }
        return false;
    } 
    else if (read_cnt == 0)
    {
        return false;
    }
    *c = read_buf[0];
    return true;
#endif
}

Client::~Client()
{
    close(fSockfd);
}
Client::Client(short Port)
{
    int                 option;
    struct sockaddr_in	servaddr;

    fConnected = false;
    fSockfd    = socket(AF_INET, SOCK_STREAM, 0);
    if (fSockfd >=0)
    {
        /* setsockopt returns 0 for success, -1 for failure */
        if (setsockopt(fSockfd,SOL_SOCKET,SO_REUSEADDR,
                       (char *) &option,sizeof(option)) < 0)
        {
            cerr << "Error setting socket option." << endl;
            fError = BAD_OPTION;
            return;
        }

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port   = htons(Port);
        inet_pton(AF_INET, "localhost", &servaddr.sin_addr);
	
        connect(fSockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
        fError = NO_ERROR;
        fConnected = true;
    }
    else
    {
        fError = OPEN_ERROR;
    }
}
