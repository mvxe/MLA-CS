#ifndef MAKO_H
#define MAKO_H

#include "camobj.h"
#include "vmbwrap.h"
#include "frame_queues.h"
#include "sharedvars.h"
#include <vector>
#include "_config.h"
class camobj;

class MAKO : public mako_config{
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
};

#include "mako_events.h"


#endif // MAKO_H

