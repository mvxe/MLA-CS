#ifndef CONTAINERSTEMPLATE_H
#define CONTAINERSTEMPLATE_H
#include "containers.h"         //include not needed, but qtcreator recognizes external types and colors them if It's there
#include <iomanip>

/*########## cc_save ##########*/

template <typename T> cc_save<T>::cc_save(T& var, T initial, _savelst* vec, std::string name): val(var){
    val=initial;
    if(vec!=nullptr){
        for (int i=0;i!=vec->size();i++){
            if (name.compare((*vec)[i].strname)==0){
                std::istringstream ss((*vec)[i].strval);
                ss >> val;
                tfvec=&(*vec)[i];
                return;
            }
        }
        vec->push_back({name,""});  //if it doesnt exist
        tfvec=&vec->back();
    }
}
template <typename T> cc_save<T>::~cc_save(){
    if (tfvec!=nullptr){
        std::ostringstream ss;
        if (std::is_floating_point<T>::value) ss<<std::fixed<<std::setprecision(6)<<val;
        else ss<<val;
        tfvec->strval=ss.str();
    }
}


/*########## tsvar ##########*/

template <typename T>
tsvar<T>::tsvar(std::mutex *mxn, T initial): mx(mxn), var(initial){}
template <typename T>
bool tsvar<T>::set(T nvar){
    if(check(nvar)) return true;
    std::lock_guard<std::mutex>lock(*mx);
    var = nvar;
    change = true;
    return false;
}
template <typename T>
T tsvar<T>::get(bool silent){
    std::lock_guard<std::mutex>lock(*mx);
    if (!silent) change = false;
    return var;
}
template <typename T>
bool tsvar<T>::changed(){
    std::lock_guard<std::mutex>lock(*mx);
    return change;
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


#endif // CONTAINERSTEMPLATE_H
