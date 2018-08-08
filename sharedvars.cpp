#include "sharedvars.h"

/* GUI <-> XPS thread communication */
std::mutex GUI_XPS;
mxvar_ip XPS_ip(&GUI_XPS, std::string("192.168.0.254"));
mxvar_port XPS_port(&GUI_XPS, 5001);
mxvar<std::string> Xaxis_groupname(&GUI_XPS,std::string("GROUP1"));
mxvar<std::string> Yaxis_groupname(&GUI_XPS,std::string("GROUP2"));
mxvar<std::string> Zaxis_groupname(&GUI_XPS,std::string("GROUP3"));
mxvar<int> XPS_keepalive(&GUI_XPS, 500);     //keepalive and connect timeout, in ms
mxvar<bool> XPS_end(&GUI_XPS,false);         //for signaling the XPS thread it's time to close
mxvar<bool> XPS_connected(&GUI_XPS,false);   //XPS thread -> GUI


/* GUI <-> MAKO thread communication */
std::mutex GUI_MAKO;
mxvar<int> MAKO_keepalive(&GUI_MAKO, 500);   //in ms
mxvar<bool> MAKO_connected(&GUI_XPS,false);   //MAKO thread -> GUI


/* GUI <-> RPTY thread communication */
std::mutex GUI_RPTY;
mxvar_ip RPTY_ip(&GUI_RPTY, std::string("192.168.1.2"));
mxvar_port RPTY_port(&GUI_RPTY, 32);
mxvar<int> RPTY_keepalive(&GUI_RPTY, 500);   //in ms
mxvar<bool> RPTY_connected(&GUI_XPS,false);   //RPTY thread -> GUI
