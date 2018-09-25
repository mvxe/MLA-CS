#ifndef CAMOBJ_H
#define CAMOBJ_H

#include <VimbaCPP/Include/VimbaCPP.h>
#include "DEV/MAKO/vmbwrap.h"
#include "DEV/MAKO/frame_queues.h"
#include <vector>
#include "_config.h"

const int N_FRAMES_MAKO_VMB  = 15;   //queue length of CAMOBJ<->VIMBA communication
class MAKO;

class camobj: public camobj_config{
    friend class MAKO;
    friend class mako_config;
    friend class FrameObserver;
public:

    struct _cam{
        AVT::VmbAPI::CameraPtr ptr;
        std::string ID;
    };

    template <typename T>
    VmbErrorType set(char* atr, T nvar);
    template <typename T>
    T get(char* atr);
    VmbErrorType run(char* atr);

    std::mutex mkmx;
    tsvar_save<std::string> ID;                             //thread safe access to camera ID
    const std::atomic<bool>& connected{_connected};         //thread safe access to camera status

    FQsPC FQsPCcam;                                         //use this to get frame queue from camera, see frame_queues.h for commands
private:
    std::atomic<bool> _connected{false};
    camobj(MAKO *cobj, std::string strID);
    _cam cptr;

    void start();
    void work();                                //call this periodically
    void end();     //mutexed
    void _end();    //not mutexed
    void con_cam(bool ch);

    void newFrame();
    bool requeueFrame(cv::Mat* MatPtr);
    void linkFrameToMat(AVT::VmbAPI::FramePtr FramePtr, cv::Mat* MatPtr);

    MAKO *cobj;
    int lost_frames_MAKO_VMB{0};
    std::deque<int> frames;
    _cam cam;

    struct FMP{
        AVT::VmbAPI::FramePtr FramePtr;
        cv::Mat* MatPtr;
    };
    AVT::VmbAPI::IFrameObserverPtr VMBo;
    std::vector<FMP> VMBframes;        //CAMOBJ<->VIMBA frames
    VmbInt64_t nPLS;
    int Xsize;
    int Ysize;
    double ackFPS;
    int format_enum;

    bool ackstatus{false};                                     //acquisition status

    std::mutex mtx;
};

/*########### template functions ###########*/

template <typename T>
VmbErrorType camobj::set(char* atr, T nvar){
    std::lock_guard<std::mutex>lock(mtx);
    return wfun::set<T>(cam.ptr, atr, nvar);
}
template <typename T>
T camobj::get(char* atr){
    std::lock_guard<std::mutex>lock(mtx);
    return wfun::get<T>(cam.ptr, atr);
}
inline VmbErrorType camobj::run(char* atr){
    std::lock_guard<std::mutex>lock(mtx);
    return wfun::run(cam.ptr, atr);
}

#endif // CAMOBJ_H
