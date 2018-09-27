#ifndef GET_DEPTH_MAP_H
#define GET_DEPTH_MAP_H

#include "PROCEDURES/procedure.h"

class pGetDepthMap: public sproc
{
public:
    pGetDepthMap(double range, double speed, unsigned char threshold);
    ~pGetDepthMap();
private:
    void run();

    double range, speed, threshold;
};

#endif // GET_DEPTH_MAP_H
