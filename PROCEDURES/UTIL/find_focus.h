#ifndef FIND_FOCUS_H
#define FIND_FOCUS_H

#include "PROCEDURES/procedure.h"
class FQ;
class PVTobj;

class PFindFocus: public procedure{
public:
    PFindFocus(double len, double speed);
private:
    bool startup();
    void cleanup();
    bool work();

    FQ* framequeue;
    PVTobj* po;
    double len;    //mm
    double speed;  //mm/s
};

#endif // FIND_FOCUS_H
