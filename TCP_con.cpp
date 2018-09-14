#include "TCP_con.h"
#include "globals.h"

/* this is a class that can be used for TCP communication*/

TCP_con::TCP_con (){
    captr = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
}
TCP_con::~TCP_con (){
    disconnect();
}


bool TCP_con::resolve(std::string host, int port, std::string *resolved){
    if (captr!=nullptr) freeaddrinfo(captr);
    captr = nullptr;
    int result = getaddrinfo(host.c_str(), NULL, &hints, &captr);
    if (result)   //error
        return true;

    char chost[256];
    getnameinfo(captr->ai_addr, captr->ai_addrlen, chost, sizeof (chost), NULL, 0, NI_NUMERICHOST);
    if (resolved != nullptr) *resolved = chost;
    servernm.sin_addr.s_addr = inet_addr(chost);
    servernm.sin_family = hints.ai_family;
    servernm.sin_port = htons( port );
    return false;
}
void TCP_con::connect(int timeout_ms){
    timeout.tv_sec = timeout_ms/1000;
    timeout.tv_usec = (timeout_ms%1000)*1000;

    if (captr==nullptr) return;

    sock = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
    if (sock < 0){
        std::cerr << "ERROR: Cannot open socket.\n";
        go.quit();
    }
    if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout))<0){   //receive timeout fucks it up: setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout))<0 ||
        std::cerr << "ERROR: Cannot set socket timeout.\n";
        go.quit();
    }
    int err = ::connect(sock, (struct sockaddr *)&servernm , sizeof(servernm));
    if (err<0 && sock>=0) close(sock);
    _connected = (err>=0);
    //std::cerr<<"error="<<errno<<"\n";
}
void TCP_con::disconnect(){
    if (sock>=0) close(sock);
    if (captr!=nullptr) freeaddrinfo(captr);
    captr = nullptr;
    _connected = false;
}
int TCP_con::write(std::string write_string){
    rn = ::write(sock,write_string.c_str(),write_string.size());
    //std::cerr<<"Num of chars written:"<<rn<<"\n";
    return rn;
}
int TCP_con::read(std::string &read_string){   //TODO: this needs rewriting
    bzero(block,BLOCK_SIZE+1);
    rn = ::read(sock,block,BLOCK_SIZE);
    read_string=block;
    //std::cerr<<"Num of chars read:"<<rn<<"\n";
    return rn;
}
int TCP_con::rw(std::string write_string,std::string &read_string){
    if (write(write_string)!=0)
        return read(read_string);
}

