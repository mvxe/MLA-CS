#ifndef XPS_H
#define XPS_H

#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include "mutex_containers.h"
#include "TCP_con.h"
#include "sharedvars.h"
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#define TRAJ_PATH "/public/Trajectories/"

class XPS : public TCP_con{
    friend class globals;    //to be able to call run()
public:
    XPS();
    ~XPS();

    template <typename T>
    void execCommand(T value);
    template<typename T, typename... Args>
    void execCommand(T value, Args... args);            //exectue a command (nonblocking) without return, implemented as a constatntly executing FIFO to ensure the execution order matches the command call order if called repeatedly
    template <typename T>
    std::string execCommandR(T value);
    template<typename T, typename... Args>
    std::string execCommandR(T value, Args... args);    //exectue a command (blocking) with return, waits for its turn in the FIFO
    void initGroups();
    void homeGroups();
    void killGroups();

    void addToPVTqueue(std::string str);
    void clearPVTqueue();
    std::string copyPVToverFTP(std::string name);
    void execPVTQueue(std::string name);
    std::string listPVTfiles();

    void XYZMoveRelative(double dX,double dY, double dZ, bool limit=true);
    void XYZMoveAbsolute(double X, double Y, double Z, bool limit=true);


private:
    template <typename T>
    void eexecCommand(std::stringstream* strm, T value);
    template<typename T, typename... Args>
    void eexecCommand(std::stringstream* strm, T value, Args... args);
    template <typename T>
    std::string eexecCommandR(std::stringstream* strm, T value);
    template<typename T, typename... Args>
    std::string eexecCommandR(std::stringstream* strm, T value, Args... args);
    void flushQueue();
    void _XYZMoveAbsolute(bool limit=true);

    void run();
    struct execss{
        std::string comm;
        std::string* ret;
    };
    std::queue<execss> priority_queue;
    std::mutex mpq;
    bool _writef;

    std::mutex ftpmx;
    std::string upPVTfile;
};


/*##### Template functions #####*/
template <typename T>
void XPS::execCommand(T value){
    std::stringstream* nstrm=new std::stringstream();
    eexecCommand(nstrm, value);
}
template<typename T, typename... Args>
void XPS::execCommand(T value, Args... args){
    std::stringstream* nstrm=new std::stringstream();
    eexecCommand(nstrm, value, args...);
}
template <typename T>
void XPS::eexecCommand(std::stringstream* strm, T value){
    *strm<<value;
    mpq.lock();
    priority_queue.push({strm->str(),nullptr});
    mpq.unlock();
    strm->str("");
    strm->clear();
    delete strm;
}
template<typename T, typename... Args>
void XPS::eexecCommand(std::stringstream* strm, T value, Args... args){
    *strm<<value;
    eexecCommand(strm, args...);
}

template <typename T>
std::string XPS::execCommandR(T value){
    std::stringstream* nstrm=new std::stringstream();
    return eexecCommandR(nstrm, value);
}
template<typename T, typename... Args>
std::string XPS::execCommandR(T value, Args... args){
    std::stringstream* nstrm=new std::stringstream();
    return eexecCommandR(nstrm, value, args...);
}
template <typename T>
std::string XPS::eexecCommandR(std::stringstream* strm, T value){
    *strm<<value;
    std::string ret;
    mpq.lock();
    priority_queue.push({strm->str(),&ret});
    mpq.unlock();
    strm->str("");
    strm->clear();
    delete strm;
    for(bool done=false;done==false;){
        std::this_thread::sleep_for (std::chrono::milliseconds(1));
        mpq.lock();
        if(!ret.empty()) done=true;
        mpq.unlock();
    }
    return ret;
}
template<typename T, typename... Args>
std::string XPS::eexecCommandR(std::stringstream* strm, T value, Args... args){
    *strm<<value;
    return eexecCommandR(strm, args...);
}

#endif // XPS_H
