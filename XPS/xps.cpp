#include "xps.h"

PVTobj::PVTobj():verified(false){}
void PVTobj::clear(){
    data.str(std::string());
}

void PVTobj::_add(int n, double val){
    n++;
    if(n!=(1+2*go.pXPS->groups[ID].AxisNum)) {std::cerr<<"ERROR: You tried to add a line into a PVT file with the wrong number of parameters for group "<<go.pXPS->groupGetName(ID)<<", gave "<<n<<" pars but "<<go.pXPS->groups[ID].AxisNum<<" axes are defined for this group.\n"; go.quit();}
    data<<val<<"\n";
}

    /*~~~ XPS ~~~*/

XPS::XPS() : _writef(false), limit(true){
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

        std::this_thread::sleep_for (std::chrono::milliseconds(1));
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


void XPS::initGroup(GroupID ID){                 //add a queue containing groups, add command initGroups that inits them all, do this for commands below too
    execCommand("GroupInitialize",groupGetName(ID));
    flushQueue();
}
void XPS::initGroups(){
    for (int i=0;i!=_GROUP_NUM;i++) initGroup(groups[i].ID);
}
void XPS::homeGroup(GroupID ID){
    execCommand("GroupHomeSearchAndRelativeMove",groupGetName(ID), axisCoords[ID].pos[0], axisCoords[ID].pos[1], axisCoords[ID].pos[2]);
    flushQueue();
}
void XPS::homeGroups(){
    for (int i=0;i!=_GROUP_NUM;i++) homeGroup(groups[i].ID);
}
void XPS::killGroup(GroupID ID){
    execCommand("GroupKill",groupGetName(ID));
    flushQueue();
}
void XPS::killGroups(){
    for (int i=0;i!=_GROUP_NUM;i++) killGroup(groups[i].ID);
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

pPVTobj XPS::createNewPVTobj(GroupID ID, std::string filename){
    pPVTobj a = new PVTobj;
    a->ID=ID;
    a->filename=filename;
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
xps_dat XPS::verifyPVTobj(pPVTobj obj){
    xps_ret ret;
    execCommand(&ret, "MultipleAxesPVTVerification",groupGetName(obj->ID),obj->filename);
    ret.block_till_done();
    obj->verified=(ret.v.retval==0);
    return ret.v;
}
xps_dat XPS::execPVTobj(pPVTobj obj){
    xps_ret ret;
    if (!obj->verified) return ret.v;  //retval should be -9999 indicating an error
    execCommand(&ret, "MultipleAxesPVTExecution",groupGetName(obj->ID),obj->filename,1);
    ret.block_till_done();
    return ret.v;
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


XPS::raxis XPS::getPos(GroupID ID){
    std::lock_guard<std::mutex>lock(axisCoords[ID].mx);
    return raxis(&axisCoords[ID]);
}
void XPS::getPos(GroupID ID, raxis& pos){
    std::lock_guard<std::mutex>lock(axisCoords[ID].mx);
    pos=raxis(&axisCoords[ID]);
}

void XPS::MoveRelative(GroupID ID, double val){
    std::lock_guard<std::mutex>lock(axisCoords[ID].mx);
    _MoveRelative(0, ID, val);
}
void XPS::MoveAbsolute(GroupID ID, double val){
    std::lock_guard<std::mutex>lock(axisCoords[ID].mx);
    _MoveAbsolute(0, ID, val);
}
void XPS::_MoveRelative(int n, GroupID ID, double val){
    axisCoords[ID].pos[n]+=val; n++;
    if(n!=groups[ID].AxisNum) {std::cerr<<"ERROR: You called MoveRelative with the wrong number of parameters for group "<<groupGetName(ID)<<", gave "<<n<<" pars but "<<groups[ID].AxisNum<<" axes are defined for this group.\n"; go.quit();}
    __MoveAbsolute(ID);
}
void XPS::_MoveAbsolute(int n, GroupID ID, double val){
    axisCoords[ID].pos[n]=val; n++;
    if(n!=groups[ID].AxisNum) {std::cerr<<"ERROR: You called MoveAbsolute with the wrong number of parameters for group "<<groupGetName(ID)<<", gave "<<n<<" pars but "<<groups[ID].AxisNum<<" axes are defined for this group.\n"; go.quit();}
    __MoveAbsolute(ID);
}

void XPS::__MoveAbsolute(GroupID ID){ //the mutex should be locked by caller
    if(limit) XPS::_restrict_pos(axisCoords[ID]);
    else limit=true;
    execCommand("GroupMoveAbsolute",groupGetName(ID),axisCoords[ID].pos[0],axisCoords[ID].pos[1],axisCoords[ID].pos[2]);           //TODO perhaps implement move return value
}
void XPS::setLimit(GroupID ID, int axis, elimit lim){   //the mutex should be locked by caller
    std::lock_guard<std::mutex>lock(axisCoords[ID].mx);
    if(axis<0||axis>=axisCoords[ID].num) {std::cerr<<"ERROR: You tried using setlimit on axis no "<<axis+1<<", but group has ",axisCoords[ID].num," axes.\n";go.quit();return;}
    if(lim==min)
        axisCoords[ID].min[axis]=axisCoords[ID].pos[axis];
    else if(lim==max)
        axisCoords[ID].max[axis]=axisCoords[ID].pos[axis];
}
void XPS::_restrict_pos(axis& pos){ //the mutex should be locked by caller
    for(int i=0; i!=pos.num; i++){
        if(pos.pos[i]<pos.min[i]) pos.pos[i]=pos.min[i];
        else if(pos.pos[i]>pos.max[i]) pos.pos[i]=pos.max[i];
    }
}

void XPS::groupSetName(GroupID ID, std::string name){
    std::lock_guard<std::mutex>lock(gmx);
    groups[ID].groupname=name;
}
std::string XPS::groupGetName(GroupID ID){
    std::lock_guard<std::mutex>lock(gmx);
    return groups[ID].groupname;
}
