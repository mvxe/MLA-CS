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
    ~procedure();
    std::atomic<bool> end{false};           //this can be used by the creator thread or go.cleanup() to shut down the thread
private:
    void run();
};

#endif // PROCEDURE_H
