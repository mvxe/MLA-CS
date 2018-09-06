#ifndef MAKO_H
#define MAKO_H

#include <VimbaCPP/Include/VimbaCPP.h>
#include "camobj.h"
#include "vmbwrap.h"
#include "frame_queues.h"
#include "sharedvars.h"
#include <vector>
class camobj;

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

