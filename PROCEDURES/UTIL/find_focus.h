#ifndef FIND_FOCUS_H
#define FIND_FOCUS_H

#include "PROCEDURES/procedure.h"
class FQ;
class PVTobj;

class PFindFocus: public procedure{
public:
    PFindFocus();
private:
    bool init();
    void cleanup();
    bool work();

    FQ* framequeue;
    PVTobj* po;
    double len;
};

#endif // FIND_FOCUS_H
