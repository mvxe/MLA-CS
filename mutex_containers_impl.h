#ifndef MUTEX_CONTAINERS_IMPL_H
#define MUTEX_CONTAINERS_IMPL_H

/*########## mxvar ##########*/

template <typename T>
mxvar<T>::mxvar(std::mutex *mxn, T initial) : mx(mxn){
    var = initial;
    change = false;
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
    const T nvar = var;
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
