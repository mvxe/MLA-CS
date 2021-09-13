#include "DEV/RPTY/rpty.h"
#include "globals.h"
#include "motionDevices/pinexactstage.h"
#include "motionDevices/simpleservo.h"

RPTY::RPTY(){
    conf["RPTY_ip"]=IP;
    conf["RPTY_port"]=port;
    conf["RPTY_keepalive"]=keepalive;

    conf["main_command_queue"]=main_cq;
    conf["helper_command_queue"]=helper_cq;
    conf["serial_command_queue"]=serial_cq;
    conf["main_acquisition_queue"]=main_aq;
    conf["serial_acquisition_queue"]=serial_aq;
}
RPTY::~RPTY(){
    for(auto& dev : motionAxes){
        delete dev.second.dev;
    }
}

void RPTY::run(){
    std::string tmp;
    std::this_thread::sleep_for (std::chrono::seconds(1));  //TODO remove
    for (;;){
        while (!connected && !end){
            std::cerr<<"not connected\n";
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
//                    for(int i=0;i!=4;i++){
//                        std::cerr<<"bufn:"<<i<<" lost="<<getNum(F2A_lostN, i)<<" inqueue="<<getNum(F2A_RSCur, i)<<" max="<<getNum(F2A_RSMax, i)<<"\n";
//                        std::cerr<<"bufn:"<<i<<" bufw: lost="<<getNum(A2F_lostN, i)<<" inqueue="<<getNum(A2F_RSCur, i)<<" max="<<getNum(A2F_RSMax, i)<<"\n";
//                    }
                    FIFOreset();
                    initMotionDevices();
                    referenceMotionDevices();
                    retraceMotionDevices();
                }
            }
            IP.resolved.set(resname);
        }
        if(connected && (IP.changed() || port.changed())) {/*TODO RPTY disco*/
            disconnect();}  //if the user changes the IP or port setting we disconnect

        std::this_thread::sleep_for (std::chrono::milliseconds(1));
        if (connected){ /*TODO: RPTY do work here*/
            if(recheck_position)
                if(getNum(A2F_RSCur, main_cq)!=0){
                    for(auto& dev : motionAxes){
                        std::lock_guard<std::mutex>lock(motionAxes[dev.first].dev->mux);
                        motionAxes[dev.first].dev->updatePosition();
                    }
                    recheck_position=false;
                }
        }
        if(end){
            if (connected){
                /*TODO RPTY disco*/

                deinitMotionDevices();
                disconnect();
                conf.save(); //TODO put to button
            }
            std::cout<<"RPTY thread exited.\n";
            done=true;
            return;
        }
    }
}

int RPTY::F2A_read(uint8_t queue, uint32_t *data, uint32_t size4){
    std::lock_guard<std::mutex>lock(mux);
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
    std::lock_guard<std::mutex>lock(mux);
    uint32_t command[3]={1,queue,size4};
    TCP_con::write(command,12);
    return TCP_con::write(data,4*size4);
}
int RPTY::getNum(getNumType statID, uint8_t queue){
    std::lock_guard<std::mutex>lock(mux);
    uint32_t command[2]={2,queue};
    command[0]+=(int)statID;
    TCP_con::write(command,8);
    uint32_t nret; int ret;
    do nret=TCP_con::read(&ret,4);
    while (nret<4);
    return ret;
}
int RPTY::A2F_trig(uint8_t queue){
    std::lock_guard<std::mutex>lock(mux);
    uint32_t command[2]={8,(uint32_t)(queue)};
    return TCP_con::write(command,8);
}
int RPTY::FIFOreset(uint8_t A2Fqueues, uint8_t F2Aqueues){
    std::lock_guard<std::mutex>lock(mux);
    uint32_t command[2]={9,(uint32_t)((A2Fqueues&0xF)|((F2Aqueues&0xF)<<4))};
    return TCP_con::write(command,8);
}
int RPTY::FIFOreset(){
    std::lock_guard<std::mutex>lock(mux);
    uint32_t command[2]={9,0x100};
    return TCP_con::write(command,8);
}
int RPTY::PIDreset(uint8_t PIDN){
    std::lock_guard<std::mutex>lock(mux);
    uint32_t command[2]={100,(uint32_t)(PIDN&0x3)};
    return TCP_con::write(command,8);
}
int RPTY::PIDreset(){
    std::lock_guard<std::mutex>lock(mux);
    uint32_t command[2]={100,0x4};
    return TCP_con::write(command,8);
}
int RPTY::A2F_loop(uint8_t queue, bool loop){
    std::lock_guard<std::mutex>lock(mux);
    uint32_t command[2]={10,(uint32_t)(queue)|(loop?0x10000:0)};
    return TCP_con::write(command,8);
}

