#ifndef XPS_H
#define XPS_H

#include "TCP_con.h"
#include "sharedvars.h"
#include <thread>


class XPS : public TCP_con{
public:
    XPS();
    ~XPS();
    void run();
};

#endif // XPS_H
