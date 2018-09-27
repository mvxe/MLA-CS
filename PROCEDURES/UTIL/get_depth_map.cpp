#include "get_depth_map.h"
#include "includes.h"

pGetDepthMap::pGetDepthMap(double range, double speed, unsigned char threshold): range(range), speed(speed), threshold(threshold){
}
pGetDepthMap::~pGetDepthMap(){
}
void pGetDepthMap::run(){


    done=true;
    end=true;
}
