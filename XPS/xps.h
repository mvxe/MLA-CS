#ifndef XPS_H
#define XPS_H

#include "includes.h"
#include "TCP_con.h"

class XPS : public TCP_con{
    friend int main(int argc, char *argv[]);    //to be able to call run()
public:
    XPS();
    ~XPS();
    void addCommandToQueue(std::string command);        //adds the command to the queue to be executed (FIFO), this can be done even if the queue is executing, it simply adds one more to the FIFO
    void clearCommandQueue(void);                       //clears all the commands in the queue, can be done even if the queue is executing
    void execQueueStart(void);                          //starts the execution of the queue commands, when queue is empty it waits and immediately executes new commands, unless halted
    void execQueueHalt(void);                           //halts the queue command execution
    unsigned getCommandQueueSize(void);                 //gets the number of commands waiting to be executed

    void execCommandNow(std::string command);           //exectue a command now, implemented as a constatntly executing FIFO to ensure the execution order matches the command call order if called repeatedly

    //void execPVTQueue(void);                          //TODO implement PVT

private:
    void run();
    std::queue<std::string>     main_queue;
    std::queue<std::string> priority_queue;
    std::mutex mpq;
    bool _writef;
};

#endif // XPS_H
