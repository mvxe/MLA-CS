#ifndef XPS_TEMPLATE_H
#define XPS_TEMPLATE_H
#include "xps.h"

template<typename... Args>
void PVTobj::add(double val, Args... vals){
    data<<val<<" ";
    _add(1, vals...);
}
template<typename... Args>
void PVTobj::_add(int n, double val, Args... vals){
    data<<val<<" ";
    _add(n+1, vals...);
}

    /*~~~ XPS ~~~*/

template <typename T>
void XPS::execCommandStr(T value){
    std::stringstream* nstrm=new std::stringstream();
    eexecCommandStr(nstrm, nullptr, value);
}
template <typename T>
void XPS::execCommandStr(xps_ret* ret ,T value){
    std::stringstream* nstrm=new std::stringstream();
    return eexecCommandStr(nstrm, ret, value);
}
template<typename T, typename... Args>
void XPS::execCommandStr(T value, Args... args){
    std::stringstream* nstrm=new std::stringstream();
    eexecCommandStr(nstrm, nullptr, value, args...);
}
template<typename T, typename... Args>
void XPS::execCommandStr(xps_ret* ret, T value, Args... args){
    std::stringstream* nstrm=new std::stringstream();
    return eexecCommandStr(nstrm, ret, value, args...);
}

template <typename T>
void XPS::execCommand(std::string command, T value){
    std::stringstream* nstrm=new std::stringstream();
    *nstrm<<command<<"(";
    eexecCommand(nstrm, nullptr, value);
}
template <typename T>
void XPS::execCommand(xps_ret* ret ,std::string command, T value){
    std::stringstream* nstrm=new std::stringstream();
    *nstrm<<command<<"(";
    return eexecCommand(nstrm, ret, value);
}
template<typename T, typename... Args>
void XPS::execCommand(std::string command, T value, Args... args){
    std::stringstream* nstrm=new std::stringstream();
    *nstrm<<command<<" (";
    eexecCommand(nstrm, nullptr, value, args...);
}
template<typename T, typename... Args>
void XPS::execCommand(xps_ret* ret, std::string command, T value, Args... args){
    std::stringstream* nstrm=new std::stringstream();
    *nstrm<<command<<"(";
    return eexecCommand(nstrm, ret, value, args...);
}

template <typename T>
void XPS::eexecCommandStr(std::stringstream* strm, xps_ret* ret, T value){
    *strm<<value;
    {std::lock_guard<std::mutex>lock(mpq);
        priority_queue.push({strm->str(),ret});}
    strm->str("");
    strm->clear();
    delete strm;
}
template<typename T, typename... Args>
void XPS::eexecCommandStr(std::stringstream* strm, xps_ret* ret, T value, Args... args){
    *strm<<value;
    return eexecCommandStr(strm, ret, args...);
}

template <typename T>
void XPS::eexecCommand(std::stringstream* strm, xps_ret* ret, T value){
    *strm<<value<<")";
    {std::lock_guard<std::mutex>lock(mpq);
        priority_queue.push({strm->str(),ret});}
    strm->str("");
    strm->clear();
    delete strm;
}
template<typename T, typename... Args>
void XPS::eexecCommand(std::stringstream* strm, xps_ret* ret, T value, Args... args){
    *strm<<value<<",";
    return eexecCommand(strm, ret, args...);
}

/*##############*/

template<typename... Args>
void XPS::MoveRelative(GroupID ID, double val, Args... vals){
    std::lock_guard<std::mutex>lock(axisCoords[ID].mx);
    _MoveRelative(0, ID, val, vals...);
}
template<typename... Args>
void XPS::MoveAbsolute(GroupID ID, double val, Args... vals){
    std::lock_guard<std::mutex>lock(axisCoords[ID].mx);
    _MoveAbsolute(0, ID, val, vals...);
}
template<typename... Args>
void XPS::_MoveRelative(int n, GroupID ID, double val, Args... vals){
    axisCoords[ID].pos[n]+=val;
    _MoveRelative(n+1, ID, vals...);
}
template<typename... Args>
void XPS::_MoveAbsolute(int n, GroupID ID, double val, Args... vals){
    axisCoords[ID].pos[n]=val;
    _MoveAbsolute(n+1, ID, vals...);
}

#endif // XPS_TEMPLATE_H
