#ifndef _CONFIG_MAKO_H
#define _CONFIG_MAKO_H
#include "sharedvars.h"
class mako_config{
public:
    mako_config():  iuScope((MAKO*)this,"iuScope")
                    {

    }
    camobj iuScope;     //add new cameras here, and also in constructor MAKO::mako (mako.cpp) and run() (mako.cpp)
};

#endif // _CONFIG_MAKO_H
