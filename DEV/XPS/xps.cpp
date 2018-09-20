#include "DEV/XPS/xps.h"
#include "globals.h"

PVTobj::PVTobj(){}
void PVTobj::clear(){
    data.str(std::string());
}

void PVTobj::_add(int n, double val){
    n++;
    if(n!=(1+2*go.pXPS->groups[ID].AxisNum)) {std::cerr<<"ERROR: You tried to add a line into a PVT file with the wrong number of parameters for group "<<go.pXPS->groupGetName(ID)<<", gave "<<n<<" pars but "<<go.pXPS->groups[ID].AxisNum<<" axes are defined for this group.\n"; go.quit();}
    pvtqueue.push(val);
}

/*########## XPS ##########*/

XPS::XPS(){}
XPS::~XPS(){}
void XPS::run(){    //this is the XPS thread loop
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
                if (connected) {go.pXPS->execCommand("GPIODigitalSet","GPIO3.DO", 1,0);initGroups();homeGroups();flushQueue();}     //TODO put into same function
            }
            IP.resolved.set(resname);
        }
        if(connected && (IP.changed() || port.changed())) {killGroups();flushQueue();disconnect();}  //if the user changes the IP or port setting we disconnect

        std::this_thread::sleep_for (std::chrono::milliseconds(1));
        if (connected)
            flushQueue();

        if(end){
            if (connected){
                go.pXPS->execCommand("GPIODigitalSet","GPIO3.DO", 1,1);     //TODO put into same function and call here and above
                killGroups();
                flushQueue();
                disconnect();
            }
            std::cout<<"XPS thread exited.\n";
            return;
        }
    }
}


