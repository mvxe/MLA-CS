#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <atomic>
#include <mutex>

struct procLockProg{
public:
    std::mutex _lock_proc;
    std::mutex _lock_comp;
    std::atomic<int> progress_proc{0};      //should be within [0:100]
    std::atomic<int> progress_comp{0};
};

#endif // MEASUREMENT_H
