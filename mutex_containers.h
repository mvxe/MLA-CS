#ifndef MUTEX_CONTAINERS_H
#define MUTEX_CONTAINERS_H

#include <mutex>
#include <string>
#include <cstdio>
#include <iostream>

/*########## mxvar ##########
provides a template container that
is thread safe, it blocks until it
can access the variable*/

template <typename T>
class mxvar{
public:
    mxvar(std::mutex *mxn, T initial);  //must be constructed with a mutex pointer
    virtual bool set(T nvar);           //the usual set, returns false on success (if the provided value is valid)
    T get();                            //the usual get
    bool changed();                     //has the variable been changed since last get()
private:
    std::mutex *mx;
    T var;
    bool change;
protected:
    bool virtual check(T nvar);         //function added to start of set() to check if the provided value is invalid (returns false if the value is valid)
    void err(T initial);                //constructors of derived classes with overriden check should also call "if (check(initial)) err(initial);"
};

/*########## mxvar_whatever ##########
here we derive a few classes that also do variable validity checks*/

class mxvar_ip : public mxvar<std::string>{     //contain ip strings in the format of xxx:xxx:xxx:xxx
public:
    mxvar_ip(std::mutex *mxn, std::string initial);
    bool is_name;
    mxvar<std::string> resolved;
protected:
    bool check(std::string nvar) override;
};

/*####*/

class mxvar_port : public mxvar<int>{           //contain port (0 to 65535)
public:
    mxvar_port(std::mutex *mxn, int initial);
protected:
    bool check(int nvar) override;
};


#include "mutex_containers_impl.h"

#endif // MUTEX_CONTAINERS_H
