#ifndef CONTAINERS_H
#define CONTAINERS_H

#include <string>
#include <deque>

/*     FILE SAVE CONTAINER - CC_SAVE        */

struct _fo{    //for file ops
    std::string strname;
    std::string strval;
};
typedef std::deque<_fo> _savelst;

template <typename T>
class cc_save{
public:
    cc_save(T& var, T initial, _savelst* vec, std::string name);
    ~cc_save();
    T& val;
protected:
    _fo* tfvec;
};


#include "containers_template.h"
#endif // CONTAINERS_H
