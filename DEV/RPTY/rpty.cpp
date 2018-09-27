#include "DEV/RPTY/rpty.h"
#include "globals.h"

RPTY::RPTY(){}
RPTY::~RPTY(){}

void RPTY::run(){
    std::string tmp;
    for (;;){
        while (!connected && !end){
                //resolving...
            std::string resname;    //this is when user enters hostname instead of ip
            if (resolve(IP.get(), port.get(), &resname)){
                resname = "cannot resolve this hostname";
                std::this_thread::sleep_for (std::chrono::milliseconds(keepalive.get()));
            }
            else{
                    //connecting...
                connect(keepalive.get());
                if (connected) {/*TODO RPTY init*/}
            }
            IP.resolved.set(resname);
        }
        if(connected && (IP.changed() || port.changed())) {/*TODO RPTY disco*/ disconnect();}  //if the user changes the IP or port setting we disconnect

        std::this_thread::sleep_for (std::chrono::milliseconds(1));
        if (connected); /*TODO: RPTY do work here*/

        if(end){
            if (connected){
                /*TODO RPTY disco*/
                disconnect();
            }
            std::cout<<"RPTY thread exited.\n";
            done=true;
            return;
        }
    }
}
