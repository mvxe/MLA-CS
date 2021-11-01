#include "pinexactstage.h"
#include "../rpty.h"

rpMotionDevice_PINEXACTStage::rpMotionDevice_PINEXACTStage(){
    minPosition=-13;
    maxPosition=+13;
    lastPosition=0;
    restPosition=0;
    defaultVelocity=10;
    maximumVelocity=10;
    defaultAcceleration=50;
    maximumAcceleration=50;
    minimumStep=0.0000005;
    conf["minPosition"]=minPosition;
    conf["maxPosition"]=maxPosition;
    conf["lastPosition"]=lastPosition;
    conf["restPosition"]=restPosition;
    conf["defaultVelocity"]=defaultVelocity;
    //conf["maximumVelocity"]=maximumVelocity;
    conf["defaultAcceleration"]=defaultAcceleration;
    //conf["maximumAcceleration"]=maximumAcceleration;
    //conf["minimumStep"]=minimumStep;

    conf["serial_gpio_o"]=serial_gpio_o;
    conf["serial_gpio_o"].comments().push_back("N (0-7), P (8-15)");
    conf["serial_gpio_i"]=serial_gpio_i;
    conf["serial_gpio_i"].comments().push_back("N (0-7), P (8-15)");
    conf["motionType"]=reinterpret_cast<int&>(mType);
    conf["motionType"].comments().push_back("0=mt_nanostepping_delay, 1=mt_nanostepping_ontarget, 2=mt_alternating_delay, 3=mt_alternating_ontarget");
    conf["settleWindow"]=settleWindow;
    conf["settleWindow"].comments().push_back("Settle window for on target, 1 count is 0.5nm.");
    conf["settleTime"]=settleTime;
    conf["settleTime"].comments().push_back("Settle time for on target, in seconds.");
    conf["serialTimeout"]=serialTimeout;
    conf["serialTimeout"].comments().push_back("Timeout for serial acquisition, in seconds. Some commands will be resent after no response.");
}
rpMotionDevice_PINEXACTStage::~rpMotionDevice_PINEXACTStage(){
}

