#ifndef MUTEX_CONTAINERS_IMPL_H
#define MUTEX_CONTAINERS_IMPL_H

/*########## mxvar ##########*/

template <typename T>
mxvar<T>::mxvar(std::mutex *mxn, T initial, std::deque<_fovar>* vec, std::string name) : mx(mxn){
    var = initial;
    change = false;
    tfvec=nullptr;
    if(vec!=nullptr){   //initialization if additional arguments were provided
        for (int i=0;i!=vec->size();i++){
            if (name.compare((*vec)[i].strname)==0){
                std::istringstream ss((*vec)[i].strval);
                ss >> var;
                tfvec=&(*vec)[i];
                return;
            }
        }
        vec->push_back({name,""});  //if it doesnt exist
        tfvec=&vec->back();
    }
}

template <typename T>
mxvar<T>::~mxvar(){
    if (tfvec!=nullptr){
        std::ostringstream ss;
        ss<<var;
        tfvec->strval=ss.str();
    }
}

template <typename T>
bool mxvar<T>::set(T nvar){
    if(check(nvar)) return true;
    mx->lock();
    var = nvar;
    change = true;
    mx->unlock();
    return false;
}

template <typename T>
T mxvar<T>::get(){
    mx->lock();
    T nvar = var;
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
    return false;
}

template <typename T>
void mxvar<T>::err(T initial){
    std::cerr << "A container of mxvar was initialized with an invalid value ( " << initial << " ). Aborting program.\n";
    exit (EXIT_FAILURE);
}


#endif // MUTEX_CONTAINERS_IMPL_H
