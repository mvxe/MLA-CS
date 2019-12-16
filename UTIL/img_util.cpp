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
void imgAux::getNearestFreePointToCenter(const cv::Mat* mask, const int cenX, const int cenY, int &ptX, int &ptY, float randomZD){
    std::lock_guard<std::mutex>lock(mtx);
    int width=mask->cols;
    int height=mask->rows;
    pointSort* ptSort{nullptr};
    for(auto& ptSortT : lookupPtDis){
        if(ptSortT->width==2*width && ptSortT->height==2*height){
            ptSort=ptSortT;
            break;
        }
    }
    if(ptSort==nullptr){    //first time for this resolution, gotta generate the lookup table
        ptSort=new pointSort;
        lookupPtDis.push_back(ptSort);
        ptSort->width=2*width;              //we make it twice as wide/high
        ptSort->height=2*height;
        ptSort->points.reserve(4*width*height);
        for(int i=0;i!=2*width;i++) for(int j=0;j!=2*height;j++)
            ptSort->points.push_back({i,j,sqrtf(powf(i-width,2)+powf(j-height,2))});        //fist element will be at the center of the image, ie. at [width,height]
        std::sort(ptSort->points.begin(), ptSort->points.end(), sortF0);    //now theyre sorted by distance from shortest to longest
    }
    target=randomZD;
    point* firstPt{nullptr}; int iter=0;
    for(auto& oPoint : ptSort->points){
        if((oPoint.x-width/2+cenX)<0 || (oPoint.x-width/2+cenX)>=width || (oPoint.y-height/2+cenY)<0 || (oPoint.y-height/2+cenY)>=height);
        else if(mask->at<uchar>(oPoint.y-height/2+cenY,oPoint.x-width/2+cenX)==0){
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
    ptX=firstPt->x-width/2; ptY=firstPt->y-height/2;
    if(randomZD!=0){
        std::vector<point>::iterator it=std::find_if(ptSort->points.begin()+iter, ptSort->points.end(), findLT);
        if(it==ptSort->points.end()) return;   //was only that one

        std::vector<point*> pts;
        while(it!=ptSort->points.begin()+iter){
            if((it->x-width/2+cenX)<0 || (it->x-width/2+cenX)>=width || (it->y-height/2+cenY)<0 || (it->y-height/2+cenY)>=height);
            else if(mask->at<uchar>(it->y-height/2+cenY,it->x-width/2+cenX)==0) pts.push_back(&(*it));
            it--;
        }
        std::mt19937 rnd(std::random_device{}());
        std::uniform_int_distribution<int>dist(0,pts.size()-1);   //NOTE: for some reason this code segfaults after a while, debugger shows its stuck in some kind of loop, GCC version: 4.19.67-2+deb10u1
        int itt=dist(rnd);
        ptX=pts[itt]->x-width/2; ptY=pts[itt]->y-height/2;
    }
}