void rpMotionDevice_PINEXACTStage::_modesetAltNs(std::vector<uint32_t>& cq, bool _alternating){
    if(alternating!=_alternating){
        alternating=_alternating;
        serial.string_to_command(util::toString("SPA 1 0x7001A00 ",_alternating?"1":"0","\n"), cq);
    }
}
double rpMotionDevice_PINEXACTStage::motion(std::vector<uint32_t>& cq, double position, double velocity, double acceleration, CTRL::motionFlags flags){
    double posRemainder=position;
    position=std::round(position/minimumStep)*minimumStep;
    posRemainder-=position;
    if((flags&CTRL::MF_RELATIVE) && position==0) return posRemainder;
    cq.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::LOW,true,ontFlag|rdyFlag,false));    // wait for all flags to clear
    if((!(flags&CTRL::MF_RELATIVE)) && (position<minPosition || position>maxPosition))  throw std::invalid_argument(util::toString("In rpMotionDevice_PINEXACTStage::motion for axis ",axisID,", target position is not within min/max: target position=",position,", min=",minPosition,", max=", maxPosition));
    if(velocity==0) velocity=defaultVelocity;
    if(acceleration==0) acceleration=defaultAcceleration;
    if(velocity>maximumVelocity) throw std::invalid_argument(util::toString("In rpMotionDevice_PINEXACTStage::motion for axis ",axisID,", velocity>maximumVelocity"));
    if(acceleration>maximumAcceleration) throw std::invalid_argument(util::toString("In rpMotionDevice_PINEXACTStage::motion for axis ",axisID,", acceleration>maximumAcceleration"));
    if(velocity!=last_velocity) {
        serial.string_to_command(util::toString("VEL 1 ",velocity,"\n"), cq);
        last_velocity=velocity;
    }
    if(acceleration!=last_acceleration){
        serial.string_to_command(util::toString("ACC 1 ",acceleration,"\n"), cq);
        last_acceleration=acceleration;
    }
    std::string com=(flags&CTRL::MF_RELATIVE)?"MVR":"MOV";
    switch(mType){
    case mt_nanostepping_delay:
        //_modesetAltNs(cq, false);
        serial.string_to_command(util::toString(com," 1 ",position,"\n"), cq);
        break;
    case mt_nanostepping_ontarget:;
        //_modesetAltNs(cq, false);
        //TODO
        break;
    case mt_alternating_delay:;
        //_modesetAltNs(cq, true);
        //TODO
        break;
    case mt_alternating_ontarget:;
        //_modesetAltNs(cq, true);
        //TODO
    }
    return posRemainder;
}
//void rpMotionDevice_PINEXACTStage::motion(movEv mEv){
//    std::vector<uint32_t> commands;
//    motion(commands, mEv);
//    parent->executeQueue(commands, parent->main_cq);
//}
void rpMotionDevice_PINEXACTStage::_readQS(unsigned num){
    std::vector<uint32_t> commands;
    commands.push_back(CQF::FLAGS_MASK(serialFlag));
    commands.push_back(CQF::FLAGS_SHARED_SET(0x0000));
    if(num==0) commands.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::HIGH,true,serialFlag,false));
    commands.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::HIGH,true,serialFlag,false));
    serial.serial_ack(commands, parent->serial_aq, num);
    parent->FIFOreset(1<<parent->serial_cq, 1<<parent->serial_aq);
    parent->executeQueue(commands, parent->serial_cq);
    if(num==0) parent->A2F_loop(parent->serial_cq, true);
}
bool rpMotionDevice_PINEXACTStage::_readTillChar(std::string& readStr, char breakChar){
    std::vector<uint32_t> read;
    double time;
    while(1){
        time=0;
        unsigned toread=parent->getNum(RPTY::F2A_RSCur,parent->serial_aq)/8;
        while(toread<1){
            std::this_thread::sleep_for (std::chrono::milliseconds(10));
            time+=0.010;
            if(time>=serialTimeout){
                std::cerr<<"timeout in _readTillChar; got string: "<<readStr<<"\n";
                parent->FIFOreset(1<<parent->serial_cq, 1<<parent->serial_aq);
                parent->A2F_trig(parent->main_cq);
                return true;
            }
            toread=parent->getNum(RPTY::F2A_RSCur,parent->serial_aq)/8;
        }
        //std::cerr<<"toread="<<toread<<" bytes\n";
        read.reserve(toread*8);
        parent->F2A_read(parent->serial_aq, read.data(), toread*8);
        readStr+=serial.convert_data_to_string(read.data(), toread*8);
        //std::cerr<<"got string: "<<readStr<<"\n";
        read.clear();
        if(readStr.back()==breakChar) break;
    }
    parent->FIFOreset(1<<parent->serial_cq, 1<<parent->serial_aq);
    parent->A2F_trig(parent->main_cq);
    return false;
}
void rpMotionDevice_PINEXACTStage::updatePosition(){
    std::string readStr;
    do{
        _readQS(32);
        std::vector<uint32_t> commands;
        commands.push_back(CQF::FLAGS_MASK(rdyFlag));
        commands.push_back(CQF::FLAGS_SHARED_SET(rdyFlag));
        commands.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::LOW,true,ontFlag|rdyFlag,false));
        commands.push_back(CQF::WAIT(serial.cyclesPerBit*8*5));
        commands.push_back(CQF::FLAGS_MASK(serialFlag));
        commands.push_back(CQF::FLAGS_SHARED_SET(serialFlag));
        serial.string_to_command("MOV?\n", commands);
        //serial.string_to_command("POS?\n", commands); // POS returns the actual position - not what we want
        commands.push_back(CQF::W4TRIG_INTR());
        parent->executeQueue(commands, parent->main_cq);
    }while(_readTillChar(readStr,'\n'));
    position=std::stod(readStr.substr(readStr.find("=")+1));
}
int rpMotionDevice_PINEXACTStage::getMotionError(){
    std::string readStr;
    do{
        _readQS(5);
        std::vector<uint32_t> commands;
        commands.push_back(CQF::FLAGS_MASK(rdyFlag));
        commands.push_back(CQF::FLAGS_SHARED_SET(rdyFlag));
        commands.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::LOW,true,ontFlag|rdyFlag,false));
        commands.push_back(CQF::WAIT(serial.cyclesPerBit*8*5));
        commands.push_back(CQF::FLAGS_MASK(serialFlag));
        commands.push_back(CQF::FLAGS_SHARED_SET(serialFlag));
        serial.string_to_command("ERR?\n", commands);
        parent->executeQueue(commands, parent->main_cq);
    }while(_readTillChar(readStr,'\n'));
    int error=std::stoi(readStr);
    return error;
}

