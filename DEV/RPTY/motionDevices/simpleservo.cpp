#include "simpleservo.h"

rpMotionDevice_SimpleServo::rpMotionDevice_SimpleServo(){}
rpMotionDevice_SimpleServo::~rpMotionDevice_SimpleServo(){}

void rpMotionDevice_SimpleServo::motion(std::vector<uint32_t>& cq, double position, double velocity, double acceleration, bool relativeMove, bool blocking){}

void rpMotionDevice_SimpleServo::getCurrentPosition(double& position){}
void rpMotionDevice_SimpleServo::getMotionError(int& error){}

void rpMotionDevice_SimpleServo::initMotionDevice(std::vector<uint32_t>& cq, std::vector<uint32_t> &hq, unsigned &free_flag){}
void rpMotionDevice_SimpleServo::referenceMotionDevice(std::vector<uint32_t>& cq){}
void rpMotionDevice_SimpleServo::deinitMotionDevice(std::vector<uint32_t>& cq){}
