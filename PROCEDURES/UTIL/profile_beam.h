#ifndef PROFILE_BEAM_H
#define PROFILE_BEAM_H

#include "PROCEDURES/procedure.h"
#include <vector>
class FQ;
namespace cv {
    class Mat;
}

class pProfileBeam: public sproc
{
public:
    pProfileBeam(double minpos, double maxpos, double step, double speed, double trigdelay, std::vector<cv::Mat>* mats, std::mutex* matlk);
    ~pProfileBeam();
private:
    void run();
    double minpos;
    double maxpos;
    double step;
    double speed;
    double trigdelay;
    std::mutex* matlk;
    std::vector<cv::Mat>* mats;
    FQ* framequeue;
    const cv::Mat* mat;
};

#endif // PROFILE_BEAM_H
