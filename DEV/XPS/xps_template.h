#ifndef XPS_TEMPLATE_H
#define XPS_TEMPLATE_H
#include "xps.h"

template<typename... Args>
void PVTobj::add(double val, Args... vals){
    pvtqueue.push(val);
    _add(1, vals...);
}
template<typename... Args>
void PVTobj::_add(int n, double val, Args... vals){
    pvtqueue.push(val);
    _add(n+1, vals...);
}
template<typename... Args>
void PVTobj::addAction(Args... vals){
    cmdQueue.push(util::toCmdString("EventExtendedConfigurationTriggerSet",util::toString(go.pXPS->groupGetName(XPS::mgroup_XYZ),".PVT.ElementNumberStart"),pvtqueue.size()/(1+2*go.pXPS->groups[ID].AxisNum)+1,0,0,0));
    cmdQueue.push(util::toCmdString("EventExtendedConfigurationActionSet",vals...));
    cmdQueue.push("EventExtendedStart (int *)");
    cmdWasLast=true;
}

/*########## XPS ##########*/

template <typename... Args>
void XPS::execCommandStr(Args... args){
    std::lock_guard<std::mutex>lock(mpq);
    priority_queue.push({util::toString(args...),nullptr});
}
template <typename... Args>
void XPS::execCommandStr(exec_ret* ret, Args... args){
    std::lock_guard<std::mutex>lock(mpq);
    if(ret!=nullptr) ret->reset();
    priority_queue.push({util::toString(args...),ret});
}

template <typename... Args>
void XPS::execCommand(Args... args){
    std::lock_guard<std::mutex>lock(mpq);
    priority_queue.push({util::toCmdString(args...),nullptr});
}
template <typename... Args>
void XPS::execCommand(exec_ret* ret, Args... args){
    std::lock_guard<std::mutex>lock(mpq);
    if(ret!=nullptr) ret->reset();
    priority_queue.push({util::toCmdString(args...),ret});
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
