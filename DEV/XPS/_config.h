#ifndef _CONFIG_XPS_H
#define _CONFIG_XPS_H
#include "globals.h"

/* New stages and motion groups have to be
 * defined here before they can be used
 * elsewhere in the code. See next section!*/

class xps_config{
public:
    class axis{
        friend class XPS;                       //we only want XPS to access the mutex, when it returns a axis type the user has nothing with the mutex
        friend class xps_config;                //to be able to initialize _num and thus num
    public:
        axis():num(_num){}
        double pos[8], min[8], max[8];          //the extra axes are simply not used, the small extra memory usage is not worth fixing by more complicated solutions
        const int& num;                         //so that we dont have to look into groups
        double homePos[8];                      //home pos, used internally
    private:
        std::mutex mx;
        int _num;
    };

    enum GroupID: int;
    struct group{
        const GroupID ID;
        const int AxisNum;
        std::string groupname;
        const std::string description;
        const std::string positionerNames[8];
        const double defaultVel[8];
        const double defaultAcc[8];
    };
    enum GPIOID: int;
    struct GPIO{
        const GPIOID ID;
        const std::string name;
        const unsigned mask;
        unsigned value;
        const std::string description;
        const bool inverted;
    };


    /*       ##########################################    <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
     *      ######### Here we hardcode groups ##########    <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
     *       ##########################################    <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
     */

public:
    enum GroupID: int { mgroup_XYZF, mgroup_GR1, mgroup_GR2 };   // <-- <-- <-- put the ID of each new group here, this is used to reference it throughout the code, first enum is always 0, DO NOT REARANGE THEM TO KEEP CONFIG FILES BACKWARD COMPATIBLE!
protected:
    const static int _GROUP_NUM=1;
    group groups[_GROUP_NUM]={   // <-- <-- <--  put the new groups here
                                 {mgroup_XYZF, 4, "mgroup_XYZF", "Group containing 4 stages: the X(sample), Y(sample) and Z(objective), F(focus).",{"X","Y","Z","F","","","",""},{100,25,100,100,0,0,0,0},{50,50,50,50,0,0,0,0}}    //max 8 axis per group (as limited by XPS)
//   ,{mgroup_GR1, 2, "GR1", "some group with 2 stages"}
//   ,{mgroup_GR2, 1, "GR2", "some group with 1 stage"}
    };
    std::mutex gmx;     //group access mutex


    /*       ##########################################    <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
     *      ######### Here we add some devices #########    <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
     *       ##########################################    <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
     */

public:
    enum GPIOID: int { iuScopeLED, writingLaser };
protected:
    const static int _GPIO_NUM=2;
    GPIO GPIOs[_GPIO_NUM]={
      {iuScopeLED, "GPIO3.DO", 2, 0, "LED light source for the interferometric microscope.", true},
      {writingLaser, "GPIO3.DO", 4, 0, "Writing laser trigger.", true}
    };
    std::mutex dmx;     //devices access mutex



    /*       ##########################################
     *      ###### Here we define save variables #######
     *       ##########################################
     */

protected:
    axis axisCoords[_GROUP_NUM];    //this contains current positions, min and max limits for all group axes, and automatically saves and reads them from a config file
    std::mutex smx;
public:
    rtoml::vsr conf;                                        //configuration map
    tsvar_ip IP{&smx, "192.168.0.254"};
    tsvar_port port{&smx, 5001};
    tsvar<unsigned> keepalive{&smx, 500};                   //keepalive and connect timeout, in ms

    xps_config(){
        conf["XPS_IP"]=IP;
        conf["XPS_port"]=port;
        conf["XPS_keepalive"]=keepalive;

        for (int i=0;i!=_GROUP_NUM;i++){
            for (int j=0;j!=groups[i].AxisNum;j++){
                axisCoords[i].pos[j]=0;
                axisCoords[i].min[j]=-9999;
                axisCoords[i].max[j]=9999;
                conf[groups[i].groupname][util::toString("pos-",groups[i].positionerNames[j])]=axisCoords[i].pos[j];
                conf[groups[i].groupname][util::toString("min-",groups[i].positionerNames[j])]=axisCoords[i].min[j];
                conf[groups[i].groupname][util::toString("max-",groups[i].positionerNames[j])]=axisCoords[i].max[j];
            }
            axisCoords[i]._num=groups[i].AxisNum;
        }
    }
};



#endif // _CONFIG_XPS_H
