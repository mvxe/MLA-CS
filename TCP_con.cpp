#include "TCP_con.h"
#include <iostream>

/* this is a class that can be used for TCP communication*/

TCP_con::TCP_con () : sock(io_service) {
    connected = false;
}
TCP_con::~TCP_con (){
    disconnect();
}



void TCP_con::connect (std::string host, int port){
    ec.assign( -1 ,boost::system::system_category());
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(host), port);
    sock.async_connect(endpoint, boost::bind(&TCP_con::connect_fn, this, boost::asio::placeholders::error));
    io_service.poll();
    connected = (ec)?false:true;
    std::cout << "trying to connect\n" << ec << "\n";

}
void TCP_con::disconnect(){
    sock.close(ec);
    connected = false;
    if (ec) throw ec;
}
void TCP_con::write(std::string write_string){
    try{ boost::asio::write(sock, boost::asio::buffer(write_string)); }
    catch (boost::system::error_code &err) {
        throw;
        if (err) connected = false;
    }
}
void TCP_con::read(std::string &read_string){   //TODO: this needs rewriting
    read_string.clear();
    while (sock.available()==0);    //wait for response
    while (sock.available()!=0){
        size_t size = sock.read_some(boost::asio::buffer(block,sizeof(block)));
        read_string.append(block,size);
    }
}
void TCP_con::rw(std::string write_string,std::string &read_string){
    write(write_string);
    read(read_string);
}

void TCP_con::connect_fn(const boost::system::error_code& ecx){
    ec=ecx;
}

