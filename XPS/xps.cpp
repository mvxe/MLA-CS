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
    execCommandNow("GroupInitialize (",sw.XYZ_groupname.get(),")");
    flushQueue();
}
void XPS::homeGroups(){
    execCommandNow("GroupHomeSearchAndRelativeMove (",sw.XYZ_groupname.get(), ",", sw.Xaxis_position.get(), ",", sw.Yaxis_position.get(), ",", sw.Zaxis_position.get(),")");
    flushQueue();
}
void XPS::killGroups(){
    execCommandNow("GroupKill (",sw.XYZ_groupname.get(),")");
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

void XPS::addToPVTqueue(std::string str){
    ftpmx.lock();
    upPVTfile.append(str);
    ftpmx.unlock();
}
void XPS::clearPVTqueue(){
    ftpmx.lock();
    upPVTfile.clear();
    ftpmx.unlock();
}

std::string XPS::copyPVToverFTP(std::string name){
    ftpmx.lock();
    std::ostringstream os;
    try{
        cURLpp::Easy ftpHandle;
        std::stringstream datastr;
        if(!upPVTfile.empty()) datastr<<upPVTfile;
        else datastr<<" ";
        ftpHandle.setOpt(cURLpp::Options::Url(util::toString("ftp://",sw.XPS_ip.get(),TRAJ_PATH,name).c_str()));
        ftpHandle.setOpt(cURLpp::Options::UserPwd("Administrator:Administrator"));
        ftpHandle.setOpt(cURLpp::Options::ReadStream(&datastr));
        ftpHandle.setOpt(cURLpp::Options::InfileSize(datastr.str().size()));
        ftpHandle.setOpt(cURLpp::Options::Upload(true));
        curlpp::options::WriteStream ws(&os);
        ftpHandle.setOpt(ws);
        ftpHandle.perform();
    }
    catch( cURLpp::RuntimeError &e ){
        os<<e.what()<<std::endl;
    }
    catch( cURLpp::LogicError &e ){
        os<<e.what()<<std::endl;
    }
    ftpmx.unlock();
    return os.str();
}
std::string XPS::listPVTfiles(){
    std::ostringstream os;
    ftpmx.lock();
    try{
        cURLpp::Easy ftpHandle;
        ftpHandle.setOpt(cURLpp::Options::Url(util::toString("ftp://",sw.XPS_ip.get(),TRAJ_PATH).c_str()));
        ftpHandle.setOpt(cURLpp::Options::UserPwd("Administrator:Administrator"));
        curlpp::options::WriteStream ws(&os);
        ftpHandle.setOpt(ws);
        ftpHandle.perform();
    }
    catch( cURLpp::RuntimeError &e ){
        os<<e.what()<<std::endl;
    }
    catch( cURLpp::LogicError &e ){
        os<<e.what()<<std::endl;
    }
    ftpmx.unlock();
    return os.str();
}
void XPS::execPVTQueue(std::string name){
    execCommandNow("MultipleAxesPVTVerification (",sw.XYZ_groupname.get(),",",name,")");
    execCommandNow("MultipleAxesPVTExecution (",sw.XYZ_groupname.get(),",",name,",1)");
}
