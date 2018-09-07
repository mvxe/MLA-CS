#ifndef CONTAINERSTEMPLATE_H
#define CONTAINERSTEMPLATE_H
#include <sstream>

/*     FILE SAVE CONTAINER - CC_SAVE        */

template <typename T>
cc_save<T>::cc_save(T& var, T initial, _savelst* vec, std::string name): val(var){
    val = initial;
    tfvec=nullptr;
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
template <typename T>
cc_save<T>::~cc_save(){
    if (tfvec!=nullptr){
        std::ostringstream ss;
        ss<<val;
        tfvec->strval=ss.str();
    }
}

#endif // CONTAINERSTEMPLATE_H
