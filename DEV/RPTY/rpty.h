#ifndef RPTY_H
#define RPTY_H

#include "DEV/TCP_con.h"

#include "UTIL/containers.h"
#include "_config.h"
#include "fpga_const.h"

class RPTY : public TCP_con, public rpty_config, public protooth{
public:
    RPTY();
    ~RPTY();
    int F2A_read(unsigned char queue, uint32_t *data, uint32_t size4);  //queue is 0-3; note: size4 is number of uint32_t (4 bytes each), not bytes; return 0 if success, else -1
    int A2F_write(unsigned char queue, uint32_t *data, uint32_t size4); //queue is 0-3; note: size4 is number of uint32_t (4 bytes each), not bytes; return 0 if success, else -1
    enum getNumType{F2A_RSMax=0, F2A_RSCur=1, F2A_lostN=2, A2F_RSMax=3, A2F_RSCur=4, A2F_lostN=5};  //maximum number of elements, number of elemenst currently in queue, number of lost elements
    int getNum(getNumType statID, unsigned char queue);                 //queue is 0-3; for command see enum above; returns the result or -1 if error
    int trig(unsigned char queue);                                      //queue is 0-3
    int FIFOreset(unsigned char A2Fqueues, unsigned char F2Aqueues=0);  //queues is a 4bit binary value (0xF), where each queue is one bit: this lets one reset multiple queues silmultaneously
    int FIFOreset();                                                    //total reset

    int PIDreset(unsigned char PIDN);                                   //PIDN is a 2bit binary value (0x3), where each PID is one bit: this resets only the internal PID values such a integrated sum
    int PIDreset();                                                     //total reset
    int A2F_loop(unsigned char queue, bool loop);                       //queue is 0-3
private:
    void run();
};

#endif // RPTY_H
