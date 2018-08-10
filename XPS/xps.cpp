#include "xps.h"

XPS::XPS(){
    TCP_con();
}

XPS::~XPS(){
}

void XPS::run(){    //this is the XPS thread loop
    std::string tmp;
    for (;;){
        sw.XPS_connected.set(connected);
        while (!connected && !sw.XPS_end.get()){
                //resolving...
            std::string resname;    //this is when user enters hostname instead of ip
            if (resolve(sw.XPS_ip.get(), sw.XPS_port.get(), &resname)){
                resname = "cannot resolve this hostname";
                std::this_thread::sleep_for (std::chrono::milliseconds(sw.XPS_keepalive.get()));
            }
            else{
                    //connecting...
                connect(sw.XPS_keepalive.get());
            }
            sw.XPS_ip.resolved.set(resname);
        }
        if(sw.XPS_ip.changed() || sw.XPS_port.changed()) disconnect();  //if the user changes the IP or port setting we disconnect

        std::this_thread::sleep_for (std::chrono::milliseconds(sw.XPS_keepalive.get()));
        //rw("set x 0",tmp);
        //std::cout<<tmp<<"\n";

        //do something here

        if(sw.XPS_end.get()){
            //cleanup TODO
            std::cout<<"XPS thread exited.\n";
            sw.XPS_end.set(false);
            return;
        }
    }
}
