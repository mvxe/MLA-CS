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
            //std::cerr<<"not connected\n";
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
                    /* RPTY init */
                    initDevices();
                    referenceMotionDevices();
                    motionDevicesToLastPosition();
                }
            }
            IP.resolved.set(resname);
        }
        if(connected && (IP.changed() || port.changed())) {/*TODO RPTY disco*/
            disconnect();}  //if the user changes the IP or port setting we disconnect

        std::this_thread::sleep_for (std::chrono::milliseconds(10));
        if (connected){ /*TODO: RPTY do work here*/
            if(recheck_position){
                std::lock_guard<std::recursive_mutex>lock(mux);
                if(getNum(A2F_RSCur, main_cq)==0){
                    for(auto& dev : motionAxes){
                        motionAxes[dev.first].dev->updatePosition();
                        int err=motionAxes[dev.first].dev->getMotionError();
                        if(err) std::cerr<<"Got motion error: "<<err<<"\n";
                    }
                    recheck_position=false;
                }
            }
        }
        if(end){
            if (connected){
                /* RPTY disconnect */
                motionDevicesToRestPosition();
                deinitDevices();
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
    uint32_t command[2]={9,(uint32_t)((A2Fqueues&0xF)|((F2Aqueues&0xF)<<4))};
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

void RPTY::registerDevice(std::string ID, devType type){
    std::lock_guard<std::recursive_mutex>lock(mux);
    if(devicesInited) throw std::runtime_error(util::toString("In RPTY::registerDevice: cannot add new devices after running RPTY::initDevices. Deinitialize first."));
    switch(type){
    case dt_motion:
        conf[util::toString("axis_",ID)]=motionAxes[ID].conf;
        motionAxes[ID].conf["type"]=motionAxes[ID].type;
        motionAxes[ID].conf.load();
        setMotionDeviceType(ID);
        break;
    case dt_gpio:
        conf[util::toString("gpio_",ID)]=gpioDevices[ID].conf;
        gpioDevices[ID].conf.load();
        break;
    case dt_timer:
        timerDevices[ID];
        break;
    }
}
void RPTY::setMotionDeviceType(std::string axisID){
    std::lock_guard<std::recursive_mutex>lock(mux);
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

void RPTY::initDevices(){
    if(!connected) return;
    std::lock_guard<std::recursive_mutex>lock(mux);
    FIFOreset();
    if(devicesInited) throw std::runtime_error(util::toString("In RPTY::initDevices: devices already initialized."));
    _free_flag=1;
    cqueue hq, cq;

        // dt_motion
    hq.push_back(CQF::W4TRIG_INTR());
    for(auto& [key, dev] : motionAxes){
        dev.dev->initMotionDevice(cq, hq, _free_flag);
    }

        // dt_gpio
    for(auto& [key, dev] : gpioDevices){
        dev.initGPIO(cq);
    }

        // dt_timer
    for(auto& [key, dev] : timerDevices){
        dev.initTimer(_free_flag);
    }

    cq.push_back(CQF::TRIG_OTHER(1<<helper_cq));
    executeQueue(hq, helper_cq);
    executeQueue(cq, main_cq);

    A2F_loop(helper_cq, true);
    devicesInited=true;
}

void RPTY::deinitDevices(){
    if(!connected) return;
    std::lock_guard<std::recursive_mutex>lock(mux);
    if(!devicesInited) throw std::runtime_error(util::toString("In RPTY::deinitDevices: devices not initialized."));
    cqueue cq;

        // dt_motion

    for(auto& [key, dev] : motionAxes)
        dev.dev->lastPosition=dev.dev->position;
    for(auto& [key, dev] : motionAxes) {
        cqueue tmp;
        motionAxes[key].dev->holdOnTarget(tmp);
        _addHold(cq, tmp);
    }
    for(auto& [key, dev] : motionAxes)
        dev.dev->deinitMotionDevice(cq);

    // dt_gpio
    for(auto& [key, dev] : gpioDevices){
        dev.setGPIO(cq, false);
    }

    cq.push_back(CQF::FLAGS_MASK(_free_flag-1));
    cq.push_back(CQF::FLAGS_SHARED_SET(0x0000));
    FIFOreset(1<<timer_cq);
    executeQueue(cq, main_cq);
    devicesInited=false;
}

void RPTY::referenceMotionDevices(){
    if(!connected) return;
    std::lock_guard<std::recursive_mutex>lock(mux);
    if(!devicesInited) throw std::runtime_error(util::toString("In RPTY::referenceMotionDevices: devices not initialized."));
    cqueue cq;
    cq.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::LOW,true,_free_flag-1,false));   // lock main until all flags are low
    for(auto& [key, dev] : motionAxes)
        dev.dev->referenceMotionDevice(cq);
    cq.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::LOW,true,_free_flag-1,false));
    executeQueue(cq, main_cq);
}

