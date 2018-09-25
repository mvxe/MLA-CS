#include "find_focus.h"
#include "includes.h"
#include "UTIL/IMGPROC/BASIC/imgproc_basic_stats.h"

PFindFocus::PFindFocus(double len, double speed, unsigned char threshold): len(len), speed(speed), threshold(threshold){
}
bool PFindFocus::startup(){
    if(!go.pMAKO->iuScope->connected || !go.pXPS->connected) return true;
    framequeue=go.pMAKO->iuScope->FQsPCcam.getNewFQ();

    //go.pXPS->execCommand("PositionerCorrectorAutoTuning",go.pXPS->groupGetName(XPS::mgroup_XYZ), 1);
    go.pXPS->execCommand("GPIODigitalSet","GPIO3.DO", 1,1);
    go.pXPS->execCommand("EventExtendedConfigurationTriggerSet", util::toString(go.pXPS->groupGetName(XPS::mgroup_XYZ),".PVT.ElementNumberStart"),2,0,0,0);
    go.pXPS->execCommand("EventExtendedConfigurationActionSet", "GPIO3.DO.DOSet",1,0,0,0);
    go.pXPS->execCommand("EventExtendedStart","int *");
    go.pXPS->execCommand("EventExtendedConfigurationTriggerSet", util::toString(go.pXPS->groupGetName(XPS::mgroup_XYZ),".PVT.ElementNumberStart"),3,0,0,0);
    go.pXPS->execCommand("EventExtendedConfigurationActionSet", "GPIO3.DO.DOSet",1,1,0,0);
    go.pXPS->execCommand("EventExtendedStart","int *");

    XPS::raxis pos=go.pXPS->getPos(XPS::mgroup_XYZ);
    go.pXPS->MoveAbsolute(XPS::mgroup_XYZ,pos.pos[0],pos.pos[1],pos.min[2]);
    po = go.pXPS->createNewPVTobj(XPS::mgroup_XYZ, "PFindFocus.txt");

    if(len>(pos.max[2]-pos.min[2]-2)) return true;
    po->add(1,          0,0,0,0,0.1  ,speed);
    po->add(len/speed  ,0,0,0,0,len  ,speed);
    po->add(1,          0,0,0,0,0.1  ,0);
    if(go.pXPS->verifyPVTobj(po).retval!=0)       //this will block and exec after MoveAbsolute is done
        return true;
    go.pXPS->execPVTobj(po, &ret);                //we dont block here, gotta start processing frames

    framequeue->setUserFps(99999);  //set max fps
}
int aaa=0;
int bbb=0;

void PFindFocus::cleanup(){
    std::cerr<<framequeue->getFullNumber()<<" = left\n";
    std::cerr<<aaa<<" = aaa "<<bbb<<" = bbb \n";
    framequeue->setUserFps(0);
    go.pXPS->execCommand("GPIODigitalSet","GPIO3.DO", 1,0);
    go.pXPS->destroyPVTobj(po);

    while (proc_frame());
    std::cerr<<aaa<<" = aaa "<<bbb<<" = bbb \n";

    go.pMAKO->iuScope->FQsPCcam.deleteFQ(framequeue);
}
bool PFindFocus::work(){
    proc_frame();
    if (ret.check_if_done()) return true;
    else return false;
}

bool PFindFocus::proc_frame(){
    double val, valCn;
    mat=framequeue->getUserMat();
    if (mat!=nullptr){
        unsigned int newTs=framequeue->getUserTimestamp();
        //std::cerr<<"mat value: "<<imgproc_basic_stats::get_avg_value(mat)<<" ts: "<< newTs-lastTs<<"\n";
        bbb++;
        if(!endPtFl){
            val=imgproc_basic_stats::get_avg_value(mat);
            if (startPtFl){
                if (val<threshold)
                    endPtFl=true;
                else{
                    valCn=0;//imgproc_basic_stats::get_contrast(mat, val);
                    aaa++;
                    std::cerr<<"valCn: "<<valCn<<" val: "<<val<<" ts: "<< newTs-lastTs <<"\n";

                }
            }
            else if (val>threshold)
                startPtFl=true;
        }
        framequeue->freeUserMat();
        lastTs=newTs;
    }
    //else std::this_thread::sleep_for (std::chrono::milliseconds(1));
    //std::cerr<<framequeue->getFullNumber()<<" = left\n";

    if(framequeue->getFullNumber()) return true;
    else return false;
}
