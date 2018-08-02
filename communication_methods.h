#include <string>
#include <boost/asio.hpp>

#ifndef COMMUNICATIONMETHODS_H
#define COMMUNICATIONMETHODS_H

/* this is the class for communication with the XPS*/
#define BLOCK_SIZE	1024

class ComM_XPS{
public:
    ComM_XPS();
    ~ComM_XPS();
    void connect (std::string host, std::string port);  //connect to ip
    void disconnect();
    void write(std::string write_string);
    void read(std::string &read_string);
    void rw(std::string write_string,std::string &read_string);
    //TODO implement TCL scripts
    //TODO implement PVT support

private:
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket sock;
    boost::system::error_code ec;

    char block[BLOCK_SIZE+1];
};

#endif // COMMUNICATIONMETHODS_H
