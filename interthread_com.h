#ifndef INTERTHREAD_COM_H
#define INTERTHREAD_COM_H

#include "mutex_containers.h"

/* GUI <-> XPS thread communication */
static std::mutex GUI_XPS;
static mxvar_ip XPS_ip(&GUI_XPS, std::string("192.168.0.254"));
static mxvar_port XPS_port(&GUI_XPS, 5001);
static mxvar<std::string> Xaxis_groupname(&GUI_XPS,std::string("GROUP1"));
static mxvar<std::string> Yaxis_groupname(&GUI_XPS,std::string("GROUP2"));
static mxvar<std::string> Zaxis_groupname(&GUI_XPS,std::string("GROUP3"));
static mxvar<int> XPS_keepalive(&GUI_XPS, 500);     //in ms


/* GUI <-> MAKO thread communication */
static std::mutex GUI_MAKO;
static mxvar<int> MAKO_keepalive(&GUI_MAKO, 500);   //in ms


/* GUI <-> RPTY thread communication */
static std::mutex GUI_RPTY;
static mxvar_ip RPTY_ip(&GUI_RPTY, std::string("192.168.1.2"));
static mxvar_port RPTY_port(&GUI_RPTY, 32);
static mxvar<int> RPTY_keepalive(&GUI_RPTY, 500);   //in ms


#endif // INTERTHREAD_COM_H
