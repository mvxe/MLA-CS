#ifndef XPS_H
#define XPS_H

#include <string>
#include <mutex>
#include <queue>
#include <thread>

#include "TCP_con.h"
#include "sharedvars.h"
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#include "mutex_containers.h"
#include "UTIL/containers.h"
#include "globals.h"

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

class XPS : public TCP_con{
    friend class globals;    //to be able to call run()
public:
    struct axisp{
        double posX, posY, posZ;
        double minX, minY, minZ;
        double maxX, maxY, maxZ;
    };

private:
    struct execss{
        std::string comm;       //contains the command string to be executed
        xps_ret* ret;           //see UTIL/containers for available functions, set to nullptr if no return is desired
    };
public:
    XPS();
    ~XPS();

    template <typename T>                     void execCommand(T value);
    template <typename T, typename... Args>   void execCommand(T value, Args... args);                  //exectue a command without return, implemented as a constatntly executing FIFO to ensure the execution order matches the command call order if called repeatedly
    template <typename T>                     void execCommand(xps_ret* ret, T value);
    template <typename T, typename... Args>   void execCommand(xps_ret* ret, T value, Args... args);    //exectue a command with return, before calling this command create a xps_ret object and pass its pointer as the first argument. For readout, see xps_ret in UTIL/containers
    void initGroup(std::string groupname);
    void homeGroup(std::string groupname);
    void killGroup(std::string groupname);

    std::string listPVTfiles();

    pPVTobj createNewPVTobj(std::string motion_group, std::string filename);        //TODO make automatic filename generation
    void destroyPVTobj(pPVTobj obj);                                    //deallocates it
    std::string copyPVToverFTP(pPVTobj obj);                            //if successful, returns an empty string, else a string containing the error message
    xps_ret verifyPVTobj(pPVTobj obj);                                  //returns the verification result
    xps_ret execPVTobj(pPVTobj obj);

    void XYZMoveRelative(double dX, double dY, double dZ, bool limit=true);      //TODO generalize this for n variables with templates, and generalize axisp, and add groupname
    void XYZMoveAbsolute(double  X, double  Y, double  Z, bool limit=true);
    axisp getXYZpos();
    void getXYZpos(axisp& rpos);
    enum elimit { minX, minY, minZ, maxX, maxY, maxZ };
    void setLimit(elimit lim);
private:
    template <typename T>                    void eexecCommand(std::stringstream* strm, xps_ret* ret, T value);
    template<typename T, typename... Args>   void eexecCommand(std::stringstream* strm, xps_ret* ret, T value, Args... args);
    void flushQueue();
    void _XYZMoveAbsolute(bool limit=true);
    void axisp_restrict_pos(axisp& poss);                  //checks if pox? is within min? and max?, if not, sets it to min?/max?

    void run();

    std::queue<execss> priority_queue;
    std::mutex mpq;                     //queue access mutex
    bool _writef;

    std::mutex ftpmx;
    std::string upPVTfile;

    axisp pos;
    std::mutex posmx;
    cc_save<double> _posX = cc_save<double>(pos.posX,    0,&go.config.save,"posX");       //these object cause the linked variables to be automatically loaded from config file on program start, and saved to it on exit
    cc_save<double> _posY = cc_save<double>(pos.posY,    0,&go.config.save,"posY");
    cc_save<double> _posZ = cc_save<double>(pos.posZ,    0,&go.config.save,"posZ");
    cc_save<double> _maxX = cc_save<double>(pos.maxX, 9999,&go.config.save,"maxX");
    cc_save<double> _maxY = cc_save<double>(pos.maxY, 9999,&go.config.save,"maxY");
    cc_save<double> _maxZ = cc_save<double>(pos.maxZ, 9999,&go.config.save,"maxZ");
    cc_save<double> _minX = cc_save<double>(pos.minX,-9999,&go.config.save,"minX");
    cc_save<double> _minY = cc_save<double>(pos.minY,-9999,&go.config.save,"minY");
    cc_save<double> _minZ = cc_save<double>(pos.minZ,-9999,&go.config.save,"minZ");

};


#include "xps_template.h"

#endif // XPS_H
