#include "rpmotion.h"

rpMotionDevice::rpMotionDevice(){
    conf["minPosition"]=minPosition;
    conf["maxPosition"]=maxPosition;
    conf["lastPosition"]=lastPosition;
    conf["restPosition"]=restPosition;
    conf["defaultVelocity"]=defaultVelocity;
    conf["maximumVelocity"]=maximumVelocity;
    conf["defaultAcceleration"]=defaultAcceleration;
    conf["maximumAcceleration"]=maximumAcceleration;
}
