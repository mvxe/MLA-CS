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
    double ackFPS;
    int format_enum;

    FQsPC FQsPCcam;

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
