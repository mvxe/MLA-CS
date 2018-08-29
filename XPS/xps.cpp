#include "xps.h"
#include "sharedvars.h"

XPS::XPS() : _writef(false){
    TCP_con();
}

XPS::~XPS(){
    if (connected) killGroups();
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
                if (connected) {initGroups();homeGroups();}
            }
            sw.XPS_ip.resolved.set(resname);
        }
        if(sw.XPS_ip.changed() || sw.XPS_port.changed()) {killGroups();disconnect();}  //if the user changes the IP or port setting we disconnect

        std::this_thread::sleep_for (std::chrono::milliseconds(sw.XPS_keepalive.get()));    //TODO remove this sleep (XPS)

        if (connected) {
            mpq.lock();
            for(;;){
                if(!priority_queue.empty()){
                    tmp=priority_queue.front();
                    mpq.unlock();
                    if (write(tmp)!=tmp.size()) {std::cerr<<"wrong written size!\n"; disconnect(); break;}
                    mpq.lock();
                    if (!priority_queue.empty()) priority_queue.pop();
                    mpq.unlock();
                    read(tmp);
                    std::cerr<<tmp<<"\n";
                    mpq.lock();
                }
                else if(!main_queue.empty() && _writef){
                    tmp=main_queue.front();
                    mpq.unlock();
                    if (write(tmp)!=tmp.size()) {std::cerr<<"wrong written size!\n"; disconnect(); break;}
                    mpq.lock();
                    if (!main_queue.empty()) main_queue.pop();
                    mpq.unlock();
                    read(tmp);
                    std::cerr<<tmp<<"\n";
                    mpq.lock();
                }
                else {
                    mpq.unlock();
                    break;
                }
            }
        }


        if(sw.XPS_end.get()){
            //cleanup TODO
            std::cout<<"XPS thread exited.\n";
            sw.XPS_end.set(false);
            return;
        }
    }
}

void XPS::addCommandToQueue(std::string command){
    mpq.lock();
    main_queue.push(command);
    mpq.unlock();
}
void XPS::clearCommandQueue(void){
    mpq.lock();
    while(!main_queue.empty())main_queue.pop();
    mpq.unlock();
}
void XPS::execQueueStart(void){
    mpq.lock();
    _writef=true;
    mpq.unlock();
}
void XPS::execQueueHalt(void){
    mpq.lock();
    _writef=false;
    mpq.unlock();
}
unsigned XPS::getCommandQueueSize(void){
    mpq.lock();
    unsigned ret=main_queue.size();
    mpq.unlock();
    return ret;
}


void XPS::initGroups(){
    execCommandNow("GroupInitialize ",sw.Xaxis_groupname.get());
    execCommandNow("GroupInitialize ",sw.Yaxis_groupname.get());
    execCommandNow("GroupInitialize ",sw.Zaxis_groupname.get());
}
void XPS::homeGroups(){
    execCommandNow("GroupHomeSearchAndRelativeMove ",sw.Xaxis_groupname.get(), " ", sw.Xaxis_position.get());
    execCommandNow("GroupHomeSearchAndRelativeMove ",sw.Yaxis_groupname.get(), " ", sw.Yaxis_position.get());
    execCommandNow("GroupHomeSearchAndRelativeMove ",sw.Zaxis_groupname.get(), " ", sw.Zaxis_position.get());
}
void XPS::killGroups(){
    execCommandNow("GroupKill ",sw.Xaxis_groupname.get());
    execCommandNow("GroupKill ",sw.Yaxis_groupname.get());
    execCommandNow("GroupKill ",sw.Zaxis_groupname.get());
}
