#include "find_focus.h"
#include "includes.h"
#include "UTIL/IMGPROC/BASIC/imgproc_basic_stats.h"

PFindFocus::PFindFocus(double len, double speed): len(len), speed(speed){
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
void PFindFocus::cleanup(){
    std::cerr<<framequeue->getFullNumber()<<" = left\n";
    framequeue->setUserFps(0);
    while (proc_frame());

    go.pMAKO->iuScope->FQsPCcam.deleteFQ(framequeue);
    go.pXPS->execCommand("GPIODigitalSet","GPIO3.DO", 1,0);
    go.pXPS->destroyPVTobj(po);
}
bool PFindFocus::work(){
    proc_frame();
    if (ret.check_if_done()) return true;
    else return false;
}

bool PFindFocus::proc_frame(){
    mat=framequeue->getUserMat();
    if (mat!=nullptr){
        std::cerr<<"mat value: "<<imgproc_basic_stats::get_avg_value(mat)<<"\n";
        framequeue->freeUserMat();
        return true;
    }
    return false;
}
