#ifndef SIMPLESERVO_H
#define SIMPLESERVO_H

#include "../rpmotion.h"
#include "../fpga_const.h"

class rpMotionDevice_SimpleServo : public rpMotionDevice{
public:
    rpMotionDevice_SimpleServo();
    ~rpMotionDevice_SimpleServo();

    double motion(std::vector<uint32_t>& cq, double position, double velocity=0, double acceleration=0, CTRL::motionFlags flags=0);

    void updatePosition();
    int getMotionError();

    void initMotionDevice(std::vector<uint32_t>& cq, std::vector<uint32_t>& hq, unsigned& free_flag);
    void referenceMotionDevice(std::vector<uint32_t>& cq);
    void deinitMotionDevice(std::vector<uint32_t>& cq);

    void holdOnTarget(std::vector<uint32_t>& cq);

    const std::string type{"md_SimpleServo"};

    int8_t servo_gpio{0};           // N (0-7), P (8-15)
    uint8_t gpioN, gpioP;
    double minPulseWidth{0.5e-3};   // in seconds
    double maxPulseWidth{2.5e-3};
    unsigned n_repeat{10};          // repeat each pulse this many times
    double pulseSpacing{1e-3};      // delay between pulse repeats
    double iposition;
};

#endif // SIMPLESERVO_H
