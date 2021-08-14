#include "rpmotion.h"


double rpMotionDevice::getMotionSetting(mst setting){
    switch (setting){
        case mst::minPosition: return minPosition;
        case mst::maxPosition: return maxPosition;
        case mst::homePosition: return homePosition;
        case mst::defaultVelocity: return defaultVelocity;
        case mst::maximumVelocity: return maximumVelocity;
        case mst::defaultAcceleration: return defaultAcceleration;
        case mst::maximumAcceleration: return maximumAcceleration;
    }
}

rpMotionDevice::rpMotionDevice(){
    conf["minPosition"]=minPosition;
    conf["maxPosition"]=maxPosition;
    conf["homePosition"]=homePosition;
    conf["defaultVelocity"]=defaultVelocity;
    conf["maximumVelocity"]=maximumVelocity;
    conf["defaultAcceleration"]=defaultAcceleration;
    conf["maximumAcceleration"]=maximumAcceleration;
}
