#include "cvplotauxobj.h"

cvp_pointrange::cvp_pointrange(unsigned height, unsigned width, std::string unit): height(height), width(width){

}

bool cvp_pointrange::eval(double val){
    return (val>=min && val<=max)?true:false;
}
double cvp_pointrange::getMin(){
    return min;
}
double cvp_pointrange::getMax(){
    return max;
}
void cvp_pointrange::set_points(std::vector<pt>& points){

}
