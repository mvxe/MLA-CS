#ifndef CALIBRATE_XY_H
#define CALIBRATE_XY_H

#include "PROCEDURES/procedure.h"
class FQ;
class PVTobj;
namespace cv {
    class Mat;
}

class pCalibrateXY: public sproc
{
public:
    pCalibrateXY(double testDis, double *xps_x_sen, double *xps_y_sen);   //TODO remove these pointers
    ~pCalibrateXY();
private:
    void run();

    FQ* framequeue;
    exec_ret ret;
    double testDis;

    const cv::Mat* mat{nullptr};
    const int MAX_FEATURES = 1000;
    const int minDis=20;
    double* xps_x_sen;
    double* xps_y_sen;
};

#endif // CALIBRATE_XY_H


