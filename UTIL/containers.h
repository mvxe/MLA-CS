#ifndef CONTAINERS_H
#define CONTAINERS_H

#include <string>
#include <deque>
#include <mutex>
#include <thread>
#include <sstream>
#include <iostream>
#include <list>

namespace _containers_internal {
    void quit();
}

/*########## exec_dat ##########
 * provides a muxed container for exec queue
 */

struct exec_dat{
    std::string retstr;
    int retval;
};
class exec_ret{
public:
    bool block_till_done();             //for client function, returns true if an error value was given
    bool check_if_done();               //for client function, returns true if command has executed and data can be accessed
    void reset();                       //for server to reset the ret, this is needed if it is reused for multiple commands
    void set_value(std::string val);    //for server thread function , these three are thread safe

    exec_dat v{"",-9999};               //once one of the first two commands indicate it is done, the client can directly access the two vars (DO NOT DESTROY OBJECT BEFORE IT RETURNS A VALUE!! unless you like segfaults, that is)
protected:
    std::mutex mx;
    bool done{false};
};



/*########## tsvar ##########
 * provides a template container that
 * is thread safe, it blocks until it
 * can access the variable.
 * each tsvar object has its own mutex
 */

template <typename T>
class tsvar{
public:
    tsvar(std::mutex *mxn, T initial);
    virtual bool set(T nvar);           //the usual set, returns false on success (if the provided value is valid)
    T get();                            //the usual get
    bool changed();                     //returns whether the variable has been changed, also resets the changed flag
    T operator = (T nvar);
    operator T();
protected:
    std::mutex* mx;
    T var;
    bool change{false};
    bool virtual check(T nvar);         //function added to start of set() to check if the provided value is invalid (returns false if the value is valid)
    void err(T initial);                //constructors of derived classes with overriden check should also call "if (check(initial)) err(initial);"
};

    /*########## tsvar_ip ##########
     * does a is an ip check
     */

    class tsvar_ip : public tsvar<std::string>{         //contain ip strings in the format of xxx:xxx:xxx:xxx
    public:
        tsvar_ip(std::mutex *mxn, std::string initial): tsvar<std::string>(mxn, initial) , resolved(mxn, " "){if (check(initial)) err(initial);}
        bool is_name;
        tsvar<std::string> resolved;
    protected:
        bool check(std::string nvar) override;
    };

    /*########## tsvar_port ##########
     * does a is an port check
     */

    class tsvar_port : public tsvar<int>{               //contain port (0 to 65535)
    public:
        tsvar_port(std::mutex *mxn, int initial): tsvar<int>(mxn, initial){if (check(initial)) err(initial);}
    protected:
        bool check(int nvar) override;
    };

/*########## tsbool ##########
bool container with expiration time*/

class tsbool{
public:
    tsbool(std::mutex *mxn);
    void set(bool nvar, double expt=0);      //after exp time this returns to false
    bool get();
protected:
    double exp_time;
    time_t mf;
    std::mutex *mx;
    bool var{false};
};



/*########## VARIABLE FOR SAFE DISTRIBUTING SERVER -> MULTIPLE CLIENTS ##########
 provides a thread safe way to shuffle a variable or object between different threads*/

template <typename T> class varShare;
template <typename T> class varShareClient{     //after getting the client, you need to destroy it yourself, also all varShareClients should be destroyed before varShare, otherwise you will get an segfault
public:
    varShareClient(varShare<T>* parent):parent(parent){}
    ~varShareClient();
    const T* get();     //After getting the pointer, changed flag will be turned off, and the next get call may have a new variable. The old pointer is valid until a new one is returned, at which point the old data might be deleted (depending whether other clients still use it).
    bool changed();     //returns true if new data is available
private:
    typename varShare<T>::varSt* var{nullptr};
    varShare<T>* parent;
};
template <typename T> class varShare{   // also takes care of memory management
public:
    ~varShare();
    void put(T* var);                   //for the server, the object takes ownership now, and will delete the var when done with it
    varShareClient<T>* getClient();     //for the client
protected:
    void cleanup();
    friend class varShareClient<T>;
    struct varSt{
        T* var;
        uint num{0};
    };
    std::mutex active_lock;
    std::list<varSt*> active;
    std::atomic<varSt*> current{nullptr};    //just to reduce locking, used by varShareClient to return changed
};


#include "containers_template.h"

#endif // CONTAINERS_H
