#ifndef XPS_H
#define XPS_H

#include "includes.h"
#include "TCP_con.h"
#include "sharedvars.h"


class XPS : public TCP_con{
public:
    XPS();
    ~XPS();
    void run();
};

#endif // XPS_H
