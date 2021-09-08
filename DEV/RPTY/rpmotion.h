#ifndef RPMOTION_H
#define RPMOTION_H

#include "_config.h"

using namespace nRPTY;

class rpMotionDevice {
public:
    rpMotionDevice();
    virtual ~rpMotionDevice(){};

    virtual void motion(std::vector<uint32_t>& cq, double position, double velocity=0, double acceleration=0, bool relativeMove=false)=0;
        // cq - queue to put the commands into
        // position - the absolute (or relative if relativeMove==true) motion position, in mm (or radian if rotational)
        // velocity - velocity override if >0, otherwise default velocity will be used (note: overrides to not persist)
        // acceleration - acceleration override if >0, otherwise default acceleration will be used
    double getMotionSetting(mst setting);

    virtual void getCurrentPosition(double& position)=0;
    virtual void getMotionError(int& error)=0;

    virtual void initMotionDevice()=0;
    virtual void deinitMotionDevice()=0;

    rtoml::vsr conf;
    const std::string type{"md_none"};
    std::string axisID;

    double minPosition;
    double maxPosition;
    double homePosition;
    double lastPosition;
    double defaultVelocity;
    double maximumVelocity;
    double defaultAcceleration;
    double maximumAcceleration;

    RPTY* parent;
};

#endif // RPMOTION_H