void RPTY::motion(std::string axisID, double position, double velocity, double acceleration, motionFlags flags){
    cqueue cq;
    _motionDeviceThrowExc(axisID,"motion");
    std::lock_guard<std::mutex>lock(motionAxes[axisID].dev->mux);
    motionAxes[axisID].dev->motion(cq, position, velocity, acceleration, flags);
    executeQueue(cq, main_cq);
}

double RPTY::getMotionSetting(std::string axisID, mst setting){
    _motionDeviceThrowExc(axisID,"getMotionSetting");
    std::lock_guard<std::mutex>lock(motionAxes[axisID].dev->mux);
    switch (setting){
    case mst_position: return motionAxes[axisID].dev->position;
    case mst_minPosition: return motionAxes[axisID].dev->minPosition;
    case mst_maxPosition: return motionAxes[axisID].dev->maxPosition;
    case mst_lastPosition: return motionAxes[axisID].dev->lastPosition;
    case mst_restPosition: return motionAxes[axisID].dev->restPosition;
    case mst_defaultVelocity: return motionAxes[axisID].dev->defaultVelocity;
    case mst_maximumVelocity: return motionAxes[axisID].dev->maximumVelocity;
    case mst_defaultAcceleration: return motionAxes[axisID].dev->defaultAcceleration;
    case mst_maximumAcceleration: return motionAxes[axisID].dev->maximumAcceleration;
    }
}

int RPTY::getMotionError(std::string axisID){
    _motionDeviceThrowExc(axisID,"getMotionError");
    std::lock_guard<std::mutex>lock(motionAxes[axisID].dev->mux);
    return motionAxes[axisID].dev->getMotionError();
}
void RPTY::addMotionDevice(std::string axisID){
    if(axesInited) throw std::runtime_error(util::toString("In RPTY::addMotionDevice: cannot add new axes after running RPTY::initMotionDevices. Deinitialize them first."));
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
    motionAxes[axisID].dev->axisID=axisID;
    motionAxes[axisID].dev->parent=this;
    motionAxes[axisID].conf.load();
}
void RPTY::addGPIOEvent(std::string GPIOID){

}
void RPTY::initMotionDevices(){
    if(axesInited) throw std::runtime_error(util::toString("In RPTY::initMotionDevices: axes already initialized."));
    A2F_loop(helper_cq, false);
    FIFOreset(1<<helper_cq);
    cqueue hq, cq;
    hq.push_back(CQF::W4TRIG_INTR());
    for(auto& dev : motionAxes){
        std::lock_guard<std::mutex>lock(motionAxes[dev.first].dev->mux);
        motionAxes[dev.first].dev->initMotionDevice(cq, hq, _free_flag);
    }
    cq.push_back(CQF::TRIG_OTHER(1<<helper_cq));
    executeQueue(hq, helper_cq);
    executeQueue(cq, main_cq);

    A2F_loop(helper_cq, true);
    axesInited=true;
}
void RPTY::referenceMotionDevices(){
    if(!axesInited) throw std::runtime_error(util::toString("In RPTY::referenceMotionDevices: axes not initialized."));
    cqueue cq;
    cq.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::LOW,true,_free_flag-1,false));   // lock main until all flags must are low
    for(auto& dev : motionAxes){
        std::lock_guard<std::mutex>lock(motionAxes[dev.first].dev->mux);
        motionAxes[dev.first].dev->referenceMotionDevice(cq);
    }
    executeQueue(cq, main_cq);
}
void RPTY::retraceMotionDevices(){
    cqueue cq;
    cq.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::LOW,true,_free_flag-1,false));   // lock main until all flags must are low
    for(auto& dev : motionAxes){
        std::lock_guard<std::mutex>lock(motionAxes[dev.first].dev->mux);
        motionAxes[dev.first].dev->motion(cq, motionAxes[dev.first].dev->lastPosition, motionAxes[dev.first].dev->maximumVelocity, motionAxes[dev.first].dev->maximumAcceleration);
    }
    for(auto& dev : motionAxes) _addHold(cq, util::toString(dev.first,"_MOTION_ONTARGET"));
    executeQueue(cq, main_cq);
    for(auto& dev : motionAxes){
        std::lock_guard<std::mutex>lock(motionAxes[dev.first].dev->mux);
        std::cerr<<"Axis "<<dev.first<<" error code "<<motionAxes[dev.first].dev->getMotionError()<<"\n";
    }
}
void RPTY::deinitMotionDevices(){
    if(!axesInited) throw std::runtime_error(util::toString("In RPTY::deinitMotionDevices: axes not initialized."));
    cqueue cq;

    for(auto& dev : motionAxes){
        std::lock_guard<std::mutex>lock(motionAxes[dev.first].dev->mux);
        motionAxes[dev.first].dev->updatePosition();
        motionAxes[dev.first].dev->lastPosition=motionAxes[dev.first].dev->position;
    }
    for(auto& dev : motionAxes){
        std::lock_guard<std::mutex>lock(motionAxes[dev.first].dev->mux);
        motionAxes[dev.first].dev->motion(cq, motionAxes[dev.first].dev->restPosition, motionAxes[dev.first].dev->maximumVelocity, motionAxes[dev.first].dev->maximumAcceleration);
    }
    for(auto& dev : motionAxes) _addHold(cq, util::toString(dev.first,"_MOTION_ONTARGET"));
    for(auto& dev : motionAxes){
        std::lock_guard<std::mutex>lock(motionAxes[dev.first].dev->mux);
        motionAxes[dev.first].dev->deinitMotionDevice(cq);
    }
    executeQueue(cq, main_cq);
    axesInited=false;
}

