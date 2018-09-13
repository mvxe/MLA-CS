#ifndef CONTAINERS_H
#define CONTAINERS_H

#include <string>
#include <deque>
#include <mutex>
#include <thread>
#include <sstream>
#include <iostream>

namespace _containers_internal {
    void quit();
}

/*########## cc_save ##########
 * provides a file save container (can be used as base)
 * it loads from config file on construction, and saves on destruction
 * requires a global object var_save to be defined, this must construct befor any cc_save and destruct after,
 * because it handles the actual reading/writing from file
 */

struct _fo{    //for file ops
    std::string strname;
    std::string strval;
};
typedef std::deque<_fo> _savelst;

template <typename T> class cc_save{
public:
    cc_save(T& var, T initial, _savelst* vec, std::string name);
    ~cc_save();
    T& val;
protected:
    _fo* tfvec;
};

/*########## xps_dat ##########
 * provides a muxed container for XPS queue
 */

struct xps_dat{
    std::string retstr;
    int retval;
};
class xps_ret{
public:
    xps_ret();
    bool block_till_done();             //for client function, returns true if an error value was given by the xps
    bool check_if_done();               //for client function, returns true if data can be accessed
    void set_value(std::string val);    //for server(xps) function , these three are thread safe

    xps_dat v;                 //once one of the first two commands indicate it is done, the client can directly access the two vars (DO NOT DESTROY OBJECT BEFORE IT RETURNS A VALUE!! unless you like segfaults, that is)
protected:
    std::mutex mx;
    bool done;
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
    T get(bool silent = false);         //the usual get, the optional parameter silent, if true, makes the get operation not set changed to false
    bool changed();                     //has the variable been changed since last get()
protected:
    std::mutex* mx;
    T var;
    bool change;
    bool virtual check(T nvar);         //function added to start of set() to check if the provided value is invalid (returns false if the value is valid)
    void err(T initial);                //constructors of derived classes with overriden check should also call "if (check(initial)) err(initial);"
};

    /*########## tsvar_save ##########
     * combines tsvar and cc_save
     */

    template <typename T>
    class tsvar_save: public tsvar<T>, public cc_save<T>{
    public:
        tsvar_save(std::mutex *mxn, T initial, _savelst* vec, std::string name):tsvar<T>(mxn, initial),cc_save<T>(tsvar<T>::var, initial, vec, name){}
    };

        /*########## tsvar_save_ip ##########
         * does a is an ip check
         */

        class tsvar_save_ip : public tsvar_save<std::string>{     //contain ip strings in the format of xxx:xxx:xxx:xxx
        public:
            tsvar_save_ip(std::mutex *mxn, std::string initial, _savelst*  vec, std::string name): tsvar_save<std::string>(mxn, initial, vec, name) , resolved(mxn, " "){if (check(initial)) err(initial);}
            bool is_name;
            tsvar<std::string> resolved;
        protected:
            bool check(std::string nvar) override;
        };

        /*########## tsvar_save_port ##########
         * does a is an port check
         */

        class tsvar_save_port : public tsvar_save<int>{           //contain port (0 to 65535)
        public:
            tsvar_save_port(std::mutex *mxn, int initial, _savelst*  vec, std::string name): tsvar_save<int>(mxn, initial, vec, name){if (check(initial)) err(initial);}
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
    bool var;
};


#include "containers_template.h"

#endif // CONTAINERS_H
