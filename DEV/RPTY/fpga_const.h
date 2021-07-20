#include <stdint.h>
#ifndef FPGACONST_H
#define FPGACONST_H

class CQF{       //class containing functions that convert to command queue format for ARM2FPGA queues
public:
    // N/A   ( 0<<28)
    // OTHER ( 1<<28)
        // TRIG_OTHER   ( 0<<24)
        static inline uint32_t TRIG_OTHER (uint8_t command_queues);
        // ACK          ( 1<<24)
        enum ACK_CHANNELS { fADC_A__fADC_A=0,       //see AQF class
                            fADC_B__fADC_B=1,       //when doing this, left side(MSB) value is newer than the right side(LSB)
                            PIDO_0__PIDO_0=2,
                            PIDO_1__PIDO_1=3,
                            sADC_0__sADC_0=4,
                            sADC_1__sADC_1=5,
                            sADC_2__sADC_2=6,
                            sADC_3__sADC_3=7,
                            fADC_A__fADC_B=8,
                            PIDO_0__PIDO_1=9,       //here PIDO_0 is in the left side(MSB), and PIDO_1 is in the right side(LSB)
                            fADC_A__PIDO_0=10,
                            fADC_A__PIDO_1=11,
                            fADC_B__PIDO_0=12,
                            fADC_B__PIDO_1=13,
                            gpioP_gpioN=14,         // gpioP is MSB (b21-b14), gpioN is LSB (b7-b0) (other bits will be 0, so you may use )
                            generated_14bitX2=15};
        static inline uint32_t ACK (uint8_t acquisition_queues, uint8_t averaging, ACK_CHANNELS ACK_channels, bool setActive);
        // flags mask   ( 2<<24)
        static inline uint32_t FLAGS_MASK (uint16_t mask);
        // flags local set ( 3<<24)
        static inline uint32_t FLAGS_LOCAL_SET (uint16_t value);
        // flags shared set ( 4<<24)
        static inline uint32_t FLAGS_SHARED_SET (uint16_t value);
        // ??           ( 5<<24)
        // ??           ( 6<<24)
        // ??           ( 7<<24)
        // GPIO_MASK    ( 8<<24)
        static inline uint32_t GPIO_MASK (uint8_t N, uint8_t P, uint8_t LED);
        // GPIO_DIR     ( 9<<24)
        static inline uint32_t GPIO_DIR  (uint8_t N, uint8_t P, uint8_t LED);
        // GPIO_VAL     (10<<24)
        static inline uint32_t GPIO_VAL  (uint8_t N, uint8_t P, uint8_t LED);
        // PWMA         (11<<24)
        // PWMB         (12<<24)
        // PWMC         (13<<24)
        // PWMD         (14<<24)
        enum PWM_CHANNEL {PWMA=11, PWMB=12, PWMC=13, PWMD=14};
        static inline uint32_t PWM (PWM_CHANNEL channel, uint16_t val);
        // ??           (15<<24)
    // WAIT_4_TRIG ( 2<<28)
        // W4TRIG_INTR ( 0<<24)
        static inline uint32_t W4TRIG_INTR ();     //from ARM or other queue
        // W4TRIG_GPIO  ( 1<<24)
        static const bool HIGH=1;
        static const bool LOW=0;
        static inline uint32_t W4TRIG_GPIO (bool level, bool AND, uint8_t N, uint8_t P, bool getTrigTime=false);
        static inline uint32_t W4TRIG_FLAGS_LOCAL (bool level, bool AND, uint16_t mask, bool getTrigTime=false);
        static inline uint32_t W4TRIG_FLAGS_SHARED (bool level, bool AND, uint16_t mask, bool getTrigTime=false);
        // W4TRIG_ADC_S ( 2<<24)
        //static uint32_t W4TRIG_ADC_S ();  TODO
        // W4TRIG_ADC_F ( 3<<24)
        //static uint32_t W4TRIG_ADC_F ();  TODO
        static inline uint32_t IF_GPIO (bool level, bool AND, uint8_t N, uint8_t P, bool getTrigTime=false);
        static inline uint32_t IF_FLAGS_LOCAL (bool level, bool AND, uint16_t mask, bool getTrigTime=false);
        static inline uint32_t IF_FLAGS_SHARED (bool level, bool AND, uint16_t mask, bool getTrigTime=false);
        static inline uint32_t END ();
    // WAIT ( 3<<28)
    static inline uint32_t WAIT (unsigned time);
    // O0td ( 4<<28)
    // O1td ( 5<<28)
    // O0O1 ( 6<<28)
    // T0td ( 7<<28)
    // T1td ( 8<<28)
    // T0T1 ( 9<<28)
    // O0T0 (10<<28)
    // O0T1 (11<<28)
    // O1T0 (12<<28)
    // O1T1 (13<<28)
    enum SGCHANNELS {O0td=4, O1td=5, O0O1=6, T0td=7, T1td=8, T0T1=9, O0T0=10, O0T1=11, O1T0=12, O1T1=13};
    static inline uint32_t SG_SAMPLE (SGCHANNELS channels, int ch0val, int ch1val_or_time);
    //  ??  (14<<28)
    //  ??  (15<<28)

};



