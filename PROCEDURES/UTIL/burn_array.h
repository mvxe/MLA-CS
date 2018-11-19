#ifndef BURN_ARRAY_H
#define BURN_ARRAY_H


#include "PROCEDURES/procedure.h"

class PVTobj;

class pBurnArray: public sproc
{
public:
    pBurnArray(double spacing, double exp_fst, double exp_lst, int gridX, int gridY);
    ~pBurnArray();
private:
    void run();

    double spacing, exp_fst, exp_lst;
    int gridX, gridY;

    PVTobj* po;
    exec_ret ret;
};


#endif // BURN_ARRAY_H
