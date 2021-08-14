#ifndef _CONFIG_RPTY_H
#define _CONFIG_RPTY_H
#include "globals.h"

class rpMotionDevice;
class RPBBSerial;

namespace nRPTY {
    static const unsigned commandQueueNum{4};
    class QCS{  // queue command sets
    public:
        std::vector<uint32_t> commands[commandQueueNum];
        bool empty(){
            for (auto& cq : commands) if(!cq.empty()) return false;
            return true;
        }
        void clear(){
            for (auto& cq : commands) cq.clear();
            for (auto& fl : makeLoop) fl=false;
            for (auto& fl : makeTrig) fl=false;
            for (auto& fl : emptyReq) fl=false;
        }
        bool reset[commandQueueNum]{};                      // reset command queue before sending commands (default all false)
        bool makeLoop[commandQueueNum]{};                   // set command queue to loop on exec (default all false)
        bool makeTrig[commandQueueNum]{};                   // trig command queue on exec (default all false)
        bool emptyReq[commandQueueNum]{};                   // the queue should be empty on exec (if not exec should throw an error) (default all false)
    };
    struct movEv{   // motion event
        std::string axisID;
        double displacement=0;                              // the relative motion dispacement, in mm (or radian if rotational)
        double position;                                    // the absolute motion position, in mm, (used if displacement==0)
        double velocity=0;                                  // velocity override if >0, otherwise default velocity will be used (note: overrides to not persist)
        double acceleration=0;                              // acceleration override if >0, otherwise default acceleration will be used
        bool addDelay=false;                                // if true, a WAIT will be added to the queue, with duration equal to expected motion time (if no feedback)
        bool optimize=true;                                 // used for some devices, see specific implementations
    };
    enum mst{minPosition, maxPosition, homePosition, defaultVelocity, maximumVelocity, defaultAcceleration, maximumAcceleration};
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
    unsigned serial_cq{1};                      // serial command queue (for acquisition)
    unsigned main_aq{0};                        // main acquisition queue
    unsigned serial_aq{1};                      // serial acquisition queue


    struct motionAxis{
        std::string type{"md_none"};
        rtoml::vsr conf;
        rpMotionDevice* dev{nullptr};
        bool initialized{false};
    };

    std::map<std::string, motionAxis> motionAxes;

    // ## command queue class ##



};


#endif // _CONFIG_RPTY_H
