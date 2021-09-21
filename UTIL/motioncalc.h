#ifndef MOTIONCALC_H
#define MOTIONCALC_H
#include <stdexcept>
#include <cmath>

namespace mcutil{
static void evalAccMotion(double distance, double acceleration, double maxVelocity, double* totaltime, bool* hasConstantVelocitySegment=nullptr){
    if(totaltime==nullptr) throw std::invalid_argument("In mcutil::evalAccMotion totaltime cannot be nullptr.");
    double accMotionPeakVel=std::sqrt(distance*acceleration);
    if(accMotionPeakVel>maxVelocity){
        if(hasConstantVelocitySegment!=nullptr) *hasConstantVelocitySegment=true;
        *totaltime=2*maxVelocity/acceleration+(distance-maxVelocity*maxVelocity/acceleration)/maxVelocity;
    }else{
        if(hasConstantVelocitySegment!=nullptr) *hasConstantVelocitySegment=false;
        *totaltime=2*accMotionPeakVel/acceleration;
    }
}

}

#endif // MOTIONCALC_H
