#include "pinexactstage.h"
#include "../rpty.h"

rpMotionDevice_PINEXACTStage::rpMotionDevice_PINEXACTStage(){
    minPosition=-13;
    maxPosition=+13;
    homePosition=0;
    lastPosition=0;
    defaultVelocity=10;
    maximumVelocity=10;
    defaultAcceleration=50;
    maximumAcceleration=50;
    conf["serial_gpio_o"]=serial_gpio_o;
    conf["serial_gpio_o"].comments.push_back("N (0-7), P (8-15)");
    conf["serial_gpio_i"]=serial_gpio_i;
    conf["serial_gpio_i"].comments.push_back("N (0-7), P (8-15)");
    conf["motionType"]=reinterpret_cast<int&>(mType);
    conf["motionType"].comments.push_back("0=mt_nanostepping_delay, 1=mt_nanostepping_ontarget, 2=mt_alternating_delay, 3=mt_alternating_ontarget");
    conf["settleWindow"]=settleWindow;
    conf["settleWindow"].comments.push_back("Settle window for on target, 1 count is 0.5nm.");
    conf["settleTime"]=settleTime;
    conf["settleTime"].comments.push_back("Settle time for on target, in seconds.");
}
rpMotionDevice_PINEXACTStage::~rpMotionDevice_PINEXACTStage(){
}

void rpMotionDevice_PINEXACTStage::_modesetAltNs(std::vector<uint32_t>& cq, bool _alternating){
    if(alternating!=_alternating){
        alternating=_alternating;
        serial.string_to_command(util::toString("SPA 1 0x7001A00 ",_alternating?"1":"0","\n"), cq);
    }
}
void rpMotionDevice_PINEXACTStage::motion(std::vector<uint32_t>& cq, double position, double velocity, double acceleration, bool relativeMove){
    double tpos=relativeMove?lastPosition+position:position;
    if(tpos>maxPosition || tpos<minPosition)  throw std::invalid_argument(util::toString("In rpMotionDevice_PINEXACTStage::motion for axis ",axisID,", target position is not within min/max: target poisition=",tpos,", min=",minPosition,", max=", maxPosition));
    if(velocity==0) velocity=defaultVelocity;
    if(acceleration==0) acceleration=defaultAcceleration;
    if(velocity!=last_velocity) {
        serial.string_to_command(util::toString("VEL 1 ",velocity,"\n"), cq);
        last_velocity=velocity;
    }
    if(acceleration!=last_acceleration){
        serial.string_to_command(util::toString("ACC 1 ",acceleration,"\n"), cq);
        last_acceleration=acceleration;
    }
    std::string com=relativeMove?"MVR":"MOV";
    if(relativeMove) lastPosition+=position;
    else lastPosition=position;
    switch(mType){
    case mt_nanostepping_delay:
        //_modesetAltNs(cq, false);
        serial.string_to_command(util::toString(com," 1 ",position,"\n"), cq);
        //TODO relative move boundary tracking
        //TODO delay calculation
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
}
//void rpMotionDevice_PINEXACTStage::motion(movEv mEv){
//    std::vector<uint32_t> commands;
//    motion(commands, mEv);
//    parent->executeQueue(commands, parent->main_cq);
//}
void rpMotionDevice_PINEXACTStage::_readTillCharReadyAck(unsigned num, char breakChar){
    std::vector<uint32_t> commands;
    if(num==0) commands.push_back(CQF::W4TRIG_INTR());
    serial.serial_ack(commands, parent->serial_aq, num, breakChar);
    //parent->FIFOreset(1<<parent->serial_cq, 1<<parent->serial_aq);    //TODO reseting FIFO messes with gpio : fix fpga code
    parent->executeQueue(commands, parent->serial_cq);
    if(num==0) parent->A2F_loop(parent->serial_cq, true);
    if(num==0) parent->A2F_trig(parent->serial_cq);
}
void rpMotionDevice_PINEXACTStage::_readTillChar(std::string& readStr){
    std::vector<uint32_t> read;
    while(1){
        while(parent->getNum(RPTY::F2A_RSCur,parent->serial_aq)<8)  std::this_thread::sleep_for (std::chrono::milliseconds(10));
        unsigned toread=parent->getNum(RPTY::F2A_RSCur,parent->serial_aq)/8;
        read.reserve(toread*8);
        parent->F2A_read(parent->serial_aq, read.data(), toread*8);
        readStr+=serial.convert_data_to_string(read.data(), toread*8);
        std::cerr<<"got string: "<<readStr<<"\n";
        read.clear();
        if(readStr.back()=='\n') break;
    }
}
void rpMotionDevice_PINEXACTStage::getCurrentPosition(double& position){
    _readTillCharReadyAck();
    std::vector<uint32_t> commands;
    serial.string_to_command("POS?\n", commands);
    parent->executeQueue(commands, parent->main_cq);
    std::string readStr;
    _readTillChar(readStr);
    position=std::stod(readStr.substr(readStr.find("=")+1));
}
void rpMotionDevice_PINEXACTStage::getMotionError(int& error){
    _readTillCharReadyAck(2);
    std::vector<uint32_t> commands;
    serial.string_to_command("ERR?\n", commands);
    parent->executeQueue(commands, parent->main_cq);
    std::string readStr;
    _readTillChar(readStr);
    error=std::stoi(readStr);
}

void rpMotionDevice_PINEXACTStage::initMotionDevice(){
    std::vector<uint32_t> commands;
    serial.serial_init(commands, serial_gpio_o, serial_gpio_i, 115200, 125000000, 1);
    serial.string_to_command("SVO 1 1\n", commands);
    serial.string_to_command(util::toString("SPA 1 0x3F ",settleTime,"\n"), commands);
    serial.string_to_command(util::toString("SPA 1 0x36 ",settleWindow,"\n"), commands);
    commands.push_back(CQF::WAIT(0.1*serial.clock));
    serial.string_to_command("FRF\n", commands);
    parent->executeQueue(commands, parent->main_cq);
    _wait4ev(_ev_wait4rdy);

    motion(commands,lastPosition,maximumVelocity,maximumAcceleration);  // todo save current position and move to it
    parent->executeQueue(commands, parent->main_cq);

    int error;
    getMotionError(error);
    if(error) std::cerr<<"Motion device error:"<<error<<"\n";   //TODO add ID to rpMotionDevice and note it here
}
void rpMotionDevice_PINEXACTStage::deinitMotionDevice(){
    std::vector<uint32_t> commands;
    motion(commands,homePosition,maximumVelocity,maximumAcceleration);
    parent->executeQueue(commands, parent->main_cq);
    std::cerr<<"moved\n";
    _wait4ev(_ev_wait4ont);
    std::cerr<<"waited\n";
    serial.string_to_command("SVO 1 0\n", commands);        // relaxes piezos
    parent->executeQueue(commands, parent->main_cq);
}

void rpMotionDevice_PINEXACTStage::_wait4ev(_wevent ev){
    std::vector<uint32_t> commands;
    uint16_t __FLAG_SHARED=0x0100;
    switch(ev){
    case _ev_wait4rdy:
        _wait4rdy(commands,__FLAG_SHARED);
        break;
    case _ev_wait4ont:
        _wait4ont(commands,__FLAG_SHARED);
        break;
    }
    parent->executeQueue(commands, parent->helper_cq);
    parent->A2F_loop(parent->helper_cq, true);
    commands.push_back(CQF::FLAGS_MASK(__FLAG_SHARED));
    commands.push_back(CQF::FLAGS_SHARED_SET(__FLAG_SHARED));
    commands.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::LOW,false,__FLAG_SHARED,false));
    commands.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::LOW,false,__FLAG_SHARED,false));   // twice because getNum(RPTY::A2F_RSCur,parent->main_cq) returns 0 for current holding W4TRIG :: TODO implement ON_HOLD flag
    parent->executeQueue(commands, parent->main_cq);
    while(parent->getNum(RPTY::A2F_RSCur,parent->main_cq)!=0)  std::this_thread::sleep_for (std::chrono::milliseconds(10));
    parent->A2F_loop(parent->helper_cq, false);
    parent->FIFOreset(1<<parent->helper_cq);
}

