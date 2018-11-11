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
    pCalibrateXY(double testDis);
    ~pCalibrateXY();
private:
    void run();

    FQ* framequeue;
    exec_ret ret;
    double testDis;

    const cv::Mat* mat{nullptr};
    const int MAX_FEATURES = 100;

};

#endif // CALIBRATE_XY_H


