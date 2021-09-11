#ifndef SIMPLESERVO_H
#define SIMPLESERVO_H

#include "../rpmotion.h"

class rpMotionDevice_SimpleServo : public rpMotionDevice{
public:
    rpMotionDevice_SimpleServo();
    ~rpMotionDevice_SimpleServo();

    void motion(std::vector<uint32_t>& cq, double position, double velocity=0, double acceleration=0, bool relativeMove=false, bool blocking=true);

    void updatePosition();
    int getMotionError();

    void initMotionDevice(std::vector<uint32_t>& cq, std::vector<uint32_t>& hq, unsigned& free_flag);
    void referenceMotionDevice(std::vector<uint32_t>& cq);
    void deinitMotionDevice(std::vector<uint32_t>& cq);

    const std::string type{"md_SimpleServo"};
};

#endif // SIMPLESERVO_H
