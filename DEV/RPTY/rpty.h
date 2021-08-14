#ifndef RPTY_H
#define RPTY_H

#include "DEV/TCP_con.h"

#include "UTIL/containers.h"
#include "_config.h"
#include "fpga_const.h"
#include "rpbbserial.h"
#include "rpmotion.h"

using namespace nRPTY;

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



        // ## higher level functions: ##

    void executeQCS(QCS& cq, bool force=false);             // if force==true and a queue is not empty but its emptyReq==true, reset that queue
                                                            //      else an error will be thrown
                                                            // this also clears QCS if executed
    void executeQueue(std::vector<uint32_t>& cq, uint8_t queue);     // execute only one queue, and clears it

    // move commands: if cq is given append the commands to cq, otherwise just execute immediately (force=false)
    // commands go to main_command_queue, for others use the motion(std::vector<uint32_t>& cq...) overload
    // multiple axes can be addressed silmultaneously, whether the moves themselves are silmultaneous depends on the axes/controller/implementation
    void motion(QCS& cq, movEv mEv);
    void motion(std::vector<uint32_t>& cq, movEv mEv);
    void motion(movEv mEv);
    template <typename... Args>   void motion(QCS& cq, movEv mEv, Args&&... args);
    template <typename... Args>   void motion(std::vector<uint32_t>& cq, movEv mEv, Args&&... args);
    template <typename... Args>   void motion(movEv mEv, Args&&... args);

    double getMotionSetting(std::string axisID, mst setting);

    void getCurrentPosition(std::string axisID, double& position);      // this will throw an exception if main_command_queue is in use
    template <typename... Args>   void getCurrentPosition(std::string axisID, double& position, Args&&... args);
    void getMotionError(std::string axisID, int& error);                // returns error code if the motion device supports it, 0=no erro, this will throw an exception if main_command_queue is in use
    template <typename... Args>   void getMotionError(std::string axisID, int& error, Args&&... args);

        // ## aditional higher level functions: ##

    // OPERATING PROCEDURE:
    //      1. add devices
    //      2. change settings via either GUI or TOML
    //      3. init devices
    //      4. use
    //      5. uninit devices

        // ## device initialization ##          // just name them, the devices themselves are configured via the GUI/TOML configuration
    void addMotionDevice(std::string axisID);
    void setMotionDeviceType(std::string axisID, std::string type);     // useful for GUI config

    void initMotionDevice(std::string axisID);
    void initMotionDevices();                   // inits all non-inited devices
    void deinitMotionDevice(std::string deviceID);
    void deinitMotionDevices();                 // inits all non-inited devices

private:
    inline void _motionDeviceThrowExc(std::string axisID, std::string function);
    void run();
};


#endif // RPTY_H
