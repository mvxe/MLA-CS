#ifndef MAKO_H
#define MAKO_H

#include "camobj.h"
#include "vmbwrap.h"
#include "frame_queues.h"
#include "sharedvars.h"
#include <vector>

class MAKO : public mako_config{
    friend class camobj;
public:
    MAKO();
    ~MAKO();
    void run();

    std::atomic<bool> MAKO_reco{true};          //also used by GUI

    std::atomic<bool> MVM_list{true};          //TODO these two vars should not be exposed to the user, just FrameObserver::CameraListChanged
    std::atomic<bool> MVM_ignore{false};
private:
    void list_cams();
    AVT::VmbAPI::VimbaSystem &vsys{AVT::VmbAPI::VimbaSystem::GetInstance()};
    AVT::VmbAPI::CameraPtrVector cameras;
    std::vector<camobj::_cam> cams;
};

#include "mako_events.h"


#endif // MAKO_H