inline uint32_t CQF::TRIG_OTHER (uint8_t command_queues){
    return (uint32_t)( (1<<28)|(0<<24)|(command_queues&0xF) );
}       //command_queues: 4 bit value : each bit is one queue (allows one to trigger multiple queues silmultaneously)
inline uint32_t CQF::ACK (uint8_t acquisition_queues, uint8_t averaging, ACK_CHANNELS ACK_channels, bool setActive){
    return (uint32_t)( (1<<28)|(1<<24)|(((uint32_t)acquisition_queues&0xF)<<10)|(((uint32_t)averaging&0x1F)<<5)|(((uint32_t)ACK_channels)<<1)|(setActive?1:0) );
}       //acquisition_queues: 4 bit value : each bit is one queue (allows one to trigger multiple queues silmultaneously)
        //averaging: 5 bit value: 0-31 (averages 2^averaging number of samples)
        //  averaging is ignored for gpioP_gpioN
        //ACK_channels see enums above
        //setActive: the acquisiton runs as long as setActive is true for that queue
inline uint32_t FLAGS_MASK (uint16_t mask){
    return (uint32_t)( (1<<28)|(2<<24)|((uint32_t)mask) );
}
inline uint32_t FLAGS_LOCAL_SET (uint16_t value){
    return (uint32_t)( (1<<28)|(3<<24)|((uint32_t)value) );
}
inline uint32_t FLAGS_SHARED_SET (uint16_t value){
    return (uint32_t)( (1<<28)|(4<<24)|((uint32_t)value) );
}
inline uint32_t CQF::GPIO_MASK (uint8_t N, uint8_t P, uint8_t LED){
    return (uint32_t)( (1<<28)|(8<<24)|((uint32_t)N<<16)|((uint32_t)P<<8)|((uint32_t)LED) );
}       //parameters are binary representation of the ports, 8 bit each
inline uint32_t CQF::GPIO_DIR (uint8_t N, uint8_t P, uint8_t LED){
    return (uint32_t)( (1<<28)|(9<<24)|((uint32_t)N<<16)|((uint32_t)P<<8)|((uint32_t)LED) );
}       //parameters are binary representation of the ports, 8 bit each
        //NOTE: low = output, high = input ! By default (after reset) GPIOs will be inputs, LEDs will be outputs
inline uint32_t CQF::GPIO_VAL (uint8_t N, uint8_t P, uint8_t LED){
    return (uint32_t)( (1<<28)|(10<<24)|((uint32_t)N<<16)|((uint32_t)P<<8)|((uint32_t)LED) );
}       //parameters are binary representation of the ports, 8 bit each
inline uint32_t CQF::PWM (PWM_CHANNEL channel, uint16_t val){
    return (uint32_t)( (1<<28)|(((uint32_t)channel)<<24)|val );
}       //channel see enums above
        //val - 16 bit unsigned value for PWM
inline uint32_t CQF::W4TRIG_INTR (){
    return (uint32_t)( (2<<28)|(0<<24) );
}
inline uint32_t CQF::W4TRIG_GPIO (bool level, bool AND, uint8_t N, uint8_t P, bool getTrigTime){
    return (uint32_t)( (2<<28)|(1<<24)|(((uint32_t)level)<<22)|((uint32_t)AND<<21)|((uint32_t)getTrigTime<<20)|((uint32_t)N<<8)|((uint32_t)P) );
}       //level is LOW(0) or HIGH(1)
        //AND, if true, all pins must change as specified, if false, OR is used instead
        //N and P are 8 bit masks
        //getTrigTime if true, the queue will await a CQF::WAIT command, and the gpio has to continuously be in the specified state for this long to trigger
        //  this could be useful against noise. Note that for rise/fall triggers, the trigger changes into high/low after the first transition.
        //  commands other than CQF::WAIT will be ignored
