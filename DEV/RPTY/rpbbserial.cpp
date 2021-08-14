#include "rpbbserial.h"
#include <iostream>

RPBBSerial::RPBBSerial(){
          // TODO init vars to conf
}
void RPBBSerial::serial_init(std::vector<uint32_t> &commands, int8_t gpio_o, int8_t gpio_i, double _baudrate, double __clock, double _stopbits){
    baudrate=_baudrate;
    _clock=__clock;
    stopbits=_stopbits;
    if(gpio_o<-1 || gpio_o>15 || gpio_i<-1 || gpio_i>15 || gpio_o==gpio_i)
        throw std::invalid_argument("Provided gpios for RPBBSerial::gpio_init are out of range or equal.");

    cyclesPerBit=std::lround(clock/baudrate);
    cyclesPerStopBit=std::lround(stopbits*clock/baudrate);

    if(gpio_o!=-1){     // output
        gpioN_o=(gpio_o<=7)?(1<<gpio_o):0;
        gpioP_o=(gpio_o>7 && gpio_o<=15)?(1<<(gpio_o-8)):0;
        commands.push_back(CQF::GPIO_MASK(gpioN_o,gpioP_o,0));
        commands.push_back(CQF::GPIO_DIR (0,0,0));              // configure gpio pin as output
        commands.push_back(CQF::GPIO_VAL (gpioN_o,gpioP_o,0));  // set pin high (idle serial line)
        inited_o=true;
    }
    if(gpio_i!=-1){     // input
        gpioN_i=(gpio_i<=7)?(1<<gpio_i):0;
        gpioP_i=(gpio_i>7 && gpio_i<=15)?(1<<(gpio_i-8)):0;
        commands.push_back(CQF::GPIO_MASK(gpioN_i,gpioP_i,0));
        commands.push_back(CQF::GPIO_DIR (gpioN_i,gpioP_i,0));  // configure gpio pin as input
        inited_i=true;
    }
}
void RPBBSerial::__throw_serial_ack(){throw std::logic_error("When calling RPBBSerial::serial_ack with num=0, there should already be a trigger command in the queue (otherwise it execs before you make it loop).");}
void RPBBSerial::serial_ack(std::vector<uint32_t> &commands, unsigned ackqueue, unsigned num, const char breakChar){
        if(!inited_i) throw std::logic_error("Input has to be inited with RPBBSerial::serial_init before calling RPBBSerial::serial_ack.");
        bool breakOnLF=(num==0)?true:false;
        if(breakOnLF){
            if(commands.empty()) __throw_serial_ack();
            std::vector<uint32_t>::iterator trigIt=commands.end()-1;
            while(((*trigIt)&0xF0000000)!=(2<<28)){                         //until trigIt points at a WAIT4TRIG command
                if(trigIt==commands.begin()) __throw_serial_ack();
                trigIt--;
            }
            trigIt=commands.insert(trigIt, CQF::FLAGS_LOCAL_SET(0x8000));   // insert before the element ; insert these before the user defined trigger, they are NOT in the loop
            commands.insert(trigIt, CQF::FLAGS_MASK(0x8000));
            commands.push_back(CQF::IF_FLAGS_LOCAL(0,false,0x8000,false));  // this is in the loop
            num=1;
        }

        for(int j=0;j!=num;j++){
            commands.push_back(CQF::W4TRIG_GPIO(CQF::LOW,false,gpioN_i,gpioP_i,true));  // wait for the start bit
            commands.push_back(CQF::WAIT(cyclesPerBit/2));          // W4TRIG_GPIO min low duration (prevent noise trigger)
                                                                    // the stage ctrlr also seems to respond to each command with a short pulse
                                                                    // so this is set to half the cyclesPerBit to ignore that pulse
            commands.push_back(CQF::WAIT(1.0*cyclesPerBit-1));                          // wait until approx the middle of first bit
            for(int i=0;i!=8;i++){
                commands.push_back(CQF::ACK(1<<ackqueue, 0, CQF::gpioP_gpioN, true));   // measure 1 bit of data
                commands.push_back(CQF::ACK(1<<ackqueue, 0, CQF::gpioP_gpioN, false));
                if(breakOnLF) {
                    commands.push_back(CQF::IF_GPIO((breakChar&(1<<i))?CQF::LOW:CQF::HIGH,false,gpioN_i,gpioP_i));
                    commands.push_back(CQF::FLAGS_LOCAL_SET(0x0000));
                    commands.push_back(CQF::END());
                }
                commands.push_back(CQF::WAIT(1*cyclesPerBit-(breakOnLF?5:2)));            // wait until approx the middle of next bit
            }
        }
        if(breakOnLF) commands.push_back(CQF::END());
}


void RPBBSerial::string_to_command(std::string string, std::vector<uint32_t> &commands, uint16_t W4FLAGS_SHARED){
    if(!inited_o) throw std::logic_error("Output has to be inited with RPBBSerial::serial_init before calling RPBBSerial::string_to_command.");

    commands.push_back(CQF::GPIO_MASK(gpioN_o,gpioP_o,0));
    for (int i=0;i!=string.length();i++){
        if(W4FLAGS_SHARED)
            if(i==string.length()-1)
                commands.push_back(CQF::W4TRIG_FLAGS_SHARED(1,false,W4FLAGS_SHARED,false));             // for adding wait for trigger before last char is sent

        commands.push_back(CQF::W4TRIG_MIN_IN_QUEUE(20));       // forces the queue to wait for the whole character to be inserted

        commands.push_back(CQF::GPIO_VAL (0,0,0));              // start bit
        commands.push_back(CQF::WAIT(cyclesPerBit-1));
        for(int j=0;j!=8;j++){
            commands.push_back(CQF::GPIO_VAL ((((uint8_t)string[i])&(1<<j))?gpioN_o:0,(((uint8_t)string[i])&(1<<j))?gpioP_o:0,0));    // data bits
            commands.push_back(CQF::WAIT(cyclesPerBit-1));
        }
        commands.push_back(CQF::GPIO_VAL (gpioN_o,gpioP_o,0));  // stop bits
        commands.push_back(CQF::WAIT(cyclesPerStopBit-1));
    }
}

std::string RPBBSerial::convert_data_to_string(uint32_t *data, uint32_t size4){
    std::string tmp;
    convert_data_to_string(data, size4, tmp);
    return tmp;
}
void RPBBSerial::convert_data_to_string(uint32_t *data, uint32_t size4, std::string &string){
    char byte;
    if(!inited_i) throw std::logic_error("Input has to be inited with RPBBSerial::serial_init before calling RPBBSerial::convert_data_to_string.");
    if(size4%8)   throw std::logic_error("Size4(number of elements in *data) has to be a multiple of 8 in RPBBSerial::convert_data_to_string.");
    for(int i=0;i!=size4/8;i++){
        byte=0;
        for(int j=0;j!=8;j++){
            if(gpioN_i) byte^=( AQF::getN(data[i*8+j])&gpioN_i?(1<<j):0 );
            else        byte^=( AQF::getP(data[i*8+j])&gpioP_i?(1<<j):0 );
        }
        string+=byte;
    }
}
