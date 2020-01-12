#include "pipe.h"

_pipe::~_pipe(){
    fbOUT->close();
     fbIN->close();
    fbERR->close();
    delete fbOUT;
    delete _POUT;
    delete  fbIN;
    delete  _PIN;
    delete fbERR;
    delete _PERR;
}
