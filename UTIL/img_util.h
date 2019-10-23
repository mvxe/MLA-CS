#ifndef IMG_UTIL_H
#define IMG_UTIL_H

#include <mutex>
#include <atomic>
namespace cv {
    class Mat;
}

class cvMat_safe        // provides a thread safe way to shuffle a single image between different threads
{                       // also takes care of memory management (IT DESTOROYS MATRICES SO YOU DONT HAVE TO)
public:
    ~cvMat_safe();
    cv::Mat* getMat();
    void putMat(cv::Mat* pmat);
    const std::atomic<bool>& changed{_changed};
private:
    std::mutex lockx;
    cv::Mat* mat{nullptr};
    cv::Mat* oldMat{nullptr};
    std::atomic<bool> _changed{false};

};



#endif // IMG_UTIL_H
