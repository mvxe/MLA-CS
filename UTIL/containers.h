#ifndef CONTAINERS_H
#define CONTAINERS_H

#include <string>
#include <deque>
#include <mutex>
#include <thread>
#include <sstream>

/*     FILE SAVE CONTAINER - CC_SAVE        */

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

/*     MUXED STRING CONTAINER FOR XPS QUEUE        */

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


#include "containers_template.h"

#endif // CONTAINERS_H
