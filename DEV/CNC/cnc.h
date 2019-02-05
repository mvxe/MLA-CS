#ifndef CNC_H
#define CNC_H

#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <array>
#include <atomic>

#include "serial/serial.h"
#include "UTIL/containers.h"
#include "globals.h"

class CNC : public protooth{
public:
    CNC();
    ~CNC();
    tsvar_save<std::string> selected_ID;                    //thread safe access to select serial ID
    const std::atomic<bool>& connected{_connected};         //thread safe access to camera status
    std::atomic<bool> checkID{true};
    std::atomic<bool> refreshID{true};


    struct _dserial{
        std::string ID;
        std::string port;
        std::string description;
    };
    tsvar<std::vector<_dserial>*> serial_desc{&mkmx,nullptr};


    template <typename... Args>   void execCommand(Args... args);                   //exectue a command without return, implemented as a constatntly executing FIFO to ensure the execution order matches the command call order if called repeatedly
    template <typename... Args>   void execCommand(exec_ret* ret, Args... args);    //exectue a command with return, before calling this command create a xps_ret object and pass its pointer as the first argument. For readout, see exec_ret in UTIL/containers   //UNTESTED

private:
    std::string ID{"none"};
    void run();
    void cend();
    void flushQueue();

    struct execss{
        std::string comm;       //contains the command string to be executed
        exec_ret* ret;          //see UTIL/containers for available functions, set to nullptr if no return is desired
    };
    std::queue<execss> priority_queue;
    std::mutex mpq;

        std::mutex mkmx;
    std::atomic<bool> _connected{false};
    serial::Serial* sercon{nullptr};
    const int baud{57600};
    const int timeout{1}; //in ms
    std::string port;
};

#include "cnc_template.h"

#endif // CNC_H
