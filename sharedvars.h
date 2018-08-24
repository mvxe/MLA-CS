#ifndef SHAREDVARS_H
#define SHAREDVARS_H

#include "includes.h"
#include "mutex_containers.h"

class sharedvarsba{
public:
    sharedvarsba();     //to enforce the constructor is called before member constructors
    ~sharedvarsba();    //to enforce the destructor is called after member destructors
protected:
    std::deque <_fovar> var;
    std::stringstream buffer;   //contains a backup of conf file
};

struct _dcams{
    std::string ID;
    std::string description;
};


class _tqueue{
public:
    _tqueue(std::mutex* mux) : fps(mux,0), i(mux,0), isbusy(mux,false){}
    std::queue<cv::Mat*> qu;
    mxva<unsigned> fps;          //rounds to the closest divisor by two of the actual framerate (ie 9999 of 168 fps gives 168, 33 of 168 fps gives 28 fps, 0 means no acquisition)
    mxva<int> i;                 //for framerate reduction
    mxva<bool> isbusy;           //to be somewhat threadsafe, the thread that uses the images keeps this flag on while accessing the matrix so that the main thread doesnt overwrite it (this should cause skipped frames and some flashy errors onscreen)
};

class sharedvars : sharedvarsba{    //the vars to be saved in a file have two additional arguments: ...,&var,"varname")
public:
    /* GUI <-> XPS thread communication */
    std::mutex GUI_XPS;
    mxvar_ip XPS_ip = mxvar_ip(&GUI_XPS, std::string("192.168.0.254"),&var,"XPS_ip");
    mxvar_port XPS_port = mxvar_port(&GUI_XPS, 5001,&var,"XPS_port");
    mxvar<std::string> Xaxis_groupname = mxvar<std::string>(&GUI_XPS,std::string("GROUP1"),&var,"Xaxis_groupname");
    mxvar<std::string> Yaxis_groupname = mxvar<std::string>(&GUI_XPS,std::string("GROUP2"),&var,"Yaxis_groupname");
    mxvar<std::string> Zaxis_groupname = mxvar<std::string>(&GUI_XPS,std::string("GROUP3"),&var,"Zaxis_groupname");
    mxvar<int> XPS_keepalive = mxvar<int>(&GUI_XPS, 500,&var,"XPS_keepalive");     //keepalive and connect timeout, in ms
    mxva<bool> XPS_end = mxva<bool>(&GUI_XPS,false);                               //for signaling the XPS thread it's time to close
    mxva<bool> XPS_connected = mxva<bool>(&GUI_XPS,false);                         //XPS thread -> GUI


    /* GUI <-> MAKO thread communication */
    std::mutex GUI_MAKO;
    mxva<bool> MAKO_init_done = mxva<bool>(&GUI_MAKO,false);
    mxvar<int> MAKO_keepalive = mxvar<int>(&GUI_MAKO, 500,&var,"MAKO_keepalive");   //in ms
    mxva<bool> MAKO_end = mxva<bool>(&GUI_MAKO,false);
    mxva<std::vector<_dcams>*> MAKO_cam_desc = mxva<std::vector<_dcams>*>(&GUI_MAKO,nullptr);
    mxva<bool> MAKO_list = mxva<bool>(&GUI_MAKO,true);
    mxva<bool> MAKO_reco = mxva<bool>(&GUI_MAKO,true);

    mxva<bool> iuScope_connected = mxva<bool>(&GUI_MAKO,false);
    mxvar<std::string> iuScopeID = mxvar<std::string>(&GUI_MAKO,"none",&var,"iuScopeID");
    FQ* iuScope_img;                                              //iuScope -> GUI

    /*MAKO <-> Vimba events internal*/
    std::mutex MAKO_VMB;
    mxva<bool> MVM_ignore = mxva<bool>(&MAKO_VMB,false);

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