std::vector<std::string> RPTY::getMotionDevices(){
    std::lock_guard<std::recursive_mutex>lock(mux);
    std::vector<std::string> ret;
    for(auto& [key, dev] : motionAxes) ret.push_back(key);
    return ret;
}
std::vector<std::string> RPTY::getDevices(){
    std::lock_guard<std::recursive_mutex>lock(mux);
    std::vector<std::string> ret;
    for(auto& [key, dev] : motionAxes) ret.push_back(key);
    for(auto& [key, dev] : gpioDevices) ret.push_back(key);
    for(auto& [key, dev] : timerDevices) ret.push_back(key);
    return ret;
}

double RPTY::getMotionSetting(std::string ID, mst setting){
    std::lock_guard<std::recursive_mutex>lock(mux);
    _motionDeviceThrowExc(ID,"getMotionSetting");
    switch (setting){
    case mst_position: return motionAxes[ID].dev->position;
    case mst_minPosition: return motionAxes[ID].dev->minPosition;
    case mst_maxPosition: return motionAxes[ID].dev->maxPosition;
    case mst_lastPosition: return motionAxes[ID].dev->lastPosition;
    case mst_restPosition: return motionAxes[ID].dev->restPosition;
    case mst_defaultVelocity: return motionAxes[ID].dev->defaultVelocity;
    case mst_maximumVelocity: return motionAxes[ID].dev->maximumVelocity;
    case mst_defaultAcceleration: return motionAxes[ID].dev->defaultAcceleration;
    case mst_maximumAcceleration: return motionAxes[ID].dev->maximumAcceleration;
    case mst_mininumStep: return motionAxes[ID].dev->minimumStep;
    }
}

void RPTY::motion(std::string ID, double position, double velocity, double acceleration, motionFlags flags){
    if(!connected) return;
    std::lock_guard<std::recursive_mutex>lock(mux);
    cqueue cq;
    _motionDeviceThrowExc(ID,"motion");
    motionAxes[ID].dev->motion(cq, position, velocity, acceleration, flags);
    executeQueue(cq, main_cq);
}

int RPTY::getError(std::string ID){
    if(!connected) return -1;
    std::lock_guard<std::recursive_mutex>lock(mux);
    if(motionAxes.find(ID)!=motionAxes.end()){
        _motionDeviceThrowExc(ID,"getMotionError");
        return motionAxes[ID].dev->getMotionError();
    }else if (gpioDevices.find(ID)!=gpioDevices.end()){
        return 0;
    }else if (timerDevices.find(ID)!=timerDevices.end()){
        return 0;
    }
}

void RPTY::setGPIO(std::string ID, bool state){
    if(!connected) return;
    std::lock_guard<std::recursive_mutex>lock(mux);
    cqueue cq;
    gpioDevices[ID].setGPIO(cq, state);
    executeQueue(cq, main_cq);
}
void RPTY::pulseGPIO(std::string ID, double duration){
    if(!connected) return;
    std::lock_guard<std::recursive_mutex>lock(mux);
    cqueue cq;
    gpioDevices[ID].pulseGPIO(cq, duration);
    executeQueue(cq, main_cq);
}

