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

    _cyclesPerBit=std::lround(clock/baudrate);
    cyclesPerStopBit=std::lround(stopbits*clock/baudrate);

    if(gpio_o!=-1){     // output
        _gpioN_o=(gpio_o<=7)?(1<<gpio_o):0;
        _gpioP_o=(gpio_o>7 && gpio_o<=15)?(1<<(gpio_o-8)):0;
        commands.push_back(CQF::GPIO_MASK(_gpioN_o,_gpioP_o,0));
        commands.push_back(CQF::GPIO_DIR (0,0,0));              // configure gpio pin as output
        commands.push_back(CQF::GPIO_VAL (_gpioN_o,_gpioP_o,0));  // set pin high (idle serial line)
        inited_o=true;
    }
    if(gpio_i!=-1){     // input
        _gpioN_i=(gpio_i<=7)?(1<<gpio_i):0;
        _gpioP_i=(gpio_i>7 && gpio_i<=15)?(1<<(gpio_i-8)):0;
        commands.push_back(CQF::GPIO_MASK(_gpioN_i,_gpioP_i,0));
        commands.push_back(CQF::GPIO_DIR (_gpioN_i,_gpioP_i,0));  // configure gpio pin as input
        inited_i=true;
    }
}
void RPBBSerial::__throw_serial_ack(){throw std::logic_error("When calling RPBBSerial::serial_ack with num=0, there should already be a trigger command in the queue (otherwise it execs before you make it loop).");}
void RPBBSerial::serial_ack(std::vector<uint32_t> &commands, unsigned ackqueue, unsigned num){
        if(!inited_i) throw std::logic_error("Input has to be inited with RPBBSerial::serial_init before calling RPBBSerial::serial_ack.");
        if(num==0) num=1;
        for(int j=0;j!=num;j++){
            commands.push_back(CQF::W4TRIG_GPIO(CQF::LOW,false,_gpioN_i,_gpioP_i,true));  // wait for the start bit
            commands.push_back(CQF::WAIT(_cyclesPerBit/2));          // W4TRIG_GPIO min low duration (prevent noise trigger)
                                                                    // the stage ctrlr also seems to respond to each command with a short pulse
                                                                    // so this is set to half the cyclesPerBit to ignore that pulse
            commands.push_back(CQF::WAIT(1.0*_cyclesPerBit-1));                          // wait until approx the middle of first bit
            for(int i=0;i!=8;i++){
                commands.push_back(CQF::ACK(1<<ackqueue, 0, CQF::gpioP_gpioN, true));   // measure 1 bit of data
                commands.push_back(CQF::ACK(1<<ackqueue, 0, CQF::gpioP_gpioN, false));
                commands.push_back(CQF::WAIT(1*_cyclesPerBit-2));            // wait until approx the middle of next bit
            }
        }
}


void RPBBSerial::string_to_command(std::string string, std::vector<uint32_t> &commands, uint16_t W4FLAGS_SHARED){
    if(!inited_o) throw std::logic_error("Output has to be inited with RPBBSerial::serial_init before calling RPBBSerial::string_to_command.");
    //std::cerr<<util::toString("sending command: ",string);
    commands.push_back(CQF::GPIO_MASK(_gpioN_o,_gpioP_o,0));
    for (int i=0;i!=string.length();i++){
        if(W4FLAGS_SHARED)
            if(i==string.length()-1)
                commands.push_back(CQF::W4TRIG_FLAGS_SHARED(1,false,W4FLAGS_SHARED,false));             // for adding wait for trigger before last char is sent

        commands.push_back(CQF::W4TRIG_MIN_IN_QUEUE(1+2+8*2+2));    // forces the queue to wait for the whole character to be inserted

        commands.push_back(CQF::GPIO_VAL (0,0,0));              // start bit
        commands.push_back(CQF::WAIT(_cyclesPerBit-1));
        for(int j=0;j!=8;j++){
            commands.push_back(CQF::GPIO_VAL ((((uint8_t)string[i])&(1<<j))?_gpioN_o:0,(((uint8_t)string[i])&(1<<j))?_gpioP_o:0,0));    // data bits
            commands.push_back(CQF::WAIT(_cyclesPerBit-1));
        }
        commands.push_back(CQF::GPIO_VAL (_gpioN_o,_gpioP_o,0));  // stop bits
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
            if(_gpioN_i) byte^=( AQF::getN(data[i*8+j])&_gpioN_i?(1<<j):0 );
            else        byte^=( AQF::getP(data[i*8+j])&_gpioP_i?(1<<j):0 );
        }
        string+=byte;
    }
}