inline void RPTY::_motionDeviceThrowExc(std::string axisID, std::string function){
    if(!motionAxes.count(axisID)) throw std::runtime_error(util::toString("In RPTY::",function,", axis ",axisID," has not been defined."));
    else if(!axesInited) throw std::runtime_error(util::toString("In RPTY::",function,", axes have not been initialized."));
}


void RPTY::executeQueue(cqueue& cq, uint8_t queue){
    A2F_write(queue, cq.data(), cq.size());
    cq.clear();
    recheck_position=true;
}

// command object related functions

void RPTY::CO_init(CO* a){
    commandObjects[a];
}
void RPTY::CO_delete(CO* a){
    commandObjects.erase(a);
}
void RPTY::CO_execute(CO* a){
    executeQueue(commandObjects[a], main_cq);
}
void RPTY::CO_addMotion(CO* a, std::string axisID, double position, double velocity, double acceleration, motionFlags flags){
    _motionDeviceThrowExc(axisID,"motion");
    std::lock_guard<std::mutex>lock(motionAxes[axisID].dev->mux);
    motionAxes[axisID].dev->motion(commandObjects[a], position, velocity, acceleration, flags);
}
void RPTY::CO_addDelay(CO* a, double delay){
    commandObjects[a].push_back(CQF::WAIT(delay*125000000));
}
void RPTY::_addHold(cqueue& cq, std::string condition){
    bool w4tfound=false;
    for(auto& commnd: holdEvents[condition].commands){
        // first we find last non-W4TRIG command in queue and we place the non-W4TRIG commands from condition there
        // in this way we can silmultaneously start polling multiple axes, saving time
        if((commnd&0xF0000000)==(2<<28)) w4tfound=true;     //is a W4TRIG

        if(w4tfound){       //is a W4TRIG
            cq.push_back(commnd);
        }else{
            cqueue::iterator it;
            if(!cq.empty()){
                it=cq.end();
                while(1){
                    if(((*(it-1))&0xF0000000)!=(2<<28)){    //is not a W4TRIG
                        it=cq.insert(it, commnd);
                        break;
                    }else if(it!=cq.begin()){
                        it--;
                    }else{
                        it=cq.insert(it, commnd);
                        break;
                    }
                }
            }else cq.push_back(commnd);
        }
    }
}
void RPTY::CO_addHold(CO* a, std::string condition){
    _addHold(commandObjects[a], condition);
}
void RPTY::CO_addGPIOEvent(CO* a, std::string GPIOID, bool state){

}
void RPTY::CO_clear(CO* a){
    commandObjects[a].clear();
}

