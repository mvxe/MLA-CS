#ifndef _CONFIG_GCAM_H
#define _CONFIG_GCAM_H
#include "globals.h"

class camobj;

class gcam_config{
protected:
    std::mutex mkmx;
public:
    struct _dcams{
        std::string ID;
        std::string description;
    };
    struct _cinit{
        camobj*& ptr;
        std::string cname;
    };
    std::mutex cam_desc_mux;
    std::vector<_dcams> cam_desc;

    /*       ##########################################    <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
     *      ######### Here we hardcode cameras #########    <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
     *       ##########################################    <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <-- <--
     */

    const static int _CAM_NUM=2;
    camobj* iuScope;                //add new cameras here, and also in constructor GCAM::gcam (gcam.cpp) and run() (gcam.cpp)
    camobj* utilCam;
        //another cam here...
    _cinit _c[_CAM_NUM]={
        {iuScope,"iuScope"},
        {utilCam,"utilCam"}
            //also put it here...
    };


    /*       ##########################################
     *      ###### Here we define save variables #######
     *       ##########################################
     */

    //no general saves, for now
};


class camobj_config{
protected:
    camobj_config(std::string name):name(name){
        conf["exposure_time"]=expo;
    }

    std::mutex comx;
    std::string name;   //for saving to file
public:
    rtoml::vsr conf;                                        //configuration map

    /*       ##########################################
     *      ###### Here we define save variables #######
     *       ##########################################
     */

    //no general saves, for now
    tsvar<double> expo{&comx,442};           //these can be used by other threads
};

#endif // _CONFIG_GCAM_H
