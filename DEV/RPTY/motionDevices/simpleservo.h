#ifndef SIMPLESERVO_H
#define SIMPLESERVO_H

#include "../rpmotion.h"

class rpMotionDevice_SimpleServo : public rpMotionDevice{
public:
    rpMotionDevice_SimpleServo();
    ~rpMotionDevice_SimpleServo();

    void motion(std::vector<uint32_t>& cq, double position, double velocity=0, double acceleration=0, bool relativeMove=false);

    void getCurrentPosition(double& position);
    void getMotionError(int& error);

    void initMotionDevice();
    void deinitMotionDevice();

    const std::string type{"md_SimpleServo"};
};

#endif // SIMPLESERVO_H
