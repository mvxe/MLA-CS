#ifndef _CONFIG_RPTY_H
#define _CONFIG_RPTY_H
#include "globals.h"

class rpty_config{
protected:
    rpty_config(){
        conf["RPTY_ip"]=IP;
        conf["RPTY_port"]=port;
        conf["RPTY_keepalive"]=keepalive;
    }
    std::mutex smx;
public:
    rtoml::vsr conf;                                        //configuration map

    tsvar_ip IP{&smx, "192.168.1.2"};
    tsvar_port port{&smx, 32};
    tsvar<unsigned> keepalive{&smx, 500};                   //keepalive and connect timeout, in ms

};

#endif // _CONFIG_RPTY_H
