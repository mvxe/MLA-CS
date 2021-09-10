#ifndef _CONFIG_RPTY_H
#define _CONFIG_RPTY_H
#include "globals.h"

class rpMotionDevice;
class RPBBSerial;

namespace nRPTY {
    enum mst{minPosition, maxPosition, lastPosition, restPosition, defaultVelocity, maximumVelocity, defaultAcceleration, maximumAcceleration};
}
using namespace nRPTY;

class rpty_config{
protected:
    rpty_config(){
        conf["RPTY_ip"]=IP;
        conf["RPTY_port"]=port;
        conf["RPTY_keepalive"]=keepalive;

        conf["main_command_queue"]=main_cq;
        conf["helper_command_queue"]=helper_cq;
        conf["serial_command_queue"]=serial_cq;
        conf["main_acquisition_queue"]=main_aq;
        conf["serial_acquisition_queue"]=serial_aq;
    }
    std::mutex smx;
public:
    rtoml::vsr conf;                                        //configuration map

    tsvar_ip IP{&smx, "192.168.1.2"};
    tsvar_port port{&smx, 32};
    tsvar<unsigned> keepalive{&smx, 500};                   //keepalive and connect timeout, in ms

    unsigned main_cq{0};                        // main command queue
    unsigned helper_cq{1};
    unsigned serial_cq{2};                      // serial command queue (for acquisition)
    unsigned main_aq{0};                        // main acquisition queue
    unsigned serial_aq{1};                      // serial acquisition queue


    struct motionAxis{
        std::string type{"md_none"};
        rtoml::vsr conf;
        rpMotionDevice* dev{nullptr};
    };
    bool axesInited=false;
    std::map<std::string, motionAxis> motionAxes;

    // ## command queue class ##



};


#endif // _CONFIG_RPTY_H
