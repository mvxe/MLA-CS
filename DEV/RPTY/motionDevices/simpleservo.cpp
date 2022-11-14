#include "simpleservo.h"
#include "../rpty.h"

rpMotionDevice_SimpleServo::rpMotionDevice_SimpleServo(){
    minPosition=0;
    maxPosition=180;
    lastPosition=90;
    restPosition=90;
    conf["minPosition"]=minPosition;
    conf["maxPosition"]=maxPosition;
    conf["lastPosition"]=lastPosition;
    conf["restPosition"]=restPosition;

    conf["servo_gpio"]=servo_gpio;
    conf["minPulseWidth"]=minPulseWidth;
    conf["maxPulseWidth"]=maxPulseWidth;
    conf["n_repeat"]=n_repeat;
    conf["pulseSpacing"]=pulseSpacing;
}
rpMotionDevice_SimpleServo::~rpMotionDevice_SimpleServo(){}

double rpMotionDevice_SimpleServo::motion(std::vector<uint32_t>& cq, double _position, double velocity, double acceleration, CTRL::motionFlags flags){
    if(flags&CTRL::MF_RELATIVE){
        if(_position==0) return 0;
        _position+=position;
    }
    else if(_position<minPosition || _position>maxPosition)  throw std::invalid_argument(util::toString("In rpMotionDevice_SimpleServo::motion for axis ",axisID,", target position is not within min/max: target position=",position,", min=",minPosition,", max=", maxPosition));
    if(_position>maxPosition) _position=maxPosition;
    else if(_position<minPosition) _position=minPosition;

    cq.push_back(CQF::GPIO_MASK(gpioN,gpioP,0x00));
    cq.push_back(CQF::GPIO_VAL (0xFF,0xFF,0x00));
    for(unsigned i=0;i!=n_repeat;i++){
        long int cycles=((_position-minPosition)/(maxPosition-minPosition)*(maxPulseWidth-minPulseWidth)+minPulseWidth)*125000000-1;
        while(cycles>125000000){
            cq.push_back(CQF::WAIT(125000000));
            cycles-=125000000;
        }
        cq.push_back(CQF::WAIT(cycles>=0?cycles:0));
        cq.push_back(CQF::GPIO_VAL (0x00,0x00,0x00));
        long int waitcy=pulseSpacing*125000000-1;
        while(waitcy>125000000){
            cq.push_back(CQF::WAIT(125000000));
            waitcy-=125000000;
        }
        cq.push_back(CQF::WAIT(waitcy>=0?waitcy:0));
    }

    position=_position;
    return 0;
}

void rpMotionDevice_SimpleServo::updatePosition(){}
int rpMotionDevice_SimpleServo::getMotionError(){return 0;}

void rpMotionDevice_SimpleServo::initMotionDevice(std::vector<uint32_t>& cq, std::vector<uint32_t> &hq, unsigned &free_flag){
    if(servo_gpio>15) throw std::invalid_argument(util::toString("In RPTY::rpMotionDevice_SimpleServo: no such gpio pin: ",servo_gpio));
    gpioN=(servo_gpio<=7)?(1<<servo_gpio):0;
    gpioP=(servo_gpio>7)?(1<<(servo_gpio-8)):0;
    cq.push_back(CQF::GPIO_MASK(gpioN,gpioP,0x00));
    cq.push_back(CQF::GPIO_DIR (0x00,0x00,0x00));
    cq.push_back(CQF::GPIO_VAL (0x00,0x00,0x00));
    position=lastPosition;
}

void rpMotionDevice_SimpleServo::referenceMotionDevice(std::vector<uint32_t>& cq){}
void rpMotionDevice_SimpleServo::deinitMotionDevice(std::vector<uint32_t>& cq){}

void rpMotionDevice_SimpleServo::holdOnTarget(std::vector<uint32_t>& cq){}
