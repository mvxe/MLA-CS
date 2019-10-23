#include "img_util.h"
#include "opencv2/opencv.hpp"

cvMat_safe::~cvMat_safe(){
    if(mat!=nullptr && mat!=oldMat) delete mat;
    if(oldMat!=nullptr) delete oldMat;
}

cv::Mat* cvMat_safe::getMat(){
    std::lock_guard<std::mutex>lock(lockx);
    if(oldMat!=nullptr && oldMat!=mat)
        delete oldMat;
    oldMat=mat;
    _changed=false;
    return mat;
}

void cvMat_safe::putMat(cv::Mat* pmat){
    std::lock_guard<std::mutex>lock(lockx);
    if(oldMat!=mat && mat!=nullptr) delete mat;
    mat=pmat;
    _changed=true;
}
