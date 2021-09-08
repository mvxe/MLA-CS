#ifndef PINEXACTSTAGE_H
#define PINEXACTSTAGE_H

#include "../rpmotion.h"
#include "../rpbbserial.h"

class rpMotionDevice_PINEXACTStage : public rpMotionDevice{
public:
    rpMotionDevice_PINEXACTStage();
    ~rpMotionDevice_PINEXACTStage();

    void motion(std::vector<uint32_t>& cq, double position, double velocity=0, double acceleration=0, bool relativeMove=false);

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
    void _readTillCharReadyAck(unsigned num=0, char breakChar='\n');
    void _readTillChar(std::string& readStr);
    void _modesetAltNs(std::vector<uint32_t>& cq, bool alternating);
    enum _wevent{_ev_wait4rdy, _ev_wait4ont};
    void _wait4ev(_wevent ev);
    void _wait4rdy(std::vector<uint32_t> &commands, uint16_t __FLAG_SHARED);
    void _wait4ont(std::vector<uint32_t> &commands, uint16_t __FLAG_SHARED);
};

#endif // PINEXACTSTAGE_H
