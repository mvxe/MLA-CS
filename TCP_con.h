#ifndef TCP_CON_H
#define TCP_CON_H

#include "includes.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

/* this is a class that can be used for TCP communication*/
#define BLOCK_SIZE	1024

class TCP_con{
public:
    TCP_con();
    ~TCP_con();
    bool resolve(std::string host, int port, std::string *resolved = nullptr);     //returns 0 on success, *resolved is optional and returns the resolved ip string
    void connect(int timeout_ms);
    void disconnect();
    int write(std::string write_string);                        //returns number of characters written
    int read(std::string &read_string);                         //returns number of characters read
    int rw(std::string write_string,std::string &read_string);  //returns number of characters read
    //TODO implement TCL scripts
    //TODO implement PVT support
    const bool& connected;

protected:
    bool _connected;
    struct addrinfo *captr, hints;   //captr is nullptr if unresolved
    struct sockaddr_in servernm;
    int sock;

private:
    int rn;
    char block[BLOCK_SIZE+1];
    struct timeval timeout;
};

#endif // TCP_CON_H
