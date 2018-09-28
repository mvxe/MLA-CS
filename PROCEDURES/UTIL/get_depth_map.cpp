#include "get_depth_map.h"
#include "includes.h"

pGetDepthMap::pGetDepthMap(double range, double speed, unsigned char threshold): range(range), speed(speed), threshold(threshold), addOfs(speed){
}
pGetDepthMap::~pGetDepthMap(){
}
void pGetDepthMap::run(){
    if(!go.pMAKO->iuScope->connected || !go.pXPS->connected) return;
    framequeue=go.pMAKO->iuScope->FQsPCcam.getNewFQ();

    go.pXPS->setGPIO(XPS::iuScopeLED,false);

    po = go.pXPS->createNewPVTobj(XPS::mgroup_XYZ, "pGetDepthMap.txt");
    po->add(1,          0,0,0,0,-range/2-addOfs  ,0);
    po->add(1,          0,0,0,0,addOfs           ,speed);                           //same as po->addAction("GPIO3.DO.DOSet",1,0,0,0);
    po->addAction(XPS::iuScopeLED,true);
    po->add(range/speed,0,0,0,0,range            ,speed);
    po->addAction(XPS::iuScopeLED,false);                                           //same as po->addAction("GPIO3.DO.DOSet",1,1,0,0);
    po->add(1,          0,0,0,0,addOfs           ,0);
    po->add(1,          0,0,0,0,-range/2-addOfs  ,0);
    if(go.pXPS->verifyPVTobj(po).retval!=0) return;                                 //this will block and exec after MoveAbsolute is done
    go.pXPS->execPVTobj(po, &ret);                                                  //we dont block here, gotta start processing frames

std::cout<<"toal mats: "<<go.pMAKO->iuScope->FQsPCcam.getFullNumber()<<"+"<<go.pMAKO->iuScope->FQsPCcam.getFreeNumber()<<",in this:"<<framequeue->getFullNumber()<<"\n";
    framequeue->setUserFps(99999);  //set max fps

    while (!ret.check_if_done());

    framequeue->setUserFps(0);
std::cout<<"toal mats: "<<go.pMAKO->iuScope->FQsPCcam.getFullNumber()<<"+"<<go.pMAKO->iuScope->FQsPCcam.getFreeNumber()<<",in this:"<<framequeue->getFullNumber()<<"\n";


    go.pXPS->setGPIO(XPS::iuScopeLED,true);
    go.pMAKO->iuScope->FQsPCcam.deleteFQ(framequeue);
    done=true;
    end=true;
}
