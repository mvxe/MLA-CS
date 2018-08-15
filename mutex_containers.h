#ifndef MUTEX_CONTAINERS_H
#define MUTEX_CONTAINERS_H

#include "includes.h"

struct _fovar{    //for file ops
    std::string strname;
    std::string strval;
};

/*########## mxva ##########
 * provides a template container that
 * is thread safe, it blocks until it
 * can access the variable
 */

template <typename T>
class mxva{
public:
    mxva(std::mutex *mxn, T initial);  //must be constructed with a mutex pointer
    virtual bool set(T nvar);           //the usual set, returns false on success (if the provided value is valid)
    T get();                            //the usual get
    bool changed();                     //has the variable been changed since last get()
protected:
    std::mutex *mx;
    T var;
    bool change;
    bool virtual check(T nvar);         //function added to start of set() to check if the provided value is invalid (returns false if the value is valid)
    void err(T initial);                //constructors of derived classes with overriden check should also call "if (check(initial)) err(initial);"
    _fovar* tfvec;                      //for saving to file
};

/*########## mxvar ##########
 * adds automatic read from/write to file
 */

template <typename T>
class mxvar : public mxva<T>{
public:
    mxvar(std::mutex *mxn, T initial, std::deque<_fovar>* vec = nullptr, std::string name = "??");  //must be constructed with a mutex pointer
    ~mxvar();
protected:
    _fovar* tfvec;                      //for saving to file
};

/*########## mxvar_whatever ##########
here we derive a few classes that also do variable validity checks*/

class mxvar_ip : public mxvar<std::string>{     //contain ip strings in the format of xxx:xxx:xxx:xxx
public:
    mxvar_ip(std::mutex *mxn, std::string initial, std::deque<_fovar>* vec = nullptr, std::string name = "");
    bool is_name;
    mxvar<std::string> resolved;
protected:
    bool check(std::string nvar) override;
};

/*####*/

class mxvar_port : public mxvar<int>{           //contain port (0 to 65535)
public:
    mxvar_port(std::mutex *mxn, int initial, std::deque<_fovar>* vec = nullptr, std::string name = "");
protected:
    bool check(int nvar) override;
};



#include "mutex_containers_impl.h"

#endif // MUTEX_CONTAINERS_H
