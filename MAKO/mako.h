#ifndef MAKO_H
#define MAKO_H

#include "sharedvars.h"
#include <thread>
#include <VimbaCPP/Include/VimbaCPP.h>


class MAKO{
public:
    MAKO();
    ~MAKO();
    void run();
private:
    AVT::VmbAPI::VimbaSystem &vsys;
};

#endif // MAKO_H
