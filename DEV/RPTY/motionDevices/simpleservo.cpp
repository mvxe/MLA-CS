#include "simpleservo.h"

rpMotionDevice_SimpleServo::rpMotionDevice_SimpleServo(){}
rpMotionDevice_SimpleServo::~rpMotionDevice_SimpleServo(){}

void rpMotionDevice_SimpleServo::motion(QCS& cq, movEv mEv){}
void rpMotionDevice_SimpleServo::motion(std::vector<uint32_t>& cq, movEv mEv){}
void rpMotionDevice_SimpleServo::motion(movEv mEv){}

void rpMotionDevice_SimpleServo::getCurrentPosition(double& position){}
void rpMotionDevice_SimpleServo::getMotionError(int& error){}

void rpMotionDevice_SimpleServo::initMotionDevice(){}
void rpMotionDevice_SimpleServo::deinitMotionDevice(){}
