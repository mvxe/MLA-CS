#ifndef _CONFIG_RPTY_H
#define _CONFIG_RPTY_H
#include "globals.h"

class rpty_config{
protected:
    rpty_config(){
        conf["RPTY_ip"]=IP;
        conf["RPTY_port"]=port;
        conf["RPTY_keepalive"]=keepalive;

        conf["main_command_queue"]=main_cq;
        conf["helper_command_queue"]=helper_cq;
        conf["main_acquisition_queue"]=main_aq;
        conf["serial_acquisition_queue"]=serial_aq;
    }
    std::mutex smx;
public:
    rtoml::vsr conf;                                        //configuration map

    tsvar_ip IP{&smx, "192.168.1.2"};
    tsvar_port port{&smx, 32};
    tsvar<unsigned> keepalive{&smx, 500};                   //keepalive and connect timeout, in ms

    std::atomic<unsigned> main_cq{0};                       // main command queue
    std::atomic<unsigned> helper_cq{1};
    std::atomic<unsigned> main_aq{0};                       // main acquisition queue
    std::atomic<unsigned> serial_aq{1};                     // serial acquisition queue
};

#endif // _CONFIG_RPTY_H
