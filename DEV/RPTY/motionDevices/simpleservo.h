#ifndef SIMPLESERVO_H
#define SIMPLESERVO_H

#include "../rpmotion.h"

class rpMotionDevice_SimpleServo : public rpMotionDevice{
public:
    rpMotionDevice_SimpleServo();
    ~rpMotionDevice_SimpleServo();

    void motion(QCS& cq, movEv mEv);
    void motion(std::vector<uint32_t>& cq, movEv mEv);
    void motion(movEv mEv);

    void getCurrentPosition(double& position);
    void getMotionError(int& error);

    void initMotionDevice();
    void deinitMotionDevice();

    const std::string type{"md_SimpleServo"};
};

#endif // SIMPLESERVO_H
