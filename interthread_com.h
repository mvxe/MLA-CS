#ifndef INTERTHREAD_COM_H
#define INTERTHREAD_COM_H

#include "mutex_containers.h"

/* GUI <-> XPS thread communication */
static std::mutex GUI_XPS;
mxvar_ip XPS_ip(&GUI_XPS);
mxvar_port XPS_port(&GUI_XPS);
mxvar<std::string> Xaxis_groupname(&GUI_XPS);
mxvar<std::string> Yaxis_groupname(&GUI_XPS);
mxvar<std::string> Zaxis_groupname(&GUI_XPS);
mxvar<int> XPS_keepalive(&GUI_XPS);     //in ms


/* GUI <-> MAKO thread communication */
static std::mutex GUI_MAKO;
mxvar<int> MAKO_keepalive(&GUI_MAKO);   //in ms


/* GUI <-> RPTY thread communication */
static std::mutex GUI_RPTY;
mxvar_ip RPTY_ip(&GUI_RPTY);
mxvar_port RPTY_port(&GUI_RPTY);
mxvar<int> RPTY_keepalive(&GUI_RPTY);   //in ms


#endif // INTERTHREAD_COM_H
