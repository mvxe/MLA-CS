#ifndef MAKO_H
#define MAKO_H

#include "DEV/MAKO/camobj.h"
#include "DEV/MAKO/vmbwrap.h"
#include "DEV/MAKO/frame_queues.h"
#include <vector>

class MAKO : public mako_config, public protooth{
    friend class camobj;
public:
    MAKO();
    ~MAKO();

    std::atomic<bool> MAKO_reco{true};          //also used by GUI

    std::atomic<bool> MVM_list{true};          //TODO these two vars should not be exposed to the user, just FrameObserver::CameraListChanged
    std::atomic<bool> MVM_ignore{false};
private:
    void run();
    void list_cams();
    AVT::VmbAPI::VimbaSystem &vsys{AVT::VmbAPI::VimbaSystem::GetInstance()};
    AVT::VmbAPI::CameraPtrVector cameras;
    std::vector<camobj::_cam> cams;
};

#include "mako_events.h"


#endif // MAKO_H

