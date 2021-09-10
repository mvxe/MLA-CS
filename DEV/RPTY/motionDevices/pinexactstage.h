#ifndef PINEXACTSTAGE_H
#define PINEXACTSTAGE_H

#include "../rpmotion.h"
#include "../rpbbserial.h"

class rpMotionDevice_PINEXACTStage : public rpMotionDevice{
public:
    rpMotionDevice_PINEXACTStage();
    ~rpMotionDevice_PINEXACTStage();

    void motion(std::vector<uint32_t>& cq, double position, double velocity=0, double acceleration=0, bool relativeMove=false, bool blocking=true);

    double getCurrentPosition(bool getTarget=false);
    int getMotionError();

    void initMotionDevice(std::vector<uint32_t>& cq, std::vector<uint32_t>& hq, unsigned& free_flag);
    void referenceMotionDevice(std::vector<uint32_t>& cq);
    void deinitMotionDevice(std::vector<uint32_t>& cq);

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
    unsigned serialFlag;
    void _readQS(unsigned num=0);   // for num=0 it loops, otherwise it will setup reading num chars
    void _readTillChar(std::string& readStr, char breakChar='\n');
    void _modesetAltNs(std::vector<uint32_t>& cq, bool alternating);
    enum _wevent{_ev_wait4rdy, _ev_wait4ont};
    unsigned rdyFlag;
    void _wait4rdy(std::vector<uint32_t> &commands);
    unsigned ontFlag;
    void _wait4ont(std::vector<uint32_t> &commands);
};

#endif // PINEXACTSTAGE_H
