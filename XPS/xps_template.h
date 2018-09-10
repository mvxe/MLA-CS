#ifndef XPS_TEMPLATE_H
#define XPS_TEMPLATE_H
#include "xps.h"

template <typename T>
void XPS::execCommand(T value){
    std::stringstream* nstrm=new std::stringstream();
    eexecCommand(nstrm, nullptr, value);
}
template <typename T>
void XPS::execCommand(xps_ret* ret ,T value){
    std::stringstream* nstrm=new std::stringstream();
    return eexecCommand(nstrm, ret, value);
}
template<typename T, typename... Args>
void XPS::execCommand(T value, Args... args){
    std::stringstream* nstrm=new std::stringstream();
    eexecCommand(nstrm, nullptr, value, args...);
}
template<typename T, typename... Args>
void XPS::execCommand(xps_ret* ret, T value, Args... args){
    std::stringstream* nstrm=new std::stringstream();
    return eexecCommand(nstrm, ret, value, args...);
}

template <typename T>
void XPS::eexecCommand(std::stringstream* strm, xps_ret* ret, T value){
    *strm<<value;
    {std::lock_guard<std::mutex>lock(mpq);
        priority_queue.push({strm->str(),ret});}
    strm->str("");
    strm->clear();
    delete strm;
}
template<typename T, typename... Args>
void XPS::eexecCommand(std::stringstream* strm, xps_ret* ret, T value, Args... args){
    *strm<<value;
    return eexecCommand(strm, ret, args...);
}

#endif // XPS_TEMPLATE_H
