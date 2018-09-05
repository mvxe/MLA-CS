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

        std::this_thread::sleep_for (std::chrono::milliseconds(1));    //TODO remove this sleep (XPS)

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
    execCommand("GroupInitialize (",sw.XYZ_groupname.get(),")");
    flushQueue();
}
void XPS::homeGroups(){
    execCommand("GroupHomeSearchAndRelativeMove (",sw.XYZ_groupname.get(), ",", sw.Xaxis_position.get(), ",", sw.Yaxis_position.get(), ",", sw.Zaxis_position.get(),")");
    flushQueue();
}
void XPS::killGroups(){
    execCommand("GroupKill (",sw.XYZ_groupname.get(),")");
    flushQueue();
}
void XPS::flushQueue(){
    std::string tmp[2];
    if (!priority_queue.empty()) {
        mpq.lock();
    }
    while (!priority_queue.empty()){
        tmp[0]=priority_queue.front().comm;
        mpq.unlock();
        write(tmp[0]);
        read(tmp[1]);
        mpq.lock();
        if(!priority_queue.empty()){
            if(priority_queue.front().ret!=nullptr) *priority_queue.front().ret=tmp[1];
            priority_queue.pop();
        }
        mpq.unlock();
        if(tmp[1][0]!='0'){
            std::cerr<<"SENT: "<<tmp[0]<<"\n";
            std::cerr<<"RECV: "<<tmp[1]<<"\n";
        }
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
    std::string ret=execCommandR("MultipleAxesPVTVerification (",sw.XYZ_groupname.get(),",",name,")");
    if(ret[0]!='0')
        return;
    execCommand("MultipleAxesPVTExecution (",sw.XYZ_groupname.get(),",",name,",1)");
}

void XPS::XYZMoveRelative(double dX,double dY, double dZ, bool limit){
    sw.Xaxis_position.set(sw.Xaxis_position.get()+dX);
    sw.Yaxis_position.set(sw.Yaxis_position.get()+dY);
    sw.Zaxis_position.set(sw.Zaxis_position.get()+dZ);
    _XYZMoveAbsolute(limit);
}
void XPS::XYZMoveAbsolute(double X,double Y, double Z, bool limit){
    sw.Xaxis_position.set(X);
    sw.Yaxis_position.set(Y);
    sw.Zaxis_position.set(Z);
    _XYZMoveAbsolute(limit);
}
void XPS::_XYZMoveAbsolute(bool limit){
    if (limit){
        if(sw.Xaxis_min.get()>sw.Xaxis_position.get()) sw.Xaxis_position.set(sw.Xaxis_min.get());
        else if(sw.Xaxis_max.get()<sw.Xaxis_position.get()) sw.Xaxis_position.set(sw.Xaxis_max.get());
        if(sw.Yaxis_min.get()>sw.Yaxis_position.get()) sw.Yaxis_position.set(sw.Yaxis_min.get());
        else if(sw.Yaxis_max.get()<sw.Yaxis_position.get()) sw.Yaxis_position.set(sw.Yaxis_max.get());
        if(sw.Zaxis_min.get()>sw.Zaxis_position.get()) sw.Zaxis_position.set(sw.Zaxis_min.get());
        else if(sw.Zaxis_max.get()<sw.Zaxis_position.get()) sw.Zaxis_position.set(sw.Zaxis_max.get());
    }
    execCommand("GroupMoveAbsolute (",sw.XYZ_groupname.get(),",",sw.Xaxis_position.get(),",",sw.Yaxis_position.get(),",",sw.Zaxis_position.get(),")");
}
