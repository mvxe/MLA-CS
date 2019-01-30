#ifndef CNC_H
#define CNC_H

#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <array>
#include <atomic>

#include "UTIL/containers.h"
#include "globals.h"

class cnc : public protooth
{
public:
    cnc();
    ~cnc();
};

#endif // CNC_H
