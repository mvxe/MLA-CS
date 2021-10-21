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
    rtoml::vsr conf;

    std::atomic<bool> MVM_list{true};
private:
    void run();
    void list_cams();
};

#endif // GCAM_H