void rpMotionDevice_PINEXACTStage::initMotionDevice(std::vector<uint32_t>& cq, std::vector<uint32_t>& hq, unsigned& free_flag){
    serial.serial_init(cq, serial_gpio_o, serial_gpio_i, 115200, 125000000, 1);
    serial.string_to_command("SVO 1 1\n", cq);
    serial.string_to_command(util::toString("SPA 1 0x3F ",settleTime,"\n"), cq);
    serial.string_to_command(util::toString("SPA 1 0x36 ",settleWindow,"\n"), cq);
    serial.string_to_command(util::toString("VEL 1 ",defaultVelocity,"\n"), cq);
    serial.string_to_command(util::toString("ACC 1 ",defaultAcceleration,"\n"), cq);
    serial.string_to_command(util::toString("SPA 1 0x15 ",maxPosition,"\n"), cq);
    serial.string_to_command(util::toString("SPA 1 0x30 ",minPosition,"\n"), cq);
    rdyFlag=free_flag; free_flag<<=1;
    ontFlag=free_flag; free_flag<<=1;
    serialFlag=free_flag; free_flag<<=1;
    // TODO throw error if free_flag > 16bit
    cq.push_back(CQF::FLAGS_MASK(rdyFlag|ontFlag|serialFlag));
    cq.push_back(CQF::FLAGS_SHARED_SET(0x0000));

    _wait4rdy(hq);
    _wait4ont(hq);
}
void rpMotionDevice_PINEXACTStage::referenceMotionDevice(std::vector<uint32_t>& cq){
    cq.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::LOW,true,ontFlag|rdyFlag,false));
    serial.string_to_command("FRF\n", cq);
    cq.push_back(CQF::FLAGS_MASK(rdyFlag));
    cq.push_back(CQF::FLAGS_SHARED_SET(rdyFlag));
}
void rpMotionDevice_PINEXACTStage::deinitMotionDevice(std::vector<uint32_t>& cq){
    cq.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::LOW,true,ontFlag|rdyFlag,false));
    serial.string_to_command("SVO 1 0\n", cq);          // relaxes piezos
}

void rpMotionDevice_PINEXACTStage::_wait4rdy(std::vector<uint32_t> &commands){
    std::string cmd;
    cmd+=(unsigned char)0x07;       // #7 (Request Controller Ready Status)
    commands.push_back(CQF::IF_FLAGS_SHARED(CQF::HIGH,false,rdyFlag,false));                        // if CTRL_RDY==FALSE or main thread triggered it through __FLAG_SHARED
        serial.string_to_command(cmd, commands, false);
        // 0xB1=rdy, 0xB0 not rdy - so we are interested in bit 0
        commands.push_back(CQF::W4TRIG_GPIO(CQF::LOW,false,serial.gpioN_i,serial.gpioP_i,true));    // wait for the start bit of the byte
        commands.push_back(CQF::WAIT(0.5*serial.cyclesPerBit));
        commands.push_back(CQF::WAIT(1.0*serial.cyclesPerBit));
        commands.push_back(CQF::IF_GPIO(CQF::HIGH,false,serial.gpioN_i,serial.gpioP_i,true));       // if CTRL_RDY==TRUE bit 0
        commands.push_back(CQF::WAIT(10));
            commands.push_back(CQF::FLAGS_MASK(rdyFlag));
            commands.push_back(CQF::FLAGS_SHARED_SET(0x0000));
        commands.push_back(CQF::END());
        commands.push_back(CQF::WAIT(7.0*serial.cyclesPerBit-1));                                   // wait until the end
    commands.push_back(CQF::END());
}

