#include "cnc.h"

template <typename... Args>
void CNC::execCommand(Args... args){
    std::lock_guard<std::mutex>lock(mpq);
    priority_queue.push({util::toString(args...),nullptr});
}
template <typename... Args>
void CNC::execCommand(exec_ret* ret, Args... args){
    std::lock_guard<std::mutex>lock(mpq);
    if(ret!=nullptr) ret->reset();
    priority_queue.push({util::toString(args...),ret});
}
