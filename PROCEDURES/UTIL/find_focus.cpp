#include "find_focus.h"
#include "includes.h"
PFindFocus::PFindFocus(){

}

bool PFindFocus::init(){
    const double _SPEED=1;  //mm/s
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

    len=(pos.max[2]-pos.min[2]-2);
    po->add(1,          0,0,0,0,1  ,_SPEED);
    po->add(len/_SPEED ,0,0,0,0,len,_SPEED);
    po->add(1,          0,0,0,0,1  ,0);
    if(go.pXPS->verifyPVTobj(po).retval!=0)       //this will block and exec after MoveAbsolute is done
        return true;
    go.pXPS->execPVTobj(po);                       //we dont block here, gotta start processing frames

    framequeue->setUserFps(99999);  //set max fps
}
void PFindFocus::cleanup(){
    std::cerr<<framequeue->getFullNumber()<<" = full\n";
    framequeue->setUserFps(0);
    go.pMAKO->iuScope->FQsPCcam.deleteFQ(framequeue);
    go.pXPS->execCommand("GPIODigitalSet","GPIO3.DO", 1,0);
    go.pXPS->destroyPVTobj(po);
}
bool PFindFocus::work(){

    return true;
}
