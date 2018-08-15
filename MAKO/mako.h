#ifndef MAKO_H
#define MAKO_H

#include "includes.h"
#include <VimbaCPP/Include/VimbaCPP.h>
#include "sharedvars.h"



class MAKO{
public:
    MAKO();
    ~MAKO();
    void run();

private:
    struct _cams{
        AVT::VmbAPI::CameraPtr ptr;
        std::string ID;
    };
    void refresh_cams();
    AVT::VmbAPI::VimbaSystem &vsys;
    AVT::VmbAPI::CameraPtrVector cameras;

    std::vector<_cams> cams;
};

#endif // MAKO_H
