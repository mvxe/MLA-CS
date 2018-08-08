#ifndef SHAREDVARS_H
#define SHAREDVARS_H

#include "mutex_containers.h"

/* GUI <-> XPS thread communication */
extern std::mutex GUI_XPS;
extern mxvar_ip XPS_ip;
extern mxvar_port XPS_port;
extern mxvar<std::string> Xaxis_groupname;
extern mxvar<std::string> Yaxis_groupname;
extern mxvar<std::string> Zaxis_groupname;
extern mxvar<int> XPS_keepalive;
extern mxvar<bool> XPS_end;
extern mxvar<bool> XPS_connected;


/* GUI <-> MAKO thread communication */
extern std::mutex GUI_MAKO;
extern mxvar<int> MAKO_keepalive;
extern mxvar<bool> MAKO_connected;



/* GUI <-> RPTY thread communication */
extern std::mutex GUI_RPTY;
extern mxvar_ip RPTY_ip;
extern mxvar_port RPTY_port;
extern mxvar<int> RPTY_keepalive;
extern mxvar<bool> RPTY_connected;


#endif // SHAREDVARS_H
