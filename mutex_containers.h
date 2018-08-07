#ifndef MUTEX_CONTAINERS_H
#define MUTEX_CONTAINERS_H

#include <mutex>
#include <string>

/*########## mxvar ##########
provides a template container that
is thread safe, it blocks until it
can access the variable*/

template <typename T>           //We define the base class function implmentations here instead of in mutex_containers.cpp because the compiler has to have access to template functions.
class mxvar{
public:
    explicit mxvar(std::mutex *mxn) : mx(mxn){   //must be constructed with a mutex pointer
        change = false;
    }
    bool set(T nvar){                            //the usual set, returns false on success (if the provided value is valid)
        if(check(nvar)) return true;
        mx->lock();
        var = nvar;
        change = true;
        mx->unlock();
        return false;
    }
    T get(){                                     //the usual get
        mx->lock();
        const T nvar = var;
        change = false;
        mx->unlock();
        return nvar;
    }
    bool changed(){                              //has the variable been changed since last get()
        bool tmp;
        mx->lock();
        tmp = change;
        mx->unlock();
        return tmp;
    }
private:
    std::mutex *mx;
    T var;
    bool change;
protected:
    bool virtual check(T nvar){                  //function added to start of set() to check if the provided value is invalid (returns false if the value is valid)
        return false;
    }
};

/*########## mxvar_whatever ##########
here we derive a few classes that also do variable validity checks*/

class mxvar_ip : public mxvar<std::string>{     //contain ip strings in the format of xxx:xxx:xxx:xxx
public:
    explicit mxvar_ip(std::mutex *mxn);
protected:
    bool check(std::string nvar) override;
};

/*####*/

class mxvar_port : public mxvar<int>{           //contain port (0 to 65535)
public:
    explicit mxvar_port(std::mutex *mxn);
protected:
    bool check(int nvar) override;
};


#endif // MUTEX_CONTAINERS_H
