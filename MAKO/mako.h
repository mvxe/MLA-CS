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
    AVT::VmbAPI::VimbaSystem &vsys;
};

#endif // MAKO_H
