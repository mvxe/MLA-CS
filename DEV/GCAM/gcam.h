#ifndef GCAM_H
#define GCAM_H

#include "DEV/GCAM/camobj.h"
#include "DEV/GCAM/arvwrap.h"
#include "DEV/GCAM/frame_queues.h"
#include <vector>

class GCAM : public gcam_config, public protooth{
    friend class camobj;
public:
    GCAM();
    ~GCAM();

    std::atomic<bool> MVM_list{true};          //TODO these two vars should not be exposed to the user, just FrameObserver::CameraListChanged
private:
    void run();
    void list_cams();
};

#endif // GCAM_H