inline void RPTY::_motionDeviceThrowExc(std::string ID, std::string function){
    std::lock_guard<std::recursive_mutex>lock(mux);
    if(!motionAxes.count(ID)) throw std::runtime_error(util::toString("In RPTY::",function,", axis ",ID," has not been defined."));
    else if(!devicesInited) throw std::runtime_error(util::toString("In RPTY::",function,", devices have not been initialized."));
}

void RPTY::executeQueue(cqueue& cq, uint8_t queue){
    if(!connected) return;
    std::lock_guard<std::recursive_mutex>lock(mux);
    A2F_write(queue, cq.data(), cq.size());
    recheck_position=true;
}



// command object related functions

void RPTY::CO_init(CO* a){
    std::lock_guard<std::recursive_mutex>lock(mux);
    commandObjects[a];
}
void RPTY::CO_delete(CO* a){
    std::lock_guard<std::recursive_mutex>lock(mux);
    commandObjects.erase(a);
}
void RPTY::CO_execute(CO* a){
    std::lock_guard<std::recursive_mutex>lock(mux);
    if(!commandObjects[a].timer.empty())
        executeQueue(commandObjects[a].timer, timer_cq);
    if(!commandObjects[a].main.empty())
        executeQueue(commandObjects[a].main, main_cq);
}
void RPTY::CO_addMotion(CO* a, std::string axisID, double position, double velocity, double acceleration, motionFlags flags){
    std::lock_guard<std::recursive_mutex>lock(mux);
    _motionDeviceThrowExc(axisID,"motion");
    if(commandObjects[a].motionRemainders.find(axisID) != commandObjects[a].motionRemainders.end())
        position+=commandObjects[a].motionRemainders[axisID];
    commandObjects[a].motionRemainders[axisID]=motionAxes[axisID].dev->motion(commandObjects[a].main, position, velocity, acceleration, flags);
}
void RPTY::CO_addDelay(CO* a, double delay){
    std::lock_guard<std::recursive_mutex>lock(mux);
    commandObjects[a].main.push_back(CQF::WAIT(delay*125000000));
}

void RPTY::CO_setGPIO(CO* a, std::string ID, bool state){
    std::lock_guard<std::recursive_mutex>lock(mux);
    gpioDevices[ID].setGPIO(commandObjects[a].main, state);
}
void RPTY::CO_pulseGPIO(CO* a, std::string ID, double duration){
    std::lock_guard<std::recursive_mutex>lock(mux);
    gpioDevices[ID].pulseGPIO(commandObjects[a].main, duration);
}
void RPTY::CO_addHold(CO* a, std::string ID, _holdCondition condition){
    std::lock_guard<std::recursive_mutex>lock(mux);
    cqueue cq;
    switch(condition){
    case he_gpio_low:
        gpioDevices[ID].w4trig(cq, false);
        break;
    case he_gpio_high:
        gpioDevices[ID].w4trig(cq, true);
        break;
    case he_timer_done:
        timerDevices[ID].holdTimer(cq);
        break;
    case he_motion_ontarget:
        motionAxes[ID].dev->holdOnTarget(cq);
        break;
    }
    _addHold(commandObjects[a].main, cq);
}
void RPTY::CO_startTimer(CO* a, std::string ID, double duration){
    std::lock_guard<std::recursive_mutex>lock(mux);
    timerDevices[ID].addTimer(commandObjects[a].main, commandObjects[a].timer, duration);
}

void RPTY::_addHold(cqueue& cq, cqueue &cqhold){
    std::lock_guard<std::recursive_mutex>lock(mux);
    bool w4tfound=false;
    for(auto& commnd: cqhold){
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

void RPTY::CO_clear(CO* a, bool keepMotionRemainders){
    std::lock_guard<std::recursive_mutex>lock(mux);
    commandObjects[a].main.clear();
    commandObjects[a].timer.clear();
    if(!keepMotionRemainders) commandObjects[a].motionRemainders.clear();
}
double RPTY::getPulsePrecision(){
    return 8e-9;
}

