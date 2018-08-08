#include "xps.h"
#include <iostream>

XPS::XPS(){
    TCP_con();
}

void XPS::run(){    //this is the CPS thread loop
    std::string tmp;
    for (;;){
        XPS_connected.set(connected);
        while (!connected){
            connect(XPS_ip.get(), XPS_port.get());
            usleep(1000*XPS_keepalive.get());
        }

        usleep(1000*XPS_keepalive.get());
        //rw("set x 0",tmp);
        //std::cout<<tmp<<"\n";

        //do something here

        if(XPS_end.get()){
            //cleanup TODO
            XPS_end.set(false);
            return;
        }
    }
}
