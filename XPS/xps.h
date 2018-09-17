#ifndef XPS_H
#define XPS_H

#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <array>
#include <atomic>

#include "TCP_con.h"
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
private:
    template<typename... Args>  void _add(int n, double val, Args... vals);
    void _add(int n, double val);           //no PVT file row has only one var(time), so we set this private
    xps_config::GroupID ID;
    std::string filename;
    std::stringstream data;
    bool verified{false};
};
typedef PVTobj* pPVTobj;

/*########## XPS ##########*/

class XPS : public TCP_con, public xps_config{
    friend class globals;    //to be able to call run()
    friend void PVTobj::_add(int n, double val);    //needs access to groups for axisnum
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

    template <typename T>                     void execCommandStr(T value);
    template <typename T, typename... Args>   void execCommandStr(T value, Args... args);                  //exectue a command without return, implemented as a constatntly executing FIFO to ensure the execution order matches the command call order if called repeatedly
    template <typename T>                     void execCommandStr(exec_ret* ret, T value);
    template <typename T, typename... Args>   void execCommandStr(exec_ret* ret, T value, Args... args);    //exectue a command with return, before calling this command create a xps_ret object and pass its pointer as the first argument. For readout, see xps_ret in UTIL/containers

    template <typename T>                     void execCommand(std::string command, T value);              //same as above command, except it automatically adds brackets and comas, for example  <command>(<arg0>,<arg1>,<arg2>...) and sends that to eexecCommandStr
    template <typename T, typename... Args>   void execCommand(std::string command, T value, Args... args);
    template <typename T>                     void execCommand(exec_ret* ret, std::string command, T value);
    template <typename T, typename... Args>   void execCommand(exec_ret* ret, std::string command, T value, Args... args);

    void initGroup(GroupID ID);
    void initGroups();
    void homeGroup(GroupID ID);
    void homeGroups();
    void killGroup(GroupID ID);
    void killGroups();

    std::string listPVTfiles();

    pPVTobj createNewPVTobj(GroupID ID, std::string filename);
    void destroyPVTobj(pPVTobj obj);                                    //deallocates it
    std::string copyPVToverFTP(pPVTobj obj);                            //if successful, returns an empty string, else a string containing the error message
    exec_dat verifyPVTobj(pPVTobj obj);                                  //returns the verification result
    exec_dat execPVTobj(pPVTobj obj);

    std::atomic<bool> limit{true};        //set to false to disable limits for the next move command (it is automatically set to true afterwards), the atomic type is thread safe
    template<typename... Args>  void MoveRelative(GroupID ID, double val, Args... vals);
    void MoveRelative(GroupID ID, double val);
    template<typename... Args>  void MoveAbsolute(GroupID ID, double val, Args... vals);
    void MoveAbsolute(GroupID ID, double val);

    raxis getPos(GroupID ID);
    void getPos(GroupID ID, raxis& pos);
    enum elimit { min, max };
    void setLimit(GroupID ID, int axis, elimit lim);

    void groupSetName(GroupID ID, std::string name);
    std::string groupGetName(GroupID ID);

private:
    template <typename T>                    void eexecCommandStr(std::stringstream* strm, exec_ret* ret, T value);
    template <typename T, typename... Args>  void eexecCommandStr(std::stringstream* strm, exec_ret* ret, T value, Args... args);

    template <typename T>                    void eexecCommand(std::stringstream* strm, exec_ret* ret, T value);
    template <typename T, typename... Args>  void eexecCommand(std::stringstream* strm, exec_ret* ret, T value, Args... args);

    template<typename... Args>  void _MoveRelative(int n, GroupID ID, double val, Args... vals);
    void _MoveRelative(int n, GroupID ID, double val);
    template<typename... Args>  void _MoveAbsolute(int n, GroupID ID, double val, Args... vals);
    void _MoveAbsolute(int n, GroupID ID, double val);
    void flushQueue();
    void __MoveAbsolute(GroupID ID);
    void _restrict_pos(axis& pos);                  //checks if pox? is within min? and max?, if not, sets it to min?/max?

    void run();

    std::queue<execss> priority_queue;
    std::mutex mpq;                     //queue access mutex
    bool _writef{false};

    std::mutex ftpmx;
};


#include "xps_template.h"

#endif // XPS_H