void rpMotionDevice_PINEXACTStage::_wait4rdy(std::vector<uint32_t> &commands, uint16_t __FLAG_SHARED){
    std::string cmd;
    cmd+=(unsigned char)0x07;       // #7 (Request Controller Ready Status)
    commands.push_back(CQF::FLAGS_MASK(__FLAG_SHARED));
    commands.push_back(CQF::FLAGS_SHARED_SET(0x0000));
    commands.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::HIGH,false,__FLAG_SHARED,false));              // first W4T gets lost in loop
    commands.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::HIGH,false,__FLAG_SHARED,false));              // if CTRL_RDY==FALSE or main thread triggered it through __FLAG_SHARED
        serial.string_to_command(cmd, commands, false);
        // 0xB1=rdy, 0xB0 not rdy - so we are interested in bit 0
        commands.push_back(CQF::W4TRIG_GPIO(CQF::LOW,false,serial.gpioN_i,serial.gpioP_i,true));    // wait for the start bit of the byte
        commands.push_back(CQF::WAIT(0.5*serial.cyclesPerBit));
        commands.push_back(CQF::WAIT(1.0*serial.cyclesPerBit));
        commands.push_back(CQF::IF_GPIO(CQF::HIGH,false,serial.gpioN_i,serial.gpioP_i,true));       // if CTRL_RDY==TRUE bit 0
        commands.push_back(CQF::WAIT(10));
            commands.push_back(CQF::FLAGS_MASK(__FLAG_SHARED));
            commands.push_back(CQF::FLAGS_SHARED_SET(0x0000));
        commands.push_back(CQF::END());
        commands.push_back(CQF::WAIT(7.0*serial.cyclesPerBit-1));                                   // wait until the end
    commands.push_back(CQF::END());
}

void rpMotionDevice_PINEXACTStage::_wait4ont(std::vector<uint32_t> &commands, uint16_t __FLAG_SHARED){
    std::string cmd;
    cmd+=(unsigned char)0x04;       // #4 (Request Status Register)
    commands.push_back(CQF::FLAGS_MASK(__FLAG_SHARED));
    commands.push_back(CQF::FLAGS_SHARED_SET(0x0000));
    commands.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::HIGH,false,__FLAG_SHARED,false));              // first W4T gets lost in loop
    commands.push_back(CQF::W4TRIG_FLAGS_SHARED(CQF::HIGH,false,__FLAG_SHARED,false));              // if ON_TARGET==FALSE or main thread triggered it through __FLAG_SHARED
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
                    commands.push_back(CQF::FLAGS_MASK(__FLAG_SHARED));
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
