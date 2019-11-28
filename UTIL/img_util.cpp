#include "img_util.h"
#include "opencv2/opencv.hpp"
#include <random>
#include <iterator>

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

//OpenCV FONTFACES

const char OCV_FF::labels[][50];
const int  OCV_FF::ids[];

//imgAux

std::vector<imgAux::pointSort*> imgAux::lookupPtDis;
std::mutex imgAux::mtx;
float imgAux::target;
bool imgAux::sortF0(point& i,point& j){return (i.dis<j.dis);}
bool imgAux::findLT(point& i){return (i.dis>=target);}
void imgAux::getNearestFreePointToCenter(const cv::Mat* mask, int &ptX, int &ptY, float randomZD){
    std::lock_guard<std::mutex>lock(mtx);
    int width=mask->cols;
    int height=mask->rows;
    pointSort* ptSort{nullptr};
    for(auto& ptSortT : lookupPtDis){
        if(ptSortT->width==width && ptSortT->height==height){
            ptSort=ptSortT;
            break;
        }
    }
    if(ptSort==nullptr){    //first time for this resolution, gotta generate the lookup table
        ptSort=new pointSort;
        lookupPtDis.push_back(ptSort);
        ptSort->width=width;
        ptSort->height=height;
        ptSort->points.reserve(width*height);
        for(int i=0;i!=width;i++) for(int j=0;j!=height;j++)
            ptSort->points.push_back({i,j,sqrtf(powf(i-width/2.f,2)+powf(j-height/2.f,2))});
        std::sort(ptSort->points.begin(), ptSort->points.end(), sortF0);    //now theyre sorted by distance from shortest to longest
    }
    target=randomZD;
    point* firstPt{nullptr}; int iter=0;
    for(auto& oPoint : ptSort->points){
        if(mask->at<uchar>(oPoint.y,oPoint.x)==0){
            firstPt=&oPoint;
            target+=firstPt->dis;
            break;
        }
        iter++;
    }

    if(firstPt==nullptr){
        ptX=-1; ptY=-1;
        return;
    }
    ptX=firstPt->x; ptY=firstPt->y;
    if(randomZD!=0){
        std::vector<point>::iterator it=std::find_if(ptSort->points.begin()+iter, ptSort->points.end(), findLT);
        if(it==ptSort->points.end()) return;   //was only that one

        std::vector<point*> pts;
        while(it!=ptSort->points.begin()+iter){
            if(mask->at<uchar>(it->y,it->x)==0) pts.push_back(&(*it));
            it--;
        }

        std::cerr<<"size is "<<pts.size()<<" , Ofs is "<<iter<<"\n";
        std::mt19937 rnd(std::random_device{}());
        std::uniform_int_distribution<int>dist(0,pts.size()-1);   //NOTE: for some reason this code segfaults after a while, debugger shows its stuck in some kind of loop, GCC version: 4.19.67-2+deb10u1 , according to the internet
        int itt=dist(rnd);
        ptX=pts[itt]->x; ptY=pts[itt]->y;
    }
}
