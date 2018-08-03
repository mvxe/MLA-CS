#include "communication_methods.h"

ComM_XPS::ComM_XPS () : sock(io_service) , timeout(io_service){}
ComM_XPS::~ComM_XPS (){
    disconnect();
}


void ComM_XPS::connect (std::string host, std::string port){
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(host, port);
//    boost::asio::connect(sock, resolver.resolve(query),ec);
//    if (ec) throw ec;
    timeout.expires_from_now(boost::posix_time::seconds(1));
    timeout.async_wait(boost::bind(&ComM_XPS::timeout_fn, this));
    ec = boost::asio::error::would_block;

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("192.168.0.254"), 5001);
    //sock.async_connect(endpoint, boost::lambda::var(ec) =  boost::lambda::_1);
    sock.async_connect(endpoint, boost::bind(&ComM_XPS::connect_fn, this, boost::asio::placeholders::error));
    io_service.run();
    if (ec) exit(0);
}
void ComM_XPS::disconnect(){
    sock.close(ec);
    if (ec) throw ec;
}
void ComM_XPS::write(std::string write_string){
    try{ boost::asio::write(sock, boost::asio::buffer(write_string)); }
    catch (boost::system::system_error err) {throw err;}
}
void ComM_XPS::read(std::string &read_string){
    read_string.clear();
    while (sock.available()==0);    //wait for response
    while (sock.available()!=0){
        size_t size = sock.read_some(boost::asio::buffer(block,sizeof(block)));
        read_string.append(block,size);
    }
}
void ComM_XPS::rw(std::string write_string,std::string &read_string){
    write(write_string);
    read(read_string);
}

void ComM_XPS::timeout_fn(){
    sock.cancel();
}
void ComM_XPS::connect_fn(const boost::system::error_code& ecx)
{
    timeout.cancel();
    ec=ecx;
}
