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

        // ## lower level functions: ##

    int F2A_read(uint8_t queue, uint32_t *data, uint32_t size4);    //queue is 0-3; note: size4 is number of uint32_t (4 bytes each), not bytes; return 0 if success, else -1
    int A2F_write(uint8_t queue, uint32_t *data, uint32_t size4);   //queue is 0-3; note: size4 is number of uint32_t (4 bytes each), not bytes; return 0 if success, else -1
    enum getNumType{F2A_RSMax=0, F2A_RSCur=1, F2A_lostN=2, A2F_RSMax=3, A2F_RSCur=4, A2F_lostN=5};  //maximum number of elements, number of elemenst currently in queue, number of lost elements
    int getNum(getNumType statID, uint8_t queue);                   //queue is 0-3; for command see enum above; returns the result or -1 if error
    int A2F_trig(uint8_t queue);                                    //queue is 0-3
    int FIFOreset(uint8_t A2Fqueues, uint8_t F2Aqueues=0);          //queues is a 4bit binary value (0xF), where each queue is one bit: this lets one reset multiple queues silmultaneously
    int FIFOreset();                                                //total reset

    int PIDreset(uint8_t PIDN);                                     //PIDN is a 2bit binary value (0x3), where each PID is one bit: this resets only the internal PID values such a integrated sum
    int PIDreset();                                                 //total reset
    int A2F_loop(uint8_t queue, bool loop);                         //queue is 0-3


        // ## command queue class ##

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
    struct movEv{   // motion event
        std::string axisID;
        double displacement=0;                              // the relative motion dispacement, in mm (or radian if rotational)
        double position;                                    // the absolute motion position, in mm, (used if displacement==0)
        double velocity=0;                                  // velocity override if >0, otherwise default velocity will be used (note: overrides to not persist)
        double acceleration=0;                              // acceleration override if >0, otherwise default acceleration will be used
        bool optimize=true;                                 // used for some devices, see specific implementations
    };


        // ## higher level functions: ##

    void executeQCS(QCS& cq, bool force=false);             // if force==true and a queue is not empty but its emptyReq==true, reset that queue
                                                            //      else an error will be thrown
                                                            // this also clears QCS if executed
    void executeQueue(std::deque<uint32_t>& cq, uint8_t queue);     // execute only one queue

    // move commands: if cq is given append the commands to cq, otherwise just execute immediately (force=false)
    // commands go to main_command_queue, for others use the motion(std::deque<uint32_t>& cq...) overload
    // multiple axes can be addressed silmultaneously, whether the moves themselves are silmultaneous depends on the axes/controller/implementation
    void motion(QCS& cq, movEv mEv);
    void motion(std::deque<uint32_t>& cq, movEv mEv);
    void motion(movEv mEv);
    template <typename... Args>   void motion(QCS& cq, movEv mEv, Args&&... args);
    template <typename... Args>   void motion(std::deque<uint32_t>& cq, movEv mEv, Args&&... args);
    template <typename... Args>   void motion(movEv mEv, Args&&... args);

    enum mst{minPosition, maxPosition, homePosition, defaultVelocity, maximumVelocity, defaultAcceleration, maximumAcceleration};
    double getMotionSetting(std::string axisID, mst setting);

    void getCurrentPosition(std::string axisID, double& position);      // this will throw an exception if main_command_queue is in use
    template <typename... Args>   void getCurrentPosition(std::string axisID, double& position, Args&&... args);
    void getMotionError(std::string axisID, int& error);                // returns error code if the motion device supports it, 0=no erro, this will throw an exception if main_command_queue is in use
    template <typename... Args>   void getMotionError(std::string axisID, int& error, Args&&... args);

        // ## aditional higher level functions: ##

    // for compatibility you should not use these to control motion axes:
    void sendSerial(QCS& cq, std::string deviceID, std::string data);

private:
    void run();
};

#endif // RPTY_H
