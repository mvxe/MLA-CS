#ifndef HARDCODED_H
#define HARDCODED_H

/* New stages and motion groups have to be
 * defined here before they can be used
 * elsewhere in the code. See next section!*/

class xps_hardcoded{
public:
    class axis{
        friend class XPS;                       //we only want XPS to access the mutex, when it returns a axis type the user has nothing with the mutex
        friend class xps_hardcoded;             //to be able to initialize _num and thus num
    public:
        axis():num(_num){}
        double pos[8], min[8], max[8];          //the extra axes are simply not used, the small extra memory usage is not worth fixing by more complicated solutions
        const int& num;                         //so that we dont have to look into groups
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
    };





    /*       ##########################################    <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
     *      ######### Here we hardcode groups ##########    <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
     *       ##########################################    <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
     */

public:
    enum GroupID: int { mgroup_XYZ, mgroup_GR1, mgroup_GR2 };   // <-- <-- <-- put the ID of each new group here, this is used to reference it throughout the code, first enum is always 0, DO NOT REARANGE THEM TO KEEP CONFIG FILES BACKWARD COMPATIBLE!
protected:
    const static int _GROUP_NUM=1;
    group groups[_GROUP_NUM]={   // <-- <-- <--  put the new groups here
      {mgroup_XYZ, 3, "mgroup_XYZ", "Group containing 3 stages: the X(sample), Y(sample) and Z(objective)."}    //max 8 axis per group (as limited by XPS)
//   ,{mgroup_GR1, 2, "GR1", "some group with 2 stages"}
//   ,{mgroup_GR2, 1, "GR2", "some group with 1 stage"}
    };
    std::mutex gmx;     //group access mutex


    /*       ##########################################
     *      ###### Here we define save variables #######
     *       ##########################################
     */

protected:
    cc_save<std::string>* save_groupnames[_GROUP_NUM];
    cc_save<double>* save_axisCoords[3*8*_GROUP_NUM];   //not all of these are used
    axis axisCoords[_GROUP_NUM];    //this contains current positions, min and max limits for all group axes, and automatically saves and reads them from a config file
public:
    xps_hardcoded(){
        for (int i=0;i!=_GROUP_NUM;i++){
            save_groupnames[i]=new cc_save<std::string>(groups[i].groupname, groups[i].groupname, &go.config.save, util::toString("groupname_",i));    //read group names from config file (new calls the cc_save constructor which reads the old value from config file, if it exists)
            for (int j=0;j!=groups[i].AxisNum;j++){
                save_axisCoords[3*8*i+3*j  ]=new cc_save<double>(axisCoords[i].pos[j],    0,&go.config.save, util::toString("grp",i,"pos",j));
                save_axisCoords[3*8*i+3*j+1]=new cc_save<double>(axisCoords[i].min[j],-9999,&go.config.save, util::toString("grp",i,"min",j));
                save_axisCoords[3*8*i+3*j+2]=new cc_save<double>(axisCoords[i].max[j], 9999,&go.config.save, util::toString("grp",i,"max",j));
            }
            axisCoords[i]._num=groups[i].AxisNum;
        }
    }
    ~xps_hardcoded(){
        for (int i=0;i!=_GROUP_NUM;i++){
            delete save_groupnames[i];                  //save group names to config file (delete calls the cc_save destructor which queues the new value for saving into config file)
            for (int j=0;j!=groups[i].AxisNum;j++){
                delete save_axisCoords[3*8*i+3*j  ];
                delete save_axisCoords[3*8*i+3*j+1];
                delete save_axisCoords[3*8*i+3*j+2];
            }
        }
    }

};



#endif // HARDCODED_H