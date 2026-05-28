/**
 ******************************************************************
 *
 * Module Name : client.hh
 *
 * Author/Date : C.B. Lirakis / 10-Sep-10
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
 *******************************************************************
 */
#ifndef __CLIENT_h_
#define __CLIENT_h_
class Client 
{
public:
    Client(short Port);
    ~Client();
    bool GetCharacter(unsigned char *c); // get a single character from the port.
    inline int Error() {return fError;};
    inline bool Connected() {return fConnected;};

    enum Errors {NO_ERROR, OPEN_ERROR, BAD_OPTION};
private:
    int  fSockfd;
    int  fError;
    bool fConnected;
};
#endif
