#ifndef _CONFIG_RPTY_H
#define _CONFIG_RPTY_H
#include "globals.h"

class rpty_config{
protected:
    std::mutex smx;
public:
    tsvar_save_ip IP{&smx, "192.168.1.2", &go.config.save, "RPTY_ip"};
    tsvar_save_port port{&smx, 32, &go.config.save, "RPTY_port"};
    tsvar_save<unsigned> keepalive{&smx, 500, &go.config.save, "RPTY_keepalive"};     //keepalive and connect timeout, in ms

};

#endif // _CONFIG_RPTY_H
