#ifndef TCP_CON_H
#define TCP_CON_H

#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

/* this is a class that can be used for TCP communication*/
#define BLOCK_SIZE	1024

class TCP_con{
public:
    TCP_con();
    ~TCP_con();
    void connect (std::string host, int port);  //connect to ip
    void disconnect();
    void write(std::string write_string);
    void read(std::string &read_string);
    void rw(std::string write_string,std::string &read_string);
    //TODO implement TCL scripts
    //TODO implement PVT support

protected:
    bool connected;

private:
    void connect_fn(const boost::system::error_code& ec);
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket sock;
    boost::system::error_code ec;
    char block[BLOCK_SIZE+1];
};


#endif // TCP_CON_H
