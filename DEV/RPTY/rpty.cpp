#include "DEV/RPTY/rpty.h"
#include "globals.h"
#include "motionDevices/pinexactstage.h"
#include "motionDevices/simpleservo.h"

RPTY::RPTY(){}
RPTY::~RPTY(){
    for(auto& dev : motionAxes){
        delete dev.second.dev;
    }
}

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
                if (connected) {

                    /*TODO RPTY init*/
                    initMotionDevices();
                }
            }
            IP.resolved.set(resname);
        }
        if(connected && (IP.changed() || port.changed())) {/*TODO RPTY disco*/ disconnect();}  //if the user changes the IP or port setting we disconnect

        std::this_thread::sleep_for (std::chrono::milliseconds(1));
        if (connected); /*TODO: RPTY do work here*/

        if(end){
            if (connected){
                /*TODO RPTY disco*/

                deinitMotionDevices();
                disconnect();
            }
            std::cout<<"RPTY thread exited.\n";
            done=true;
            return;
        }
    }
}

int RPTY::F2A_read(uint8_t queue, uint32_t *data, uint32_t size4){
    uint32_t command[3]={0,queue,size4};
    TCP_con::write(command,12);
    uint32_t index_read=0;
    ssize_t ret=0;
    while(index_read<size4){
        ret=TCP_con::read(data+index_read,4*(size4-index_read))/4;
        if(ret<0) return -1;
        index_read+=ret;
    }
    return index_read;  //allways should =size4 else infinite loop, TODO fix maybe?
}
int RPTY::A2F_write(uint8_t queue, uint32_t *data, uint32_t size4){
    uint32_t command[3]={1,queue,size4};
    TCP_con::write(command,12);
    return TCP_con::write(data,4*size4);
}
int RPTY::getNum(getNumType statID, uint8_t queue){
    uint32_t command[2]={2,queue};
    command[0]+=(int)statID;
    TCP_con::write(command,8);
    uint32_t nret; int ret;
    do nret=TCP_con::read(&ret,4);
    while (nret<4);
    return ret;
}
int RPTY::A2F_trig(uint8_t queue){
    uint32_t command[2]={8,(uint32_t)(queue)};
    return TCP_con::write(command,8);
}
int RPTY::FIFOreset(uint8_t A2Fqueues, uint8_t F2Aqueues){
    uint32_t command[2]={9,(uint32_t)((A2Fqueues&0xF)&((F2Aqueues&0xF)<<4))};
    return TCP_con::write(command,8);
}
int RPTY::FIFOreset(){
    uint32_t command[2]={9,0x100};
    return TCP_con::write(command,8);
}
int RPTY::PIDreset(uint8_t PIDN){
    uint32_t command[2]={100,(uint32_t)(PIDN&0x3)};
    return TCP_con::write(command,8);
}
int RPTY::PIDreset(){
    uint32_t command[2]={100,0x4};
    return TCP_con::write(command,8);
}
int RPTY::A2F_loop(uint8_t queue, bool loop){
    uint32_t command[2]={10,(uint32_t)(queue)|(loop?0x10000:0)};
    return TCP_con::write(command,8);
}



void RPTY::executeQCS(QCS& cq, bool force){
    for(int i=0;i!=commandQueueNum;i++){
        if(cq.reset[i]) FIFOreset(1<<i);
        else if(cq.emptyReq[i])
            if(getNum(F2A_RSCur, i)!=0){
                if(force) FIFOreset(1<<i);
                else throw std::runtime_error("In RPTY::executeQCS, emptyReq is set, but the queue isn't empty");
            }
    }
    for(int i=0;i!=commandQueueNum;i++){
        if(cq.commands[i].empty()) continue;
        A2F_write(i, cq.commands[i].data(), cq.commands[i].size());
        if(cq.makeTrig[i])
            A2F_trig(i);
        if(cq.makeLoop[i])
            A2F_loop(i, true);
    }
    cq.clear();
}
void RPTY::executeQueue(std::vector<uint32_t>& cq, uint8_t queue){
    A2F_write(queue, cq.data(), cq.size());
    cq.clear();
}

