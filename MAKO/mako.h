#ifndef MAKO_H
#define MAKO_H

#include "includes.h"
#include <VimbaCPP/Include/VimbaCPP.h>
#include "sharedvars.h"
#include "MAKO/vmbwrap.h"

const int N_FRAMES_MAKO_VMB  = 15;   //queue length of CAMOBJ<->VIMBA communication
const int N_FRAMES_MAKO_PROC = 60;   //queue length of CAMOBJ<->Processing
class MAKO;

class camobj{
public:
    friend class MAKO;
    struct _cam{
        AVT::VmbAPI::CameraPtr ptr;
        std::string ID;
    };

    camobj(MAKO *cobj, mxvar<std::string> &ID, mxva<bool> &connected);
    _cam cptr;
    void start();
    void work();                                //call this periodically
    void end();
    void con_cam(bool ch);
private:
    MAKO *cobj;
    int lost_frames_MAKO_VMB;
    std::deque<int> frames;
    _cam cam;
    mxvar<std::string> &ID;     //pointer to the ID thread safe string, for GUI
    mxva<bool> &connected;      //pointer to the connected flag thread safe string, for GUI

    AVT::VmbAPI::IFrameObserverPtr VMBo;
    AVT::VmbAPI::FramePtrVector VMBframes;        //CAMOBJ<->VIMBA frames
    int Xsize;
    int Ysize;
    int format_enum;

    std::array<cv::Mat, N_FRAMES_MAKO_PROC> imgs;       //here be image matrices
    std::queue<cv::Mat*> ptr_queue;                     //here be image pointers
    std::deque<_tqueue*> img_cqueues;                   //vector containing queues for delivering images to other threads

    int imgs_iter;
    bool ackstatus;                                     //acquisition status
};

/*########### MAKO ###########*/

class MAKO{
    friend class camobj;
public:
    MAKO();
    ~MAKO();
    void run();

private:
    void list_cams();
    AVT::VmbAPI::VimbaSystem &vsys;
    AVT::VmbAPI::CameraPtrVector cameras;

    std::vector<camobj::_cam> cams;
    camobj iuScope; //add new cameras here, and also in constructor MAKO::mako (mako.cpp) and run() (mako.cpp)
};


#include "mako_events.h"

#endif // MAKO_H
