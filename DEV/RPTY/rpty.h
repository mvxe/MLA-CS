#ifndef RPTY_H
#define RPTY_H

#include "DEV/TCP_con.h"

#include "UTIL/containers.h"
#include "_config.h"

class RPTY : public TCP_con, public rpty_config, public protooth{
public:
    RPTY();
    ~RPTY();
private:
    void run();
};

#endif // RPTY_H
