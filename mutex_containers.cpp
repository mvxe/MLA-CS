#include "mutex_containers.h"
#include <cstdio>

/*########## mxvar ##########*/

template <typename T>
mxvar<T>::mxvar(std::mutex *mxn) : mx(mxn){
    change = false;
}

template <typename T>
void mxvar<T>::set(T nvar){
    if(!check(nvar)) return;
    mx->lock();
    var = nvar;
    change = true;
    mx->unlock();
}

template <typename T>
T mxvar<T>::get(){
    T nvar;
    mx->lock();
    nvar = var;
    change = false;
    mx->unlock();
    return nvar;
}

template <typename T>
bool mxvar<T>::changed(){
    bool tmp;
    mx->lock();
    tmp = change;
    mx->unlock();
    return tmp;
}

template <typename T>
bool mxvar<T>::check(T nvar){
    return true;
}

/*########## mxvar_whatever ##########*/

mxvar_ip::mxvar_ip(std::mutex *mxn) : mxvar<std::string>(mxn){
    set("0.0.0.0");
}

bool mxvar_ip::check(std::string nvar){
    int n = 0;
    int nums[4];
    for (int i=0;i!=nvar.size();i++){
        if (nvar[i]=='.') n++;
        else if (nvar[i]>='0' && nvar[i]<='9');
        else return false;      //character not a number or dot
    }
    if (n!=3 || sscanf(nvar.c_str(),"%d.%d.%d.%d",&nums[0],&nums[1],&nums[2],&nums[3])!=4) return false;     //must be three dots and three numbers
    for (int i=0;i!=4;i++) if (nums[i]<0 || nums[i]>255) return false;
    return true;
}

/*####*/

mxvar_port::mxvar_port(std::mutex *mxn) : mxvar<int>(mxn){
    set(0);
}

bool mxvar_port::check(int nvar){
    if (nvar<0 || nvar>65535) return false;
    return true;
}
