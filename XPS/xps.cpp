#include "xps.h"

XPS::XPS(){
    TCP_con();
}

XPS::~XPS(){
}

void XPS::run(){    //this is the XPS thread loop
    std::string tmp;
    for (;;){
        XPS_connected.set(connected);
        while (!connected){
            connect(XPS_ip.get(), XPS_port.get());
            std::this_thread::sleep_for (std::chrono::milliseconds(XPS_keepalive.get()));
        }

        std::this_thread::sleep_for (std::chrono::milliseconds(XPS_keepalive.get()));
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
