#ifndef XPS_H
#define XPS_H

#include "includes.h"
#include "mutex_containers.h"
#include "TCP_con.h"
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#define TRAJ_PATH "/public/Trajectories/"

class XPS : public TCP_con{
    friend int main(int argc, char *argv[]);    //to be able to call run()
public:
    XPS();
    ~XPS();

    template <typename T>
    void execCommandNow(T value);
    template<typename T, typename... Args>
    void execCommandNow(T value, Args... args);         //exectue a command now, implemented as a constatntly executing FIFO to ensure the execution order matches the command call order if called repeatedly

    void initGroups();
    void homeGroups();
    void killGroups();

    void addToPVTqueue(std::string str);
    void clearPVTqueue();
    std::string copyPVToverFTP(std::string name);
    void execPVTQueue(std::string name);
    std::string listPVTfiles();

private:
    template <typename T>
    void eexecCommandNow(std::stringstream* strm, T value);
    template<typename T, typename... Args>
    void eexecCommandNow(std::stringstream* strm, T value, Args... args);
    void flushQueue();

    void run();
    std::queue<std::string> priority_queue;
    std::mutex mpq;
    bool _writef;

    std::mutex ftpmx;
    std::string upPVTfile;
};


/*##### Template functions #####*/
template <typename T>
void XPS::execCommandNow(T value){
    std::stringstream* nstrm=new std::stringstream();
    eexecCommandNow(nstrm, value);
}
template<typename T, typename... Args>
void XPS::execCommandNow(T value, Args... args){
    std::stringstream* nstrm=new std::stringstream();
    eexecCommandNow(nstrm, value, args...);
}
template <typename T>
void XPS::eexecCommandNow(std::stringstream* strm, T value){
    *strm<<value;
    mpq.lock();
    priority_queue.push(strm->str());
    mpq.unlock();
    //std::cerr<<"\nexecuting command:\n"<<strm->str()<<"\n";
    strm->str("");
    strm->clear();
    delete strm;
}
template<typename T, typename... Args>
void XPS::eexecCommandNow(std::stringstream* strm, T value, Args... args){
    *strm<<value;
    eexecCommandNow(strm, args...);
}

#endif // XPS_H
