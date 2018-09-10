#include "xps.h"

PVTobj::PVTobj():verified(false){}
void PVTobj::clear(){
    data.str(std::string());
}
template<typename... Args>
void PVTobj::add(double val, Args... vals){
    data<<val<<" ";
    add(vals...);
}
void PVTobj::add(double val){
    data<<val<<"\n";
}

    /*~~~ XPS ~~~*/

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
                if (connected) {initGroup(sw.XYZ_groupname.get());homeGroup(sw.XYZ_groupname.get());}
            }
            sw.XPS_ip.resolved.set(resname);
        }
        if(sw.XPS_ip.changed() || sw.XPS_port.changed()) {killGroup(sw.XYZ_groupname.get());disconnect();}  //if the user changes the IP or port setting we disconnect

        std::this_thread::sleep_for (std::chrono::milliseconds(1));
        if (connected)
            flushQueue();

        if(sw.XPS_end.get()){
            if (connected){
                killGroup(sw.XYZ_groupname.get());
                disconnect();
            }
            std::cout<<"XPS thread exited.\n";
            sw.XPS_end.set(false);
            return;
        }
    }
}


void XPS::initGroup(std::string groupname){                 //add a queue containing groups, add command initGroups that inits them all, do this for commands below too
    execCommand("GroupInitialize (",groupname,")");
    flushQueue();
}
void XPS::homeGroup(std::string groupname){
    execCommand("GroupHomeSearchAndRelativeMove (",groupname, ",", pos.posX, ",", pos.posY, ",", pos.posZ,")");
    flushQueue();
}
void XPS::killGroup(std::string groupname){
    execCommand("GroupKill (",groupname,")");
    flushQueue();
}
void XPS::flushQueue(){
    std::string tmp[2];
    mpq.lock();
    while (!priority_queue.empty()){
            tmp[0]=priority_queue.front().comm;
        mpq.unlock();
        write(tmp[0]);
        read(tmp[1]);
        mpq.lock();
            if(priority_queue.front().ret!=nullptr) priority_queue.front().ret->set_value(tmp[1]);
            priority_queue.pop();
        mpq.unlock();
        if(tmp[1][0]!='0'){
            std::cerr<<"SENT: "<<tmp[0]<<"\n";
            std::cerr<<"RECV: "<<tmp[1]<<"\n";
        }
        mpq.lock();
    }
    mpq.unlock();
}

pPVTobj XPS::createNewPVTobj(std::string motion_group, std::string filename){
    pPVTobj a = new PVTobj;
    a->groupname=motion_group;
    return a;
}
void XPS::destroyPVTobj(pPVTobj obj){
    if (obj!=nullptr) delete obj;
    obj=nullptr;
}
std::string XPS::copyPVToverFTP(pPVTobj obj){
    if (obj==nullptr) return "obj is nullptr!\n";
    std::ostringstream os;
    try{
        cURLpp::Easy ftpHandle;
        if (obj->data.str().size()==0) return "stream is empty!\n";
        ftpHandle.setOpt(cURLpp::Options::Url(util::toString("ftp://",sw.XPS_ip.get(),TRAJ_PATH,obj->filename).c_str()));
        ftpHandle.setOpt(cURLpp::Options::UserPwd("Administrator:Administrator"));
        ftpHandle.setOpt(cURLpp::Options::ReadStream(&obj->data));
        ftpHandle.setOpt(cURLpp::Options::InfileSize(obj->data.str().size()));
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
    obj->verified=false;
    return os.str();
}
xps_ret XPS::verifyPVTobj(pPVTobj obj){
    xps_ret ret;
    execCommand(&ret, "MultipleAxesPVTVerification (",obj->groupname,",",obj->filename,")");
    ret.block_till_done();
    if (ret.retval==0) obj->verified=true;
    return ret;
}
xps_ret XPS::execPVTobj(pPVTobj obj){
    xps_ret ret;
    if (!obj->verified) return ret;  //retval should be -9999 indicating an error
    execCommand(&ret, "MultipleAxesPVTExecution (",obj->groupname,",",obj->filename,",1)");
    ret.block_till_done();
    return ret;
}

std::string XPS::listPVTfiles(){
    std::ostringstream os;
    std::lock_guard<std::mutex>lock(ftpmx);
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
    return os.str();
}

void XPS::XYZMoveRelative(double dX,double dY, double dZ, bool limit){
    std::lock_guard<std::mutex>lock(posmx);
    pos.posX+=dX; pos.posY+=dY; pos.posZ+=dZ;
    _XYZMoveAbsolute(limit);
}
void XPS::XYZMoveAbsolute(double X,double Y, double Z, bool limit){
    std::lock_guard<std::mutex>lock(posmx);
    pos.posX=X;   pos.posY=Y;   pos.posZ=Z;
    _XYZMoveAbsolute(limit);
}
XPS::axisp XPS::getXYZpos(){
    std::lock_guard<std::mutex>lock(posmx);
    return pos;
}
void XPS::getXYZpos(XPS::axisp& rpos){
    std::lock_guard<std::mutex>lock(posmx);
    rpos=pos;
}

void XPS::_XYZMoveAbsolute(bool limit){
    if(limit) XPS::axisp_restrict_pos(pos);
    execCommand("GroupMoveAbsolute (",sw.XYZ_groupname.get(),",",pos.posX,",",pos.posY,",",pos.posZ,")");           //TODO perhaps implement move return value
}
void XPS::setLimit(elimit lim){
    std::lock_guard<std::mutex>lock(posmx);
    switch(lim){
        case minX: pos.minX=pos.posX; break;
        case minY: pos.minY=pos.posY; break;
        case minZ: pos.minZ=pos.posZ; break;
        case maxX: pos.maxX=pos.posX; break;
        case maxY: pos.maxY=pos.posY; break;
        case maxZ: pos.maxZ=pos.posZ; break;
    }
}
void XPS::axisp_restrict_pos(axisp &poss){
    if     (poss.minX>poss.posX) poss.posX=poss.minX;
    else if(poss.maxX<poss.posX) poss.posX=poss.maxX;
    if     (poss.minY>poss.posY) poss.posY=poss.minY;
    else if(poss.maxY<poss.posY) poss.posY=poss.maxY;
    if     (poss.minZ>poss.posZ) poss.posZ=poss.minZ;
    else if(poss.maxZ<poss.posZ) poss.posZ=poss.maxZ;
}
