#include "xps.h"
#include "sharedvars.h"

XPS::XPS() : _writef(false){
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
                if (connected) {initGroups();homeGroups();}
            }
            sw.XPS_ip.resolved.set(resname);
        }
        if(sw.XPS_ip.changed() || sw.XPS_port.changed()) {killGroups();disconnect();}  //if the user changes the IP or port setting we disconnect

        std::this_thread::sleep_for (std::chrono::milliseconds(sw.XPS_keepalive.get()));    //TODO remove this sleep (XPS)

        if (connected)
            flushQueue();


        if(sw.XPS_end.get()){
            if (connected){
                killGroups();
                disconnect();
            }
            std::cout<<"XPS thread exited.\n";
            sw.XPS_end.set(false);
            return;
        }
    }
}


void XPS::initGroups(){
    execCommandNow("GroupInitialize (",sw.Xaxis_groupname.get(),")");
    execCommandNow("GroupInitialize (",sw.Yaxis_groupname.get(),")");
    execCommandNow("GroupInitialize (",sw.Zaxis_groupname.get(),")");
    flushQueue();
}
void XPS::homeGroups(){
    execCommandNow("GroupHomeSearchAndRelativeMove (",sw.Xaxis_groupname.get(), ",", sw.Xaxis_position.get(),")");
    execCommandNow("GroupHomeSearchAndRelativeMove (",sw.Yaxis_groupname.get(), ",", sw.Yaxis_position.get(),")");
    execCommandNow("GroupHomeSearchAndRelativeMove (",sw.Zaxis_groupname.get(), ",", sw.Zaxis_position.get(),")");
    flushQueue();
}
void XPS::killGroups(){
    execCommandNow("GroupKill (",sw.Xaxis_groupname.get(),")");
    execCommandNow("GroupKill (",sw.Yaxis_groupname.get(),")");
    execCommandNow("GroupKill (",sw.Zaxis_groupname.get(),")");
    flushQueue();
}
void XPS::flushQueue(){
    std::string tmp;
    if (!priority_queue.empty()) {
        //while(read(tmp)!=0);
        mpq.lock();
    }
    while (!priority_queue.empty()){
        tmp=priority_queue.front();
        mpq.unlock();
        //for (unsigned totw=0;totw!=tmp.size();totw+=write(tmp.substr(totw)));
        write(tmp);
        mpq.lock();
        std::cerr <<"SENT: "<<tmp<<"\n";
        if (!priority_queue.empty()) priority_queue.pop();
        mpq.unlock();
        //while(read(tmp)==0);
        read(tmp);
        std::cerr<<"RECV: "<<tmp<<"\n";
    }
}

void XPS::addToPVTqueue(){

}
void XPS::clearPVTqueue(){

}
void XPS::copyPVToverFTP(){
    ftpmx.lock();
    ftplib *ftp = new ftplib();
    std::cerr<<"Connect: "<<ftp->Connect(sw.XPS_ip.get().c_str())  <<"\n";
    std::cerr<<"Login: "<<ftp->Login("Administrator", "Administrator")    <<"\n";
    std::cerr<<"Dir: "<<ftp->Dir(NULL, "/ftp/pub")  <<"\n";
    std::cerr<<"Quit: "<<ftp->Quit() <<"\n";
    ftpmx.unlock();
}
void XPS::execPVTQueue(){

}
