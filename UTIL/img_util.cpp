#include "img_util.h"
#include "opencv2/opencv.hpp"


//CVMAT SAFE

cvMat_safe::~cvMat_safe(){
    if(mat!=nullptr && mat!=oldMat) delete mat;
    if(oldMat!=nullptr) delete oldMat;
}

cv::Mat* cvMat_safe::getMat(double* min, double* max){
    std::lock_guard<std::mutex>lock(lockx);
    if(min!=nullptr) *min=_min;
    if(max!=nullptr) *max=_max;
    if(oldMat!=nullptr && oldMat!=mat)
        delete oldMat;
    oldMat=mat;
    _changed=false;
    return mat;
}

void cvMat_safe::putMat(cv::Mat* pmat, double min, double max){
    std::lock_guard<std::mutex>lock(lockx);
    _min=min; _max=max;
    if(oldMat!=mat && mat!=nullptr) delete mat;
    mat=pmat;
    _changed=true;
}

//OpenCV COLORMAPS

const char OCV_CM::labels[][50];
const int  OCV_CM::ids[];
