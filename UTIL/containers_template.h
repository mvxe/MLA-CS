#ifndef CONTAINERSTEMPLATE_H
#define CONTAINERSTEMPLATE_H
#include "containers.h"         //include not needed, but qtcreator recognizes external types and colors them if It's there
#include <iomanip>

/*########## tsvar ##########*/

template <typename T>
tsvar<T>::tsvar(std::mutex *mxn, T initial): mx(mxn), var(initial){}
template <typename T>
bool tsvar<T>::set(T nvar){
    if(check(nvar)) return true;
    std::lock_guard<std::mutex>lock(*mx);
    if(var!=nvar) change = true;
    var = nvar;
    return false;
}
template <typename T>
T tsvar<T>::get(){
    std::lock_guard<std::mutex>lock(*mx);
    change=false;   // TODO remove, left for backward compat
    return var;
}
template <typename T>
bool tsvar<T>::changed(){
    std::lock_guard<std::mutex>lock(*mx);
    if(change){
        change=false;
        return true;
    } else return false;
}
template <typename T>
bool tsvar<T>::check(T nvar){
    return false;
}
template <typename T>
void tsvar<T>::err(T initial){
    std::cerr << "ERROR: A container of tsvar was initialized with an invalid value ( " << initial << " ).\n";
    _containers_internal::quit();
}
template <typename T>
T tsvar<T>::operator = (T nvar){
    std::lock_guard<std::mutex>lock(*mx);
    if(var!=nvar) change = true;
    var = nvar;
    return var;
}
template <typename T>
tsvar<T>::operator T(){
    std::lock_guard<std::mutex>lock(*mx);

    return var;
}


/*########## VARIABLE FOR SAFE DISTRIBUTING SERVER -> MULTIPLE CLIENTS ##########*/


template <typename T> varShareClient<T>::~varShareClient(){
    std::lock_guard<std::mutex>lock(parent->active_lock);
    if(var!=nullptr) var->num--;
}
template <typename T> const T* varShareClient<T>::get(){
    std::lock_guard<std::mutex>lock(parent->active_lock);
    if(var!=nullptr) var->num--;
    if(parent->active.empty()) return nullptr;
    var=parent->active.front();
    var->num++;
    return var->var;
}
template <typename T> bool varShareClient<T>::changed(){
    return (parent->current!=nullptr && var!=parent->current);
}
template <typename T> void varShare<T>::put(T* var){
    std::lock_guard<std::mutex>lock(active_lock);
    active.push_front(new varSt{var,0});
    current=active.front();
    cleanup();
}
template <typename T> void varShare<T>::cleanup(){
    if(active.empty()) return;
    typename std::list<varSt*>::iterator it=active.begin(); it++;
    while(it!=active.end()){
        if((*it)->num==0){
            typename std::list<varSt*>::iterator itt=it;
            it++;
            delete (*itt)->var;
            delete *itt;
            active.erase(itt);
        } else it++;
    }
}
template <typename T>  varShare<T>::~varShare(){
    while(!active.empty()) {
        delete active.back()->var;
        delete active.back();
        active.pop_back();
    }
}
template <typename T>  varShareClient<T>* varShare<T>::getClient(){
    return new varShareClient<T>(this);
}

#endif // CONTAINERSTEMPLATE_H
