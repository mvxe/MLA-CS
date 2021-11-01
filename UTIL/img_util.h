#ifndef IMG_UTIL_H
#define IMG_UTIL_H

#include <mutex>
#include <atomic>
#include <QString>
#include "opencv2/opencv.hpp"

//CVMAT SAFE

class cvMat_safe        // provides a thread safe way to shuffle a single image between different threads
{                       // also takes care of memory management (IT DESTOROYS MATRICES SO YOU DONT HAVE TO)
public:
    ~cvMat_safe();
    cv::Mat* getMat(double* min=nullptr, double* max=nullptr);
    void putMat(cv::Mat* pmat, double min=0, double max=0);
    const std::atomic<bool>& changed{_changed};
private:
    std::mutex lockx;
    cv::Mat* mat{nullptr};
    cv::Mat* oldMat{nullptr};
    std::atomic<bool> _changed{false};
    double _min,_max;
};

//OpenCV COLORMAPS  // TODO: as you upgrade OpenCV version you might want to expand this list to support more new colormaps.

class OCV_CM{
public:
    static constexpr char labels[][50]{ "COLORMAP_AUTUMN" , "COLORMAP_BONE" , "COLORMAP_JET" , "COLORMAP_WINTER" , "COLORMAP_RAINBOW" , "COLORMAP_OCEAN" , "COLORMAP_SUMMER" , "COLORMAP_SPRING" , "COLORMAP_COOL" , "COLORMAP_HSV" , "COLORMAP_PINK" , "COLORMAP_HOT" , "COLORMAP_PARULA",
                                        "COLORMAP_MAGMA" , "COLORMAP_INFERNO" , "COLORMAP_PLASMA" , "COLORMAP_VIRIDIS" , "COLORMAP_CIVIDIS" , "COLORMAP_TWILIGHT" , "COLORMAP_TWILIGHT_SHIFTED" , "COLORMAP_TURBO" };
    static constexpr int ids[]        {cv::COLORMAP_AUTUMN,cv::COLORMAP_BONE,cv::COLORMAP_JET,cv::COLORMAP_WINTER,cv::COLORMAP_RAINBOW,cv::COLORMAP_OCEAN,cv::COLORMAP_SUMMER,cv::COLORMAP_SPRING,cv::COLORMAP_COOL,cv::COLORMAP_HSV,cv::COLORMAP_PINK,cv::COLORMAP_HOT,cv::COLORMAP_PARULA,
                                       cv::COLORMAP_MAGMA,cv::COLORMAP_INFERNO,cv::COLORMAP_PLASMA,cv::COLORMAP_VIRIDIS,cv::COLORMAP_CIVIDIS,cv::COLORMAP_TWILIGHT,cv::COLORMAP_TWILIGHT_SHIFTED,cv::COLORMAP_TURBO};
    static std::vector<QString> qslabels(){        //for some reason just passing char labels[][50] to std::vector<QString> doesnt work but {"..","..",..} does?! So we workaround with this...
        std::vector<QString> ret;
        int size=sizeof(labels)/50;
        for(int i=0;i!=size;i++) ret.push_back(OCV_CM::labels[i]);
        return ret;
    }
};

//OpenCV FONTFACES

class OCV_FF{
public:
    static constexpr char labels[][50]{ "FONT_HERSHEY_SIMPLEX" , "FONT_HERSHEY_PLAIN" , "FONT_HERSHEY_DUPLEX" , "FONT_HERSHEY_COMPLEX" , "FONT_HERSHEY_TRIPLEX" , "FONT_HERSHEY_COMPLEX_SMALL" , "FONT_HERSHEY_SCRIPT_SIMPLEX" , "FONT_HERSHEY_SCRIPT_COMPLEX" , "FONT_ITALIC" };
    static constexpr int ids[]        {cv::FONT_HERSHEY_SIMPLEX,cv::FONT_HERSHEY_PLAIN,cv::FONT_HERSHEY_DUPLEX,cv::FONT_HERSHEY_COMPLEX,cv::FONT_HERSHEY_TRIPLEX,cv::FONT_HERSHEY_COMPLEX_SMALL,cv::FONT_HERSHEY_SCRIPT_SIMPLEX,cv::FONT_HERSHEY_SCRIPT_COMPLEX,cv::FONT_ITALIC};
    static std::vector<QString> qslabels(){
        std::vector<QString> ret;
        int size=sizeof(labels)/50;
        for(int i=0;i!=size;i++) ret.push_back(OCV_FF::labels[i]);
        return ret;
    }
};


// Get nearest point to center from mask mat

class imgAux{
public:
    static void getNearestFreePointToCenter(const cv::Mat* mask, const int cenX, const int cenY, int &ptX, int &ptY, float randomZD=0);//mask - 0 are free and valid points, 255 are not free
                                                                                                       //randomZD: once the closest free point is found, having some distance Dc from the center, all free points whose distance from the center
                                                                                                       //is within [Dc:Dc+randomZD] are put into a list and a random point is selected and returned as the result
                                                                                                       //returns ptX=-1 and ptY=-1 if no free point found
private:
    struct point{
        int x,y;
        float dis;
    };
    struct pointSort{
        int width, height;
        std::vector<point> points;
    };
    static std::vector<pointSort*> lookupPtDis; //we use a lookup table to find the nearest, as computing it every time would be too expensive
                                                //for the first time a mat of a certain width x height is used, this table is generated first and is used for each subsequent query for a matrix of this size
    static std::mutex mtx;
    static bool sortF0(point& i,point& j);
    static bool findLT(point& i);
    static float target;
};

// overlay

class overlay{
public:
    overlay(){}
    void* add_overlay(cv::Mat mat, long posx, long posy);
    void rm_overlay(void* ptr);
    void drawOverlays(cv::Mat& mat, long posx, long posy);
    bool empty(){return overlayElements.empty();}
    bool check(const cv::Mat& mat, long posx, long posy);
private:
    struct overlayElement{
        long posx;        // in px
        long posy;        // in px
        cv::Mat mat;
    };
    std::list<overlayElement> overlayElements;
};

#endif // IMG_UTIL_H
