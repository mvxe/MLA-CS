#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <atomic>
#include <mutex>

struct mesLockProg{
public:
    std::mutex _lock_meas;
    std::mutex _lock_comp;
    std::atomic<int> progress_meas{0};      //should be within [0:100]
    std::atomic<int> progress_comp{0};
};

#endif // MEASUREMENT_H
