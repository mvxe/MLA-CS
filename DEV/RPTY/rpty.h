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

    // lower level functions:

    int F2A_read(unsigned char queue, uint32_t *data, uint32_t size4);  //queue is 0-3; note: size4 is number of uint32_t (4 bytes each), not bytes; return 0 if success, else -1
    int A2F_write(unsigned char queue, uint32_t *data, uint32_t size4); //queue is 0-3; note: size4 is number of uint32_t (4 bytes each), not bytes; return 0 if success, else -1
    enum getNumType{F2A_RSMax=0, F2A_RSCur=1, F2A_lostN=2, A2F_RSMax=3, A2F_RSCur=4, A2F_lostN=5};  //maximum number of elements, number of elemenst currently in queue, number of lost elements
    int getNum(getNumType statID, unsigned char queue);                 //queue is 0-3; for command see enum above; returns the result or -1 if error
    int A2F_trig(unsigned char queue);                                      //queue is 0-3
    int FIFOreset(unsigned char A2Fqueues, unsigned char F2Aqueues=0);  //queues is a 4bit binary value (0xF), where each queue is one bit: this lets one reset multiple queues silmultaneously
    int FIFOreset();                                                    //total reset

    int PIDreset(unsigned char PIDN);                                   //PIDN is a 2bit binary value (0x3), where each PID is one bit: this resets only the internal PID values such a integrated sum
    int PIDreset();                                                     //total reset
    int A2F_loop(unsigned char queue, bool loop);                       //queue is 0-3

    // higher level functions:
    static const unsigned commandQueueNum{4};
    class QCS{  // queue command sets
        std::deque<uint32_t> commands[commandQueueNum];
        bool empty(){
            for (auto& cq : commands) if(!cq.empty()) return false;
            return true;
        }
        void clear(){
            for (auto& cq : commands) cq.clear();
            for (auto& fl : makeLoop) fl=false;
            for (auto& fl : makeTrig) fl=false;
            for (auto& fl : emptyReq) fl=false;
        }
        bool makeLoop[commandQueueNum]{};                   // set command queue to loop on exec (default all false)
        bool makeTrig[commandQueueNum]{};                   // trig command queue on exec (default all false)
        bool emptyReq[commandQueueNum]{};                   // the queue should be empty on exec (if not exec should throw an error) (default all false)
    };
    void executeQCS(QCS& cq, bool force=false);              // if force and a queue is not empty but its emptyReq==true, reset that queue
                                                            //      else an error will be thrown
                                                            // this also clears QCS if executed
    // TODO: implement rpty TRIG on queue having N commands or more - to get rid of makeTrig

    // move commands: if cq is given append the commands to cq, otherwise just execute (force=false)
    void moveAbsolute(QCS& cq);
    void moveAbsolute();
    void moveRelative(QCS& cq);
    void moveRelative();


private:
    void run();
};

#endif // RPTY_H
