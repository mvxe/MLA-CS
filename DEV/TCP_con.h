#ifndef TCP_CON_H
#define TCP_CON_H

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <string.h> //for bzero
#include <atomic>

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
    const std::atomic<bool>& connected{_connected};             //thread safe var to see if connected

protected:
    std::atomic<bool> _connected{false};
    struct addrinfo *captr, hints;   //captr is nullptr if unresolved
    struct sockaddr_in servernm;
    int sock;

private:
    int rn;
    char block[BLOCK_SIZE+1];
    struct timeval timeout;
};

#endif // TCP_CON_H