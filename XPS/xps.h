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

class XPS : public TCP_con{
    friend class globals;    //to be able to call run()
private:
    struct execss{
        std::string comm;
        std::string* ret;
    };
public:
    XPS();
    ~XPS();

    template <typename T>                     void execCommand(T value);
    template <typename T, typename... Args>   void execCommand(T value, Args... args);            //exectue a command (nonblocking) without return, implemented as a constatntly executing FIFO to ensure the execution order matches the command call order if called repeatedly
    template <typename T>                     std::string execCommandR(T value);
    template <typename T, typename... Args>   std::string execCommandR(T value, Args... args);    //exectue a command (blocking) with return, waits for its turn in the FIFO
    void initGroups();
    void homeGroups();
    void killGroups();

    void addToPVTqueue(std::string str);
    void clearPVTqueue();
    std::string copyPVToverFTP(std::string name);
    void execPVTQueue(std::string name);
    std::string listPVTfiles();

    void XYZMoveRelative(double dX, double dY, double dZ, bool limit=true);
    void XYZMoveAbsolute(double  X, double  Y, double  Z, bool limit=true);

private:
    template <typename T>                    void eexecCommand(std::stringstream* strm, T value);
    template<typename T, typename... Args>   void eexecCommand(std::stringstream* strm, T value, Args... args);
    template <typename T>                    std::string eexecCommandR(std::stringstream* strm, T value);
    template<typename T, typename... Args>   std::string eexecCommandR(std::stringstream* strm, T value, Args... args);
    void flushQueue();
    void _XYZMoveAbsolute(bool limit=true);

    void run();

    std::queue<execss> priority_queue;
    std::mutex mpq;                     //queue access mutex
    bool _writef;

    std::mutex ftpmx;
    std::string upPVTfile;

    std::mutex varmx;
    double posX;  cc_save<double> _posX = cc_save<double>(posX,    0,&go.config.save,"posX");       //these object cause the linked variables to be automatically loaded from config file on program start, and saved to it on exit
    double posY;  cc_save<double> _posY = cc_save<double>(posY,    0,&go.config.save,"posY");
    double posZ;  cc_save<double> _posZ = cc_save<double>(posZ,    0,&go.config.save,"posZ");
    double maxX;  cc_save<double> _maxX = cc_save<double>(maxX, 9999,&go.config.save,"maxX");
    double maxY;  cc_save<double> _maxY = cc_save<double>(maxY, 9999,&go.config.save,"maxY");
    double maxZ;  cc_save<double> _maxZ = cc_save<double>(maxZ, 9999,&go.config.save,"maxZ");
    double minX;  cc_save<double> _minX = cc_save<double>(minX,-9999,&go.config.save,"minX");
    double minY;  cc_save<double> _minY = cc_save<double>(minY,-9999,&go.config.save,"minY");
    double minZ;  cc_save<double> _minZ = cc_save<double>(minZ,-9999,&go.config.save,"minZ");

};


#include "xps_template.h"

#endif // XPS_H
