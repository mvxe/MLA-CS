#ifndef CAMOBJ_H
#define CAMOBJ_H

#include <VimbaCPP/Include/VimbaCPP.h>
#include "vmbwrap.h"
#include "frame_queues.h"
#include "sharedvars.h"
#include <vector>

const int N_FRAMES_MAKO_VMB  = 15;   //queue length of CAMOBJ<->VIMBA communication
class MAKO;

class camobj{
    friend class MAKO;
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

    mxvar<std::string> &ID;     //pointer to the ID thread safe string, for GUI
    mxva<bool> &connected;      //pointer to the connected flag thread safe string, for GUI

private:
    camobj(MAKO *cobj, mxvar<std::string> &ID, mxva<bool> &connected);
    _cam cptr;

    void start();
    void work();                                //call this periodically
    void end();
    void con_cam(bool ch);

    MAKO *cobj;
    int lost_frames_MAKO_VMB;
    std::deque<int> frames;
    _cam cam;

    AVT::VmbAPI::IFrameObserverPtr VMBo;
    AVT::VmbAPI::FramePtrVector VMBframes;        //CAMOBJ<->VIMBA frames
    int Xsize;
    int Ysize;
    double ackFPS;
    int format_enum;

    FQsPC FQsPCcam;
    bool ackstatus;                                     //acquisition status

    std::mutex mtx;
};

/*########### template functions ###########*/

template <typename T>
VmbErrorType camobj::set(char* atr, T nvar){
    mtx.lock();
    VmbErrorType ret=wfun::set<T>(cam.ptr, atr, nvar);
    mtx.unlock();
    return ret;
}
template <typename T>
T camobj::get(char* atr){
    mtx.lock();
    T ret=wfun::get<T>(cam.ptr, atr);
    mtx.unlock();
    return ret;
}
inline VmbErrorType camobj::run(char* atr){
    mtx.lock();
    VmbErrorType ret=wfun::run(cam.ptr, atr);
    mtx.unlock();
    return ret;
}

#endif // CAMOBJ_H
