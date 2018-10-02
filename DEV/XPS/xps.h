#ifndef XPS_H
#define XPS_H

#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <array>
#include <atomic>

#include "DEV/TCP_con.h"
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#include "UTIL/containers.h"
#include "_config.h"

#define TRAJ_PATH "/public/Trajectories/"

class PVTobj{
    friend class XPS;
public:
    PVTobj();
    void clear();
    template<typename... Args>  void add(double val, Args... vals);
    template<typename... Args>  void addAction(Args... vals);
    void addAction(xps_config::GPIOID ID, bool value);
private:
    template<typename... Args>  void _add(int n, double val, Args... vals);
    void _add(int n, double val);           //no PVT file row has only one var(time), so we set this private
    xps_config::GroupID ID;
    std::string filename;
    std::stringstream data;
    std::queue<double> pvtqueue;
    std::queue<std::string> cmdQueue;
    bool cmdWasLast{false};
    bool verified{false};
};
typedef PVTobj* pPVTobj;

/*########## XPS ##########*/

class XPS : public TCP_con, public xps_config, public protooth{
    friend void PVTobj::_add(int n, double val);                               //needs access to groups for axisnum
    template<typename... Args> friend void PVTobj::addAction(Args... vals);    //needs access to groups for axisnum
private:
    struct execss{
        std::string comm;       //contains the command string to be executed
        exec_ret* ret;          //see UTIL/containers for available functions, set to nullptr if no return is desired
    };
public:
    class raxis{
    public:
        raxis(axis* a):num(a->num){for(int i=0;i!=8;i++){pos[i]=a->pos[i];min[i]=a->min[i];max[i]=a->max[i];}}
        double pos[8], min[8], max[8];          //the extra axes are simply not used, the small extra memory usage is not worth fixing by more complicated solutions
        int num;
    };

    XPS();
    ~XPS();

    template <typename... Args>   void execCommandStr(Args... args);                   //exectue a command without return, implemented as a constatntly executing FIFO to ensure the execution order matches the command call order if called repeatedly
    template <typename... Args>   void execCommandStr(exec_ret* ret, Args... args);    //exectue a command with return, before calling this command create a xps_ret object and pass its pointer as the first argument. For readout, see xps_ret in UTIL/containers

    template <typename... Args>   void execCommand(Args... args);                      //same as above command, except it automatically adds brackets and comas, for example  <command>(<arg0>,<arg1>,<arg2>...)
    template <typename... Args>   void execCommand(exec_ret* ret, Args... args);

    void initGroup(GroupID ID);
    void initGroups();
    void homeGroup(GroupID ID);
    void homeGroups();
    void killGroup(GroupID ID);
    void killGroups();

    std::string listPVTfiles();

    pPVTobj createNewPVTobj(GroupID ID, std::string filename);
    void destroyPVTobj(pPVTobj obj);                                    //deallocates it
    exec_dat verifyPVTobj(pPVTobj obj);                                 //returns the verification result, on error also clears the queue
    void execPVTobj(pPVTobj obj, exec_ret* ret=nullptr);                //non blocking, with optional return, which can be used to check if its done
    exec_dat execPVTobjB(pPVTobj obj);                                  //blocks until done

    std::atomic<bool> limit{true};        //set to false to disable limits for the next move command (it is automatically set to true afterwards), the atomic type is thread safe
    template<typename... Args>  void MoveRelative(GroupID ID, double val, Args... vals);
    void MoveRelative(GroupID ID, double val);
    template<typename... Args>  void MoveAbsolute(GroupID ID, double val, Args... vals);
    void MoveAbsolute(GroupID ID, double val);
    void syncPos(GroupID ID);                                           //pulls the current position from the xps and updates axisCoords


    raxis getPos(GroupID ID);
    void getPos(GroupID ID, raxis& pos);
    enum elimit { min, max };
    void setLimit(GroupID ID, int axis, elimit lim);

    void groupSetName(GroupID ID, std::string name);
    std::string groupGetName(GroupID ID);

    void setGPIO(GPIOID ID, bool value);
    bool getGPIO(GPIOID ID);
    const GPIO& getGPIOobj(GPIOID ID);

private:
    template<typename... Args>  void _MoveRelative(int n, GroupID ID, double val, Args... vals);
    void _MoveRelative(int n, GroupID ID, double val);
    template<typename... Args>  void _MoveAbsolute(int n, GroupID ID, double val, Args... vals);
    void _MoveAbsolute(int n, GroupID ID, double val);
    std::string _copyPVToverFTP(pPVTobj obj);
    void flushQueue();
    void __MoveAbsolute(GroupID ID);
    void _restrict_pos(axis& pos);                          //checks if pox? is within min? and max?, if not, sets it to min?/max?
    void _syncPosHandle(GroupID ID);

    void run();

    std::queue<execss> priority_queue;
    std::mutex mpq;                     //queue access mutex
    bool _writef{false};

    std::mutex ftpmx;
};

#include "xps_template.h"

#endif // XPS_H
