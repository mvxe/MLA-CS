#ifndef GET_DEPTH_MAP_H
#define GET_DEPTH_MAP_H

#include "PROCEDURES/procedure.h"
class FQ;
class PVTobj;
namespace cv {
    class Mat;
}

class pGetDepthMap: public sproc
{
public:
    pGetDepthMap(double range, double speed, unsigned char threshold);
    ~pGetDepthMap();
private:
    void run();

    FQ* framequeue;
    PVTobj* po;
    exec_ret ret;
    double range, speed, threshold;
    double addOfs;
};

#endif // GET_DEPTH_MAP_H
