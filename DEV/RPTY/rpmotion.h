#ifndef RPMOTION_H
#define RPMOTION_H

#include <UTIL/.rtoml/rtoml.hpp>
#include <mutex>
#include "DEV/controller.h" // for flags
class RPTY;

class rpMotionDevice {
public:
    rpMotionDevice();
    virtual ~rpMotionDevice(){};

    virtual void motion(std::vector<uint32_t>& cq, double position, double velocity=0, double acceleration=0, CTRL::motionFlags flags=0)=0;
        // cq - queue to put the commands into
        // position - the absolute (or relative if relativeMove==true) motion position, in mm (or radian if rotational)
        // velocity - velocity override if >0, otherwise default velocity will be used (note: overrides to not persist)
        // acceleration - acceleration override if >0, otherwise default acceleration will be used
        // blocking - if true, consequent moves will wait for this one to finish (only applicable to some devices)
        // NOTE : for relative moves, the program does not check if the move is within minPosition/maxPosition (for some devices a soft limit is set on the device firmware though, so the error will be returned via getMotionError)

    virtual void updatePosition()=0;
    virtual int getMotionError()=0;

    virtual void initMotionDevice(std::vector<uint32_t>& cq, std::vector<uint32_t>& hq, unsigned& free_flag)=0;
        // hq - helper command queue commands - these will added to the helper loop and should comunicate with the main command queue via flags (use ifs and not w4trig)
        // free_flag - the lowest unused flag, if the motion device makes use of it, it should bitshift this variable to the left (multiple flags may be used by one device)
    virtual void referenceMotionDevice(std::vector<uint32_t>& cq)=0;
        // reference the axes
    virtual void deinitMotionDevice(std::vector<uint32_t>& cq)=0;

    virtual void holdOnTarget(std::vector<uint32_t>& cq)=0;
        // block cq execution until onTarget

    rtoml::vsr conf;
    const std::string type{"md_none"};
    std::string axisID;

    double position;

    double minPosition;
    double maxPosition;
    double lastPosition;
    double restPosition;
    double defaultVelocity;
    double maximumVelocity;
    double defaultAcceleration;
    double maximumAcceleration;

    RPTY* parent;
    std::mutex mux;
};

#endif // RPMOTION_H