void rpMotionDevice_PINEXACTStage::_wait4ont(std::vector<uint32_t> &commands){
    std::string cmd;
    cmd+=(unsigned char)0x04;       // #4 (Request Status Register)
    commands.push_back(CQF::IF_FLAGS_SHARED(CQF::HIGH,false,ontFlag,false));                        // if ON_TARGET==FALSE or main thread triggered it through __FLAG_SHARED
        commands.push_back(CQF::FLAGS_MASK(0x0003));
        commands.push_back(CQF::FLAGS_LOCAL_SET(0x0000));
        serial.string_to_command(cmd, commands, false);
        // we are interested in bit #15 (on target)
        // but the retard stage replies with a hexadecimal value in ascii: ie 0x1234
        // so we need to decode that, we are only interested then in the highest 4bits - writing up the table gives that ON_TARGET is true if either bit 3 XOR(^) bit 6 of this byte are true
        for(int i=0;i!=2;i++){  // skip 2 bytes
            commands.push_back(CQF::W4TRIG_GPIO(CQF::LOW,false,serial.gpioN_i,serial.gpioP_i,true));    // wait for the start bit
            commands.push_back(CQF::WAIT(serial.cyclesPerBit/2));                                       // W4TRIG_GPIO min low duration (prevent noise trigger) and ignore short pulses
            commands.push_back(CQF::WAIT(9.0*serial.cyclesPerBit-1));                                   // wait until the end of the byte (halway on the stop bit)
        }
        commands.push_back(CQF::W4TRIG_GPIO(CQF::LOW,false,serial.gpioN_i,serial.gpioP_i,true));        // wait for the start bit of the second byte
        commands.push_back(CQF::WAIT(serial.cyclesPerBit/2));
        commands.push_back(CQF::WAIT(4.0*serial.cyclesPerBit-1));                                       // wait until the middle of bit 3
        commands.push_back(CQF::IF_GPIO(CQF::HIGH,false,serial.gpioN_i,serial.gpioP_i,true));           // if ON_TARGET==TRUE bit 3
        commands.push_back(CQF::WAIT(10));
            commands.push_back(CQF::FLAGS_MASK(0x0001));
            commands.push_back(CQF::FLAGS_LOCAL_SET(0x0001));
        commands.push_back(CQF::END());
        commands.push_back(CQF::WAIT(3.0*serial.cyclesPerBit-1));                                       // wait until the middle of bit 6
        commands.push_back(CQF::IF_GPIO(CQF::HIGH,false,serial.gpioN_i,serial.gpioP_i,true));           // if ON_TARGET==TRUE bit 6
        commands.push_back(CQF::WAIT(10));
            commands.push_back(CQF::FLAGS_MASK(0x0002));
            commands.push_back(CQF::FLAGS_LOCAL_SET(0x0002));
        commands.push_back(CQF::END());
        commands.push_back(CQF::WAIT(2.0*serial.cyclesPerBit-1));                                       // wait until the end
        for(auto& i: {false,true}){
            commands.push_back(CQF::IF_FLAGS_LOCAL(i,false,0x0001,false));
                commands.push_back(CQF::IF_FLAGS_LOCAL(!i,false,0x0002,false));
                    commands.push_back(CQF::FLAGS_MASK(ontFlag));
                    commands.push_back(CQF::FLAGS_SHARED_SET(0x0000));                                  // only if bit 3 XOR(^) bit 6
                commands.push_back(CQF::END());
            commands.push_back(CQF::END());
        }
        for(int i=0;i!=3;i++){      // skip 3 bytes (the fourth will overlap with command send anyway)
            commands.push_back(CQF::W4TRIG_GPIO(CQF::LOW,false,serial.gpioN_i,serial.gpioP_i,true));    // wait for the start bit
            commands.push_back(CQF::WAIT(serial.cyclesPerBit/2));                                       // W4TRIG_GPIO min low duration (prevent noise trigger) and ignore short pulses
            commands.push_back(CQF::WAIT(9.0*serial.cyclesPerBit-1));                                   // wait until the end of the byte (halway on the stop bit)
        }
    commands.push_back(CQF::END());
}

void rpMotionDevice_PINEXACTStage::holdOnTarget(std::vector<uint32_t>& cq){
    cq.push_back(CQF::FLAGS_MASK(ontFlag));
    cq.push_back(CQF::FLAGS_SHARED_SET(ontFlag));
    cq.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::LOW,true,ontFlag|rdyFlag,false));
}
