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
    PFindFocus(double len, double speed, unsigned char threshold);
private:
    bool startup();
    void cleanup();
    bool work();

    bool proc_frame();

    FQ* framequeue;
    PVTobj* po;
    exec_ret ret;
    const cv::Mat* mat;
    double len;    //mm
    double speed;  //mm/s
    unsigned char threshold;
    bool endPtFl{false};
    bool startPtFl{false};
    unsigned int lastTs{0};
};

#endif // FIND_FOCUS_H
