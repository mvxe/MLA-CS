#ifndef SHAREDVARS_H
#define SHAREDVARS_H

#include <string>
#include <mutex>
#include <queue>
#include <fstream>
#include <iostream>
#include "mutex_containers.h"
#include "MAKO/frame_queues.h"
#include "XPS/xps.h"
#include "MAKO/mako.h"
class camobj;

class sharedvarsba{
public:
    sharedvarsba();     //to enforce the constructor is called before member constructors
    ~sharedvarsba();    //to enforce the destructor is called after member destructors
protected:
    std::deque <_fovar> var;
    std::stringstream buffer;   //contains a backup of conf file
};


class sharedvars : sharedvarsba{    //the vars to be saved in a file have two additional arguments: ...,&var,"varname")
public:
    /* global access to XPS functions */

    /* global GUI vars */
    std::mutex GUI_GL;
    mxbool GUI_disable = mxbool(&GUI_GL);


    std::mutex GUI_XPS;
        //these vars are really GUI only
    mxvar<int> xps_x_sen = mxvar<int>(&GUI_XPS,100,&var,"xps_x_sen");
    mxvar<int> xps_y_sen = mxvar<int>(&GUI_XPS,100,&var,"xps_y_sen");
    mxvar<int> xps_z_sen = mxvar<int>(&GUI_XPS,100,&var,"xps_z_sen");

    /* GUI <-> MAKO thread communication */
    std::mutex GUI_MAKO;

    //mxvar<double> iuScope_expo = mxvar<double>(&GUI_MAKO,442,&var,"iuScope_expo");          //in us



    /* GUI <-> RPTY thread communication */
    std::mutex GUI_RPTY;
    mxvar_ip RPTY_ip = mxvar_ip(&GUI_RPTY, std::string("192.168.1.2"),&var,"RPTY_ip");
    mxvar_port RPTY_port = mxvar_port(&GUI_RPTY, 32,&var,"RPTY_port");
    mxvar<int> RPTY_keepalive = mxvar<int>(&GUI_RPTY, 500,&var,"RPTY_keepalive");   //in ms
    mxva<bool> RPTY_end = mxva<bool>(&GUI_RPTY,false);
    mxva<bool> RPTY_connected = mxva<bool>(&GUI_RPTY,false);
};

extern sharedvars sw;


#endif // SHAREDVARS_H
