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
    struct _cam{
        AVT::VmbAPI::CameraPtr ptr;
        std::string ID;
    };
    void list_cams();
    void con_cams(bool ch);
    AVT::VmbAPI::VimbaSystem &vsys;
    AVT::VmbAPI::CameraPtrVector cameras;

    std::vector<_cam> cams;
    _cam iuScope;
};


#include "mako_events.h"

#endif // MAKO_H
