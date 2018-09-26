#ifndef FIND_FOCUS_H
#define FIND_FOCUS_H

#include "PROCEDURES/procedure.h"
class FQ;
class PVTobj;
namespace cv {
    class Mat;
}

class PFindFocus: public procedure{
public:
    PFindFocus(double min, double len, double speed, unsigned char threshold, unsigned recursived=0);
    ~PFindFocus();
private:
    bool startup();
    void cleanup();
    bool work();

    bool proc_frame();

    FQ* framequeue;
    PVTobj* po;
    exec_ret ret;
    const cv::Mat* mat{nullptr};
    cv::Mat* lastMat;
    double len;    //mm
    double speed;  //mm/s
    double& addOfs;
    unsigned char threshold;
    bool endPtFl{false};
    bool startPtFl{false};
    unsigned int lastTs{0};
    util::doTimes doTms0{10};
    util::doTimes doTms1{10};

    double minpos,posx,posy;
    unsigned totalFr{0};
    unsigned peakFr[2]{0,0};
    double maxDif[2]{0,0};

    unsigned recursived;    //the procedure will call itself recursively this many times, with an order of magnitude smaller speed and around the last focus point
};

#endif // FIND_FOCUS_H
