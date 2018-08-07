#ifndef MUTEX_CONTAINERS_H
#define MUTEX_CONTAINERS_H

#include <mutex>
#include <string>


/*########## mxvar ##########
provides a template container that
is thread safe*/

template <typename T>
class mxvar{
public:
    explicit mxvar(std::mutex *mxn);    //must be constructed with a mutex pointer
    void set(T nvar);                   //the usual set
    T get();                            //the usual get
    bool changed();                     //has the variable been changed since last get()
private:
    std::mutex *mx;
    T var;
    bool change;
protected:
    bool virtual check(T nvar);         //function added to start of set() to check the validity of the variable; should be overriden in derived classes
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
