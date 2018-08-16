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
    mxvar<bool> XPS_end = mxvar<bool>(&GUI_XPS,false);                             //for signaling the XPS thread it's time to close
    mxvar<bool> XPS_connected = mxvar<bool>(&GUI_XPS,false);                       //XPS thread -> GUI


    /* GUI <-> MAKO thread communication */
    std::mutex GUI_MAKO;
    mxvar<int> MAKO_keepalive = mxvar<int>(&GUI_MAKO, 500,&var,"MAKO_keepalive");   //in ms
    mxvar<bool> MAKO_end = mxvar<bool>(&GUI_MAKO,false);
    mxvar<bool> iuScope_connected = mxvar<bool>(&GUI_MAKO,false);
    mxva<std::vector<_dcams>*> MAKO_cam_desc = mxva<std::vector<_dcams>*>(&GUI_MAKO,nullptr);
    mxvar<std::string> iuScopeID = mxvar<std::string>(&GUI_MAKO,"none",&var,"iuScopeID");
    mxvar<bool> MAKO_list = mxvar<bool>(&GUI_MAKO,true);

    /*MAKO <-> Vimba events internal*/
    std::mutex MAKO_VMB;
    mxvar<bool> MVM_ignore = mxvar<bool>(&MAKO_VMB,false);

    /* GUI <-> RPTY thread communication */
    std::mutex GUI_RPTY;
    mxvar_ip RPTY_ip = mxvar_ip(&GUI_RPTY, std::string("192.168.1.2"),&var,"RPTY_ip");
    mxvar_port RPTY_port = mxvar_port(&GUI_RPTY, 32,&var,"RPTY_port");
    mxvar<int> RPTY_keepalive = mxvar<int>(&GUI_RPTY, 500,&var,"RPTY_keepalive");   //in ms
    mxvar<bool> RPTY_end = mxvar<bool>(&GUI_RPTY,false);
    mxvar<bool> RPTY_connected = mxvar<bool>(&GUI_RPTY,false);
};

extern sharedvars sw;


#endif // SHAREDVARS_H
