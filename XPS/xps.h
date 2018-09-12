#ifndef XPS_H
#define XPS_H

#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <array>

#include "TCP_con.h"
#include "sharedvars.h"
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#include "mutex_containers.h"
#include "UTIL/containers.h"
#include "globals.h"
#include "hardcoded.h"

#define TRAJ_PATH "/public/Trajectories/"

class PVTobj{
    friend class XPS;
public:
    PVTobj();
    void clear();
    template<typename... Args>  void add(double val, Args... vals);
    void add(double val);
private:
    std::string filename;
    std::string groupname;
    std::stringstream data;
    bool verified;
};
typedef PVTobj* pPVTobj;

    /*~~~ XPS ~~~*/

class XPS : public TCP_con, public xps_hardcoded{
    friend class globals;    //to be able to call run()
private:
    struct execss{
        std::string comm;       //contains the command string to be executed
        xps_ret* ret;           //see UTIL/containers for available functions, set to nullptr if no return is desired
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

    template <typename T>                     void execCommand(T value);
    template <typename T, typename... Args>   void execCommand(T value, Args... args);                  //exectue a command without return, implemented as a constatntly executing FIFO to ensure the execution order matches the command call order if called repeatedly
    template <typename T>                     void execCommand(xps_ret* ret, T value);
    template <typename T, typename... Args>   void execCommand(xps_ret* ret, T value, Args... args);    //exectue a command with return, before calling this command create a xps_ret object and pass its pointer as the first argument. For readout, see xps_ret in UTIL/containers

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
    xps_dat verifyPVTobj(pPVTobj obj);                                  //returns the verification result
    xps_dat execPVTobj(pPVTobj obj);

    void MoveRelative(GroupID ID, double dX, double dY, double dZ, bool limit=true);      //TODO generalize this for n variables with templates, and generalize axisp, and add groupname
    void MoveAbsolute(GroupID ID, double  X, double  Y, double  Z, bool limit=true);
    raxis getPos(GroupID ID);
    void getPos(GroupID ID, raxis& pos);
    enum elimit { min, max };
    void setLimit(GroupID ID, int axis, elimit lim);

    void groupSetName(GroupID ID, std::string name);
    std::string groupGetName(GroupID ID);

private:
    template <typename T>                    void eexecCommand(std::stringstream* strm, xps_ret* ret, T value);
    template<typename T, typename... Args>   void eexecCommand(std::stringstream* strm, xps_ret* ret, T value, Args... args);
    void flushQueue();
    void _MoveAbsolute(GroupID ID, bool limit=true);
    void _restrict_pos(axis& pos);                  //checks if pox? is within min? and max?, if not, sets it to min?/max?

    void run();

    std::queue<execss> priority_queue;
    std::mutex mpq;                     //queue access mutex
    std::mutex gmx;                     //group access mutex
    bool _writef;

    std::mutex ftpmx;
};


#include "xps_template.h"

#endif // XPS_H
