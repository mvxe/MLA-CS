/* This is the procedure template class. Use it as a base for new procedures.
 */
#include <atomic>
#include "globals.h"

#ifndef PROCEDURE_H
#define PROCEDURE_H


class procedure: public protooth
{
public:
    procedure();
    virtual ~procedure();
    std::atomic<bool> end{false};           //this can be used by the creator thread or go.cleanup() to shut down the thread
private:
    virtual void run();                     //this function gets called by the new thread
};

#endif // PROCEDURE_H