void XPS::initGroup(GroupID ID){                 //add a queue containing groups, add command initGroups that inits them all, do this for commands below too
    execCommand("GroupInitialize",groupGetName(ID));
}
void XPS::initGroups(){
    for (int i=0;i!=_GROUP_NUM;i++) initGroup(groups[i].ID);
}
void XPS::homeGroup(GroupID ID){
    execCommand("GroupHomeSearchAndRelativeMove",groupGetName(ID), axisCoords[ID].pos[0], axisCoords[ID].pos[1], axisCoords[ID].pos[2]);
}
void XPS::homeGroups(){
    for (int i=0;i!=_GROUP_NUM;i++) homeGroup(groups[i].ID);
}
void XPS::killGroup(GroupID ID){
    execCommand("GroupKill",groupGetName(ID));
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
std::string XPS::_copyPVToverFTP(pPVTobj obj){
    if (obj==nullptr) return "obj is nullptr!\n";
    std::ostringstream os;
    try{
        cURLpp::Easy ftpHandle;
        if (obj->data.str().size()==0) return "stream is empty!\n";
        ftpHandle.setOpt(cURLpp::Options::Url(util::toString("ftp://",IP.get(),TRAJ_PATH,obj->filename).c_str()));
        ftpHandle.setOpt(cURLpp::Options::UserPwd("Administrator:Administrator"));
        ftpHandle.setOpt(cURLpp::Options::ReadStream(&obj->data));
        ftpHandle.setOpt(cURLpp::Options::InfileSize(obj->data.str().size()));
        ftpHandle.setOpt(cURLpp::Options::Upload(true));
        curlpp::options::WriteStream ws(&os);
        ftpHandle.setOpt(ws);
        ftpHandle.perform();
    }
    catch( cURLpp::RuntimeError &e ){
        os<<e.what()<<"\n";
    }
    catch( cURLpp::LogicError &e ){
        os<<e.what()<<"\n";
    }
    obj->verified=false;
    return os.str();
}
exec_dat XPS::verifyPVTobj(pPVTobj obj){
    exec_ret ret;
    std::lock_guard<std::mutex>lock(axisCoords[obj->ID].mx);
    XPS::raxis taxis{axisCoords};
    const int oline=(1+2*axisCoords[obj->ID].num);
    while(!obj->pvtqueue.empty()){
        obj->data<<obj->pvtqueue.front();
        obj->pvtqueue.pop();
        for(int i=0; i!=axisCoords[obj->ID].num; i++){
            obj->data<<" "<<obj->pvtqueue.front();
            taxis.pos[i]+=obj->pvtqueue.front();
            if(taxis.pos[i]<taxis.min[i] || taxis.pos[i]>taxis.max[i]){
                clearPVTobj(obj);
                return {"-999,out of sw defined bounds",-999};
            }
            obj->pvtqueue.pop();
            obj->data<<" "<<obj->pvtqueue.front();
            obj->pvtqueue.pop();
        }
        obj->data<<"\n";
    }

    std::string tmp=_copyPVToverFTP(obj);
    if(!tmp.empty())
        return {tmp,-999};

    execCommand(&ret, "MultipleAxesPVTVerification",groupGetName(obj->ID),obj->filename);
    ret.block_till_done();
    obj->verified=(ret.v.retval==0);
    if(!obj->verified) clearPVTobj(obj);
    return ret.v;
}
void XPS::execPVTobj(pPVTobj obj, exec_ret* ret){
    if (!obj->verified) return;  //retval should be -9999 indicating an error
    if (ret!=nullptr) execCommand(ret, "MultipleAxesPVTExecution",groupGetName(obj->ID),obj->filename,1);
    else execCommand("MultipleAxesPVTExecution",groupGetName(obj->ID),obj->filename,1);
    syncPos(obj->ID);
}
exec_dat XPS::execPVTobjB(pPVTobj obj){
    exec_ret ret;
    if (!obj->verified) return ret.v;  //retval should be -9999 indicating an error
    execCommand(&ret, "MultipleAxesPVTExecution",groupGetName(obj->ID),obj->filename,1);
    ret.block_till_done();
    syncPos(obj->ID);
    return ret.v;
}
void XPS::clearPVTobj(pPVTobj obj){
    while (!obj->pvtqueue.empty()) obj->pvtqueue.pop();
    obj->data.str(std::string());
}

std::string XPS::listPVTfiles(){
    std::ostringstream os;
    std::lock_guard<std::mutex>lock(ftpmx);
    try{
        cURLpp::Easy ftpHandle;
        ftpHandle.setOpt(cURLpp::Options::Url(util::toString("ftp://",IP.get(),TRAJ_PATH).c_str()));
        ftpHandle.setOpt(cURLpp::Options::UserPwd("Administrator:Administrator"));
        curlpp::options::WriteStream ws(&os);
        ftpHandle.setOpt(ws);
        ftpHandle.perform();
    }
    catch( cURLpp::RuntimeError &e ){
        os<<e.what()<<"\n";
    }
    catch( cURLpp::LogicError &e ){
        os<<e.what()<<"\n";
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
void XPS::syncPos(GroupID ID){
    exec_ret* ret = new exec_ret();
    std::stringstream buf;
    buf<<groupGetName(ID);
    for (int i=0;i!=groups[ID].AxisNum;i++) buf<<",double *";
    execCommand(ret,"GroupPositionSetpointGet",buf.str());
    std::thread handle (&XPS::_syncPosHandle,this,ID,ret);
    handle.detach();
}
void XPS::_syncPosHandle(GroupID ID, exec_ret* ret){
    ret->block_till_done();
    if(ret->v.retval==0){
        std::istringstream cs(ret->v.retstr); cs.ignore();
        std::lock_guard<std::mutex>lock(axisCoords[ID].mx);
        for (int i=0;i!=axisCoords[ID].num;i++){
            cs.ignore();
            cs>>axisCoords[ID].pos[i];
        }
        std::cerr<<"Setpoint: "<<ret->v.retstr<<"\n";
    }
    delete ret;
}

void XPS::groupSetName(GroupID ID, std::string name){
    if(connected) killGroup(ID);
    {std::lock_guard<std::mutex>lock(gmx);
    groups[ID].groupname=name;}
    if(connected){
        initGroup(ID);
        homeGroup(ID);
    }
}
std::string XPS::groupGetName(GroupID ID){
    std::lock_guard<std::mutex>lock(gmx);
    return groups[ID].groupname;
}

void XPS::execCommand(std::string command){
    std::stringstream* nstrm=new std::stringstream();
    *nstrm<<command;
    eexecCommand(nstrm, nullptr, "(");
}
void XPS::execCommand(exec_ret* ret ,std::string command){
    std::stringstream* nstrm=new std::stringstream();
    *nstrm<<command;
    return eexecCommand(nstrm, ret, "(");
}
