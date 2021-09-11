#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <string>
#include <UTIL/.rtoml/rtoml.hpp>

template<typename commandQueue> class controller {
public:
    virtual ~controller(){};

    rtoml::vsr conf;
    typedef commandQueue cqueue;

        // ## higher level functions: ##

    virtual void executeQueue(cqueue& cq, uint8_t queue)=0;
    virtual void motion(cqueue& cq, std::string axisID, double position, double velocity=0, double acceleration=0, bool relativeMove=false, bool blocking=true)=0;
    virtual void motion(std::string axisID, double position, double velocity=0, double acceleration=0, bool relativeMove=false, bool blocking=true)=0;

    enum mst{mst_position, mst_minPosition, mst_maxPosition, mst_lastPosition, mst_restPosition, mst_defaultVelocity, mst_maximumVelocity, mst_defaultAcceleration, mst_maximumAcceleration};
    virtual double getMotionSetting(std::string axisID, mst setting)=0;
    virtual int getMotionError(std::string axisID)=0;


        // ## aditional higher level functions: ##

    // OPERATING PROCEDURE:
    //      1. add devices
    //      2. change settings via either GUI or TOML
    //      3. init devices
    //      4. use
    //      5. uninit devices

        // ## device initialization ##          // just name them, the devices themselves are configured via the GUI/TOML configuration
    virtual void addMotionDevice(std::string axisID)=0;
    virtual void setMotionDeviceType(std::string axisID, std::string type)=0;   // useful for GUI config

    virtual void initMotionDevices()=0;                 // inits all non-inited devices
    virtual void referenceMotionDevices()=0;
    virtual void retraceMotionDevices()=0;              // moves all motion devices to the last saved position (RPTY::motion overrides last saved positions)
    virtual void deinitMotionDevices()=0;               // inits all non-inited devices
};

#endif // CONTROLLER_H
