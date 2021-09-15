#ifndef RPTY_H
#define RPTY_H

#include "DEV/TCP_con.h"

#include "globals.h"
#include "UTIL/containers.h"
#include "fpga_const.h"
#include "rpbbserial.h"
#include "rpmotion.h"
#include "DEV/controller.h"

class RPTY : public TCP_con, public protooth, public CTRL{
public:
    RPTY();
    ~RPTY();

        // ## lower level functions: - thread safe##

// TODO make private

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


        // higher level for internal use

    typedef std::vector<uint32_t> cqueue;
    void executeQueue(cqueue& cq, uint8_t queue);


    // following functions are standatd CTRL interface; see CTRL class for more details

public:

        // #### device initialization ####

    void registerDevice(std::string ID, devType type);
    void initDevices();
    void deinitDevices();
    void referenceMotionDevices();
    std::vector<std::string> getMotionDevices();
    std::vector<std::string> getDevices();

        // #### higher level functions: ####

    double getMotionSetting(std::string ID, mst setting);
    void motion(std::string ID, double position, double velocity=0, double acceleration=0, motionFlags flags=0);
    int getError(std::string ID);
    void setGPIO(std::string ID, bool state);
    void pulseGPIO(std::string ID, double duration);

private:
    void CO_init(CO* a);
    void CO_delete(CO* a);
    void CO_execute(CO* a);
    void CO_addMotion(CO* a, std::string ID, double position, double velocity=0, double acceleration=0, motionFlags flags=0);
    void CO_addDelay(CO* a, double delay);
    void CO_setGPIO(CO* a, std::string ID, bool state);
    void CO_pulseGPIO(CO* a, std::string ID, double duration);
    void CO_addHold(CO* a, std::string ID, _holdCondition condition);
    void CO_startTimer(CO* a, std::string ID, double duration);
    void CO_clear(CO* a);

    struct cqus{
        cqueue main;
        cqueue timer;
    };
    std::map<CO*, cqus> commandObjects;

private:
    unsigned _free_flag;
    inline void _motionDeviceThrowExc(std::string axisID, std::string function);
    void setMotionDeviceType(std::string axisID);
    void _addHold(cqueue& cq, cqueue& cqhold);
    void run();
    std::recursive_mutex mux;
    std::atomic<bool> recheck_position{true};


private:
    std::mutex smx; // TODO remove this
public:
    // TODO change these:
    tsvar_ip IP{&smx, "192.168.1.2"};
    tsvar_port port{&smx, 32};
    tsvar<unsigned> keepalive{&smx, 500};                   //keepalive and connect timeout, in ms

    unsigned main_cq{0};                        // main command queue
    unsigned helper_cq{1};
    unsigned serial_cq{2};                      // serial command queue (for acquisition)
    unsigned timer_cq{3};
    unsigned main_aq{0};                        // main acquisition queue
    unsigned serial_aq{1};                      // serial acquisition queue


    class motionAxis{       // devices of type dt_motion
    public:
        std::string type{"md_none"};
        rtoml::vsr conf;
        rpMotionDevice* dev{nullptr};
    };
    class gpioDevice{       // devices of type dt_gpio
    public:
        gpioDevice();
        void initGPIO(cqueue& cq);
        void setGPIO(cqueue& cq, bool state);
        void pulseGPIO(cqueue& cq, double duration);
        void w4trig(cqueue& cq, bool state);
        int gpio;
        uint8_t gpioN, gpioP, gpioLED;
        bool isInput{false};
        bool defaultState{false};
        bool inverted{false};
        rtoml::vsr conf;
    };
    class timerDevice{      // devices of type dt_timer
    public:
        void initTimer(unsigned& free_flag);
        void addTimer(cqueue& cq, cqueue& cq_timer, double duration);
        void holdTimer(cqueue& cq);
        uint16_t timerFlag;
    };

    bool devicesInited=false;
    std::map<std::string, motionAxis> motionAxes;       // devices of type dt_motion
    std::map<std::string, gpioDevice> gpioDevices;      // devices of type dt_gpio
    std::map<std::string, timerDevice> timerDevices;    // devices of type dt_timer



};


#endif // RPTY_H