void RPTY::motion(QCS& cq, movEv mEv){
    _motionDeviceThrowExc(mEv.axisID,"motion");
    motionAxes[mEv.axisID].dev->motion(cq, mEv);
}
void RPTY::motion(std::vector<uint32_t>& cq, movEv mEv){
    _motionDeviceThrowExc(mEv.axisID,"motion");
    motionAxes[mEv.axisID].dev->motion(cq, mEv);
}
void RPTY::motion(movEv mEv){
    _motionDeviceThrowExc(mEv.axisID,"motion");
    motionAxes[mEv.axisID].dev->motion(mEv);
}
template <typename... Args>
void RPTY::motion(QCS& cq, movEv mEv, Args&&... args){
    motion(cq, mEv);
    motion(cq, args...);
}
template <typename... Args>
void RPTY::motion(std::vector<uint32_t>& cq, movEv mEv, Args&&... args){
    motion(cq, mEv);
    motion(cq, args...);
}
template <typename... Args>
void RPTY::motion(movEv mEv, Args&&... args){
    motion(mEv);
    motion(args...);
}
double RPTY::getMotionSetting(std::string axisID, mst setting){
    _motionDeviceThrowExc(axisID,"getMotionSetting");
    return motionAxes[axisID].dev->getMotionSetting(setting);
}

void RPTY::getCurrentPosition(std::string axisID, double& position){
    _motionDeviceThrowExc(axisID,"getCurrentPosition");
    motionAxes[axisID].dev->getCurrentPosition(position);
}
template <typename... Args>
void RPTY::getCurrentPosition(std::string axisID, double& position, Args&&... args){
    getCurrentPosition(axisID, position);
    getCurrentPosition(axisID, args...);
}
void RPTY::getMotionError(std::string axisID, int& error){
    _motionDeviceThrowExc(axisID,"getMotionError");
    motionAxes[axisID].dev->getMotionError(error);
}
template <typename... Args>
void RPTY::getMotionError(std::string axisID, int& error, Args&&... args){
    getMotionError(axisID, error);
    getMotionError(axisID, args...);
}

void RPTY::addMotionDevice(std::string axisID){
    conf[util::toString("axis_",axisID)]=motionAxes[axisID].conf;
    motionAxes[axisID].conf["type"]=motionAxes[axisID].type;
    motionAxes[axisID].conf.load();
    setMotionDeviceType(axisID, motionAxes[axisID].type);
}
void RPTY::setMotionDeviceType(std::string axisID, std::string type){
    if(motionAxes[axisID].dev!=nullptr) delete motionAxes[axisID].dev;
    motionAxes[axisID].type=type;
    if("md_PINEXACTStage"==motionAxes[axisID].type){
            motionAxes[axisID].dev=new rpMotionDevice_PINEXACTStage;
            motionAxes[axisID].conf["rpMotionDevice_PINEXACTStage"]=motionAxes[axisID].dev->conf;
    }else if("md_SimpleServo"==motionAxes[axisID].type){
            motionAxes[axisID].dev=new rpMotionDevice_SimpleServo;
            motionAxes[axisID].conf["rpMotionDevice_SimpleServo"]=motionAxes[axisID].dev->conf;
    }else{
        motionAxes[axisID].conf["type"].comments.push_back("Implemented devices: md_PINEXACTStage, md_SimpleServo.");
        motionAxes[axisID].conf.save();
        throw std::runtime_error(util::toString("In RPTY::initMotionDevice, uncrecognized/nonexistant motion device defined for axis ",axisID,". Saved possible options to conf."));
    }
    motionAxes[axisID].dev->parent=this;
    motionAxes[axisID].conf.load();
}
void RPTY::initMotionDevice(std::string axisID){
    if(!motionAxes.count(axisID)) throw std::runtime_error(util::toString("In RPTY::initMotionDevice, axis ",axisID," has not been defined. Define it using RPTY::addMotionDevice."));
    if(motionAxes[axisID].initialized==true)  deinitMotionDevice(axisID);

    motionAxes[axisID].dev->initMotionDevice();
    motionAxes[axisID].initialized=true;
}
void RPTY::initMotionDevices(){
    for(auto& dev : motionAxes){
        initMotionDevice(dev.first);
    }
}
void RPTY::deinitMotionDevice(std::string axisID){
    if(!motionAxes.count(axisID)) throw std::runtime_error(util::toString("In RPTY::deinitMotionDevice, axis ",axisID," has not been defined. Define it using RPTY::addMotionDevice."));
    if(motionAxes[axisID].initialized==false) return;

    motionAxes[axisID].dev->deinitMotionDevice();
    motionAxes[axisID].initialized=false;
}
void RPTY::deinitMotionDevices(){
    for(auto& dev : motionAxes){
        deinitMotionDevice(dev.first);
    }
}



inline void RPTY::_motionDeviceThrowExc(std::string axisID, std::string function){
    if(!motionAxes.count(axisID)) throw std::runtime_error(util::toString("In RPTY::",function,", axis ",axisID," has not been defined."));
    else if(!motionAxes[axisID].initialized) throw std::runtime_error(util::toString("In RPTY::",function,", axis ",axisID," has not been initialized."));
}
