#ifndef PINEXACTSTAGE_H
#define PINEXACTSTAGE_H

#include "../rpmotion.h"
#include "../rpbbserial.h"

class rpMotionDevice_PINEXACTStage : public rpMotionDevice{
public:
    rpMotionDevice_PINEXACTStage();
    ~rpMotionDevice_PINEXACTStage();

    void motion(QCS& cq, movEv mEv);
    void motion(std::vector<uint32_t>& cq, movEv mEv);
    void motion(movEv mEv);

    void getCurrentPosition(double& position);
    void getMotionError(int& error);

    void initMotionDevice();
    void deinitMotionDevice();

    const std::string type{"md_PINEXACTStage"};

    enum motionType{mt_nanostepping_delay, mt_nanostepping_ontarget, mt_alternating_delay, mt_alternating_ontarget};
    motionType mType{mt_nanostepping_delay};
    bool alternating{false};        // current mode: alternating or nanostepping

    RPBBSerial serial;
    int8_t serial_gpio_o{0};        // N (0-7), P (8-15)
    int8_t serial_gpio_i{1};        // N (0-7), P (8-15)
    double last_velocity=-1;
    double last_acceleration=-1;
    int settleWindow{6};
    double settleTime{0.001};

private:
    void home(std::vector<uint32_t>& commands);
    void _readTillChar(std::string& readStr);
    void _modesetAltNs(std::vector<uint32_t>& cq, bool alternating);
};

#endif // PINEXACTSTAGE_H