inline uint32_t CQF::W4TRIG_FLAGS_LOCAL (bool level, bool AND, uint16_t mask, bool getTrigTime){
    return (uint32_t)( (2<<28)|(4<<24)|(((uint32_t)level)<<22)|((uint32_t)AND<<21)|((uint32_t)getTrigTime<<20)|((uint32_t)mask) );
}
inline uint32_t CQF::W4TRIG_FLAGS_SHARED (bool level, bool AND, uint16_t mask, bool getTrigTime){
    return (uint32_t)( (2<<28)|(5<<24)|(((uint32_t)level)<<22)|((uint32_t)AND<<21)|((uint32_t)getTrigTime<<20)|((uint32_t)mask) );
}
inline uint32_t CQF::IF_GPIO (bool level, bool AND, uint8_t N, uint8_t P, bool getTrigTime){
    return (uint32_t)( (14<<28)|(1<<24)|(((uint32_t)level)<<22)|((uint32_t)AND<<21)|((uint32_t)getTrigTime<<20)|((uint32_t)N<<8)|((uint32_t)P) );
}       //same as above, except nonblocking (if condition is false, it skips everything up to the assocciated END, nesting is possible (up to 256))
inline uint32_t CQF::IF_FLAGS_LOCAL (bool level, bool AND, uint16_t mask, bool getTrigTime){
    return (uint32_t)( (14<<28)|(4<<24)|(((uint32_t)level)<<22)|((uint32_t)AND<<21)|((uint32_t)getTrigTime<<20)|((uint32_t)mask) );
}
inline uint32_t CQF::IF_FLAGS_SHARED (bool level, bool AND, uint16_t mask, bool getTrigTime){
    return (uint32_t)( (14<<28)|(5<<24)|(((uint32_t)level)<<22)|((uint32_t)AND<<21)|((uint32_t)getTrigTime<<20)|((uint32_t)mask) );
}
inline uint32_t CQF::END (){
    return (uint32_t)(15<<28);
}       //extra ENDs are ignored
inline uint32_t CQF::WAIT (unsigned time){
    return (uint32_t)( (3<<28)|time );
}       //time is a 28 bit unsigned number (num of clock cycles to wait), NOTE 0 does the same as 1
inline uint32_t CQF::SG_SAMPLE (SGCHANNELS channels, int ch0val, int ch1val_or_time){
    return (uint32_t)( (((uint32_t)channels)<<28)|((ch0val&0x3FFF)<<14)|(ch1val_or_time&0x3FFF) );
}       //channels see enums above
        //ch0val value of the first channel, 14 bit signed value (NOTE: the program does not handle overlow)
        //ch1val value of the second channel, 14 bit signed value or time 14 bit unsigned delay, NOTE 0 does the same as 1

class AQF{       //class containing functions that convert from acquisition queue format for FPGA2ARM queues
public:
    static inline int16_t getChMSB(uint32_t acq);     //example: gets the fADC_A from fADC_A__fADC_B
    static inline int16_t getChLSB(uint32_t acq);     //example: gets the fADC_B from fADC_A__fADC_B
    static inline uint8_t getN(uint32_t acq);                //use with gpioP_gpioN
    static inline uint8_t getP(uint32_t acq);                //-||-
};

inline int16_t AQF::getChMSB(uint32_t acq){
    int ret = (acq&0x0FFFC000)>>14;
    if (ret&0x2000) {ret^=0x2000; ret^=0xFFFFE000;}
    return ret;
}
inline int16_t AQF::getChLSB(uint32_t acq){
    int ret = (acq&0x00003FFF);
    if (ret&0x2000) {ret^=0x2000; ret^=0xFFFFE000;}
    return ret;
}
inline uint8_t AQF::getN(uint32_t acq){
    return (acq&0x000000FF);
}
inline uint8_t AQF::getP(uint32_t acq){
    return ((acq&0x003FC000)>>14);
}

#endif //FPGACONST_H
