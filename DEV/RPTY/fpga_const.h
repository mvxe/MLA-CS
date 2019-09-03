#include <stdint.h>
#ifndef FPGACONST_H
#define FPGACONST_H

class CQF{       //class containing functions that convert to command queue format for ARM2FPGA queues
public:
    // N/A   ( 0<<28)
    // OTHER ( 1<<28)
        // TRIG_OTHER   ( 0<<24)
        static inline uint32_t TRIG_OTHER (unsigned char command_queues);
        // ACK          ( 1<<24)
        enum ACK_CHANNELS { fADC_A__fADC_A=0,       //when doing this, left side(MSB) value is newer than the right side(LSB)
                            fADC_B__fADC_B=1,
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
                            generated_14bit=14,
                            generated_14bitX2=15};
        static inline uint32_t ACK (unsigned char acquisition_queues, unsigned char averaging, ACK_CHANNELS ACK_channels, bool setActive);
        // ??           ( 2<<24)
        // ??           ( 3<<24)
        // ??           ( 4<<24)
        // ??           ( 5<<24)
        // ??           ( 6<<24)
        // ??           ( 7<<24)
        // GPIO_MASK    ( 8<<24)
        static inline uint32_t GPIO_MASK (unsigned char N, unsigned char P, unsigned char LED);
        // GPIO_DIR     ( 9<<24)
        static inline uint32_t GPIO_DIR  (unsigned char N, unsigned char P, unsigned char LED);
        // GPIO_VAL     (10<<24)
        static inline uint32_t GPIO_VAL  (unsigned char N, unsigned char P, unsigned char LED);
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
        enum W4TRIG_GPIO_TYPE{LOW=0, HIGH=1, FALL=2, RISE=3};
        static inline uint32_t W4TRIG_GPIO (W4TRIG_GPIO_TYPE type, bool AND, unsigned char N, unsigned char P);
        // W4TRIG_ADC_S ( 2<<24)
        //static uint32_t W4TRIG_ADC_S ();  TODO
        // W4TRIG_ADC_F ( 3<<24)
        //static uint32_t W4TRIG_ADC_F ();  TODO
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
    enum W4TRIG_SGCHANNELS {O0td=4, O1td=5, O0O1=6, T0td=7, T1td=8, T0T1=9, O0T0=10, O0T1=11, O1T0=12, O1T1=13};
    static inline uint32_t SG_SAMPLE (W4TRIG_SGCHANNELS channels, int ch0val, int ch1val_or_time);
    //  ??  (14<<28)
    //  ??  (15<<28)

};



inline uint32_t CQF::TRIG_OTHER (unsigned char command_queues){
    return (uint32_t)( (1<<28)|(0<<24)|(command_queues&0xF) );
}       //command_queues: 4 bit value : each bit is one queue (allows one to trigger multiple queues silmultaneously)
inline uint32_t CQF::ACK (unsigned char acquisition_queues, unsigned char averaging, ACK_CHANNELS ACK_channels, bool setActive){
    return (uint32_t)( (1<<28)|(1<<24)|(((uint32_t)acquisition_queues&0xF)<<10)|(((uint32_t)averaging&0x1F)<<5)|(((uint32_t)ACK_channels)<<1)|(setActive?1:0) );
}       //acquisition_queues: 4 bit value : each bit is one queue (allows one to trigger multiple queues silmultaneously)
        //averaging: 5 bit value: 0-31 (averages 2^averaging number of samples)
        //ACK_channels see enums above
        //setActive: the acquisiton runs as long as setActive is true for that queue
inline uint32_t CQF::GPIO_MASK (unsigned char N, unsigned char P, unsigned char LED){
    return (uint32_t)( (1<<28)|(8<<24)|((uint32_t)N<<16)|((uint32_t)P<<8)|((uint32_t)LED) );
}       //parameters are binary representation of the ports, 8 bit each
inline uint32_t CQF::GPIO_DIR (unsigned char N, unsigned char P, unsigned char LED){
    return (uint32_t)( (1<<28)|(9<<24)|((uint32_t)N<<16)|((uint32_t)P<<8)|((uint32_t)LED) );
}       //parameters are binary representation of the ports, 8 bit each
inline uint32_t CQF::GPIO_VAL (unsigned char N, unsigned char P, unsigned char LED){
    return (uint32_t)( (1<<28)|(10<<24)|((uint32_t)N<<16)|((uint32_t)P<<8)|((uint32_t)LED) );
}       //parameters are binary representation of the ports, 8 bit each
inline uint32_t CQF::PWM (PWM_CHANNEL channel, uint16_t val){
    return (uint32_t)( (1<<28)|(((uint32_t)channel)<<24)|val );
}       //channel see enums above
        //val - 16 bit unsigned value for PWM
inline uint32_t CQF::W4TRIG_INTR (){
    return (uint32_t)( (2<<28)|(0<<24) );
}
inline uint32_t CQF::W4TRIG_GPIO (W4TRIG_GPIO_TYPE type, bool AND, unsigned char N, unsigned char P){
    return (uint32_t)( (2<<28)|(1<<24)|(((uint32_t)type)<<22)|((uint32_t)AND<<21)|((uint32_t)N<<8)|((uint32_t)P) );
}       //type see enums above
        //AND, if true, all pins must change as specified, if false, OR is used instead
        //N and P are 8 bit masks
inline uint32_t CQF::WAIT (unsigned time){
    return (uint32_t)( (3<<28)|time );
}       //time is a 28 bit unsigned number (num of clock cycles to wait), NOTE 0 does the same as 1
inline uint32_t CQF::SG_SAMPLE (W4TRIG_SGCHANNELS channels, int ch0val, int ch1val_or_time){
    return (uint32_t)( (((uint32_t)channels)<<28)|((ch0val&0x3FFF)<<14)|(ch1val_or_time&0x3FFF) );
}       //channels see enums above
        //ch0val value of the first channel, 14 bit signed value (NOTE: the program does not handle overlow)
        //ch1val value of the second channel, 14 bit signed value or time 14 bit unsigned delay, NOTE 0 does the same as 1

class AQF{       //class containing functions that convert from acquisition queue format for FPGA2ARM queues
public:
    static inline int32_t getChMSB(uint32_t acq);     //example: gets the fADC_A from fADC_A__fADC_B
    static inline int32_t getChLSB(uint32_t acq);     //example: gets the fADC_B from fADC_A__fADC_B
};

inline int32_t AQF::getChMSB(uint32_t acq){
    int ret = (acq&0x0FFFC000)>>14;
    if (ret&0x2000) {ret^=0x2000; ret^=0xFFFFE000;}
    return ret;
}
inline int32_t AQF::getChLSB(uint32_t acq){
    int ret = (acq&0x00003FFF);
    if (ret&0x2000) {ret^=0x2000; ret^=0xFFFFE000;}
    return ret;
}

#endif //FPGACONST_H
