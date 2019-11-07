#ifndef IMG_UTIL_H
#define IMG_UTIL_H

#include <mutex>
#include <atomic>
#include <QString>
#include "opencv2/opencv.hpp"
namespace cv {
    class Mat;
}

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

//MATOP

class matOp{
public:
    template <typename T>
    static void spread(cv::Mat* mat);        //spreads the nonzero values around by 1 pixel (for mask spreading)
};

template <typename T>
void matOp::spread(cv::Mat* mat){
    if(mat==nullptr) return;
    cv::Mat temp=mat->clone();
    T val;
    for(int i=0;i!=temp.cols;i++) for(int j=0;j!=temp.rows;j++){
        val=temp.at<T>(j,i);
        if(val!=0){
            if(i!=0){               mat->at<T>(  j,i-1)=val;
                if(j!=0)            mat->at<T>(j-1,i-1)=val;
                if(j!=temp.rows)    mat->at<T>(j+1,i-1)=val;
            }
            if(i!=temp.cols)        mat->at<T>(  j,i+1)=val;
            if(j!=0){               mat->at<T>(j-1,i  )=val;
                if(i!=0)            mat->at<T>(j-1,i-1)=val;
                if(i!=temp.cols)    mat->at<T>(j-1,i+1)=val;
            }
            if(j!=temp.rows)        mat->at<T>(j+1,i  )=val;
        }
    }
}

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

#endif // IMG_UTIL_H
