#ifndef INTERTHREAD_COM_H
#define INTERTHREAD_COM_H

#include "mutex_containers.h"

/* GUI <-> XPS thread communication */
static std::mutex GUI_XPS;
static mxvar_ip XPS_ip(&GUI_XPS);
static mxvar_port XPS_port(&GUI_XPS);
static mxvar<std::string> Xaxis_groupname(&GUI_XPS);
static mxvar<std::string> Yaxis_groupname(&GUI_XPS);
static mxvar<std::string> Zaxis_groupname(&GUI_XPS);
static mxvar<int> XPS_keepalive(&GUI_XPS);     //in ms


/* GUI <-> MAKO thread communication */
static std::mutex GUI_MAKO;
static mxvar<int> MAKO_keepalive(&GUI_MAKO);   //in ms


/* GUI <-> RPTY thread communication */
static std::mutex GUI_RPTY;
static mxvar_ip RPTY_ip(&GUI_RPTY);
static mxvar_port RPTY_port(&GUI_RPTY);
static mxvar<int> RPTY_keepalive(&GUI_RPTY);   //in ms


#endif // INTERTHREAD_COM_H
