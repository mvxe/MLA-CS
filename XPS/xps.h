#ifndef XPS_H
#define XPS_H

#include "includes.h"
#include "mutex_containers.h"
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

    template <typename T>
    void execCommandNow(T value);
    template<typename T, typename... Args>
    void execCommandNow(T value, Args... args);         //exectue a command now, implemented as a constatntly executing FIFO to ensure the execution order matches the command call order if called repeatedly

    void initGroups();
    void homeGroups();
    void killGroups();

    //void execPVTQueue(void);                          //TODO implement PVT

private:
    template <typename T>
    void execCommandNow(std::stringstream* strm, T value);
    template<typename T, typename... Args>
    void execCommandNow(std::stringstream* strm, T value, Args... args);

    void run();
    std::queue<std::string>     main_queue;
    std::queue<std::string> priority_queue;
    std::mutex mpq;
    bool _writef;
};


/*##### Template functions #####*/
template <typename T>
void XPS::execCommandNow(T value){
    std::stringstream* nstrm=new std::stringstream();
    execCommandNow(nstrm, value);
}
template<typename T, typename... Args>
void XPS::execCommandNow(T value, Args... args){
    std::stringstream* nstrm=new std::stringstream();
    execCommandNow(nstrm, value, args...);
}
template <typename T>
void XPS::execCommandNow(std::stringstream* strm, T value){
    *strm<<value;
    mpq.lock();
    priority_queue.push(strm->str());
    mpq.unlock();
    std::cerr<<"\nexecuting command:\n"<<strm->str()<<"\n";
    strm->str("");
    strm->clear();
    delete strm;
}
template<typename T, typename... Args>
void XPS::execCommandNow(std::stringstream* strm, T value, Args... args){
    *strm<<value;
    execCommandNow(strm, args...);
}

#endif // XPS_H
