#ifndef RPMOTION_H
#define RPMOTION_H

#include "_config.h"

using namespace nRPTY;

class rpMotionDevice {
public:
    rpMotionDevice();
    virtual ~rpMotionDevice(){};

    virtual void motion(QCS& cq, movEv mEv)=0;
    virtual void motion(std::vector<uint32_t>& cq, movEv mEv)=0;
    virtual void motion(movEv mEv)=0;
    double getMotionSetting(mst setting);

    virtual void getCurrentPosition(double& position)=0;
    virtual void getMotionError(int& error)=0;

    virtual void initMotionDevice()=0;
    virtual void deinitMotionDevice()=0;

    rtoml::vsr conf;
    const std::string type{"md_none"};

    double minPosition;
    double maxPosition;
    double homePosition;
    double defaultVelocity;
    double maximumVelocity;
    double defaultAcceleration;
    double maximumAcceleration;

    RPTY* parent;
};

#endif // RPMOTION_H
