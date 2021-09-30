#include "simpleservo.h"

rpMotionDevice_SimpleServo::rpMotionDevice_SimpleServo(){
    conf["minPosition"]=minPosition;
    conf["maxPosition"]=maxPosition;
    conf["lastPosition"]=lastPosition;
    conf["restPosition"]=restPosition;
    conf["defaultVelocity"]=defaultVelocity;
    //conf["maximumVelocity"]=maximumVelocity;
    conf["defaultAcceleration"]=defaultAcceleration;
    //conf["maximumAcceleration"]=maximumAcceleration;
    //conf["minimumStep"]=minimumStep;
}
rpMotionDevice_SimpleServo::~rpMotionDevice_SimpleServo(){}

double rpMotionDevice_SimpleServo::motion(std::vector<uint32_t>& cq, double position, double velocity, double acceleration, CTRL::motionFlags flags){}

void rpMotionDevice_SimpleServo::updatePosition(){}
int rpMotionDevice_SimpleServo::getMotionError(){}

void rpMotionDevice_SimpleServo::initMotionDevice(std::vector<uint32_t>& cq, std::vector<uint32_t> &hq, unsigned &free_flag){}
void rpMotionDevice_SimpleServo::referenceMotionDevice(std::vector<uint32_t>& cq){}
void rpMotionDevice_SimpleServo::deinitMotionDevice(std::vector<uint32_t>& cq){}

void rpMotionDevice_SimpleServo::holdOnTarget(std::vector<uint32_t>& cq){}
