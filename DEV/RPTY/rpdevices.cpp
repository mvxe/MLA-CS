#include "rpty.h"

    // #### gpioDevice ####

RPTY::gpioDevice::gpioDevice(){
    conf["gpio_pin"]=gpio;
    conf["gpio_pin"].comments.push_back("N (0-7), P (8-15), LED(16-23)");
    conf["isInput"]=isInput;
    conf["direction"].comments.push_back("If true, gpio configured as input.");
    conf["inverted"]=inverted;
    conf["inverted"].comments.push_back("If true and output, the gpio will be high by default (even after deinit), and pulse will pull it low. Also inverts state in setGPIO. Does not afect input.");
};
void RPTY::gpioDevice::initGPIO(cqueue& cq){
    if(gpio>23) throw std::invalid_argument(util::toString("In RPTY::gpioDevice::initGPIO: no such gpio pin: ",gpio));
    if(gpio>15 && isInput) throw std::invalid_argument(util::toString("In RPTY::gpioDevice::initGPIO: LED gpios cannot be configured as input."));
    gpioN=(gpio<=7)?(1<<gpio):0;
    gpioP=(gpio>7 && gpio<=15)?(1<<(gpio-8)):0;
    gpioLED=(gpio>15 && gpio<=23)?(1<<(gpio-16)):0;
    uint8_t tmp=isInput?0xFF:0x00;
    cq.push_back(CQF::GPIO_MASK(gpioN,gpioP,gpioLED));
    cq.push_back(CQF::GPIO_DIR (tmp,tmp,tmp));                  // 0 is output
    tmp=defaultState?0xFF:0x00;
    if(!isInput)
        cq.push_back(CQF::GPIO_VAL (tmp,tmp,tmp));
}
void RPTY::gpioDevice::setGPIO(cqueue& cq, bool state){
    cq.push_back(CQF::GPIO_MASK(gpioN,gpioP,gpioLED));
    uint8_t tmp=(state^inverted)?0xFF:0x00;
    cq.push_back(CQF::GPIO_VAL (tmp,tmp,tmp));
}
void RPTY::gpioDevice::pulseGPIO(cqueue& cq, double duration){
    cq.push_back(CQF::GPIO_MASK(gpioN,gpioP,gpioLED));
    uint8_t tmp=(inverted)?0x00:0xFF;
    cq.push_back(CQF::GPIO_VAL (tmp,tmp,tmp));
    long int cycles=duration*125000000-1;
    while(cycles>125000000){
        cq.push_back(CQF::WAIT(125000000));
        cycles-=125000000;
    }
    cq.push_back(CQF::WAIT(cycles>=0?cycles:0));
    cq.push_back(CQF::GPIO_VAL (~tmp,~tmp,~tmp));
}
void RPTY::gpioDevice::w4trig(cqueue& cq, bool state){
    cq.push_back(CQF::W4TRIG_GPIO(state,false,gpioN,gpioP,false));
}


    // #### timerDevice ####

void RPTY::timerDevice::initTimer(unsigned& free_flag){
    timerFlag=free_flag;
    free_flag<<=1;
}
void RPTY::timerDevice::addTimer(cqueue& cq, cqueue& cq_timer, double duration){
    cq_timer.push_back(CQF::FLAGS_MASK(timerFlag));
    cq_timer.push_back(CQF::W4TRIG_FLAGS_SHARED(true,false,timerFlag,false));
    long int cycles=duration*125000000-4;
    while(cycles>125000000){
        cq.push_back(CQF::WAIT(125000000));
        cycles-=125000000;
    }
    cq_timer.push_back(CQF::WAIT(cycles>=0?cycles:0));
    cq_timer.push_back(CQF::FLAGS_SHARED_SET(0x0000));
    cq.push_back(CQF::FLAGS_MASK(timerFlag));
    cq.push_back(CQF::FLAGS_SHARED_SET(timerFlag));
}
void RPTY::timerDevice::holdTimer(cqueue& cq){
    cq.push_back(CQF::W4TRIG_FLAGS_SHARED(false,false,timerFlag,false));
}
