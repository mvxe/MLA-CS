#include "find_focus.h"
#include "includes.h"

PFindFocus::PFindFocus(double min, double len, double speed, unsigned char threshold, unsigned recursived): minpos(min), len(len), speed(speed), threshold(threshold), addOfs(speed), recursived(recursived){
    lastMat = new cv::Mat;
}
PFindFocus::~PFindFocus(){
    delete lastMat;
}
bool PFindFocus::startup(){
    if(!go.pGCAM->iuScope->connected || !go.pXPS->connected) return true;
    framequeue=go.pGCAM->iuScope->FQsPCcam.getNewFQ();

    //go.pXPS->execCommand("PositionerCorrectorAutoTuning",go.pXPS->groupGetName(XPS::mgroup_XYZ), 1);
    go.pXPS->setGPIO(XPS::iuScopeLED,false);

    XPS::raxis pos=go.pXPS->getPos(XPS::mgroup_XYZ);
    posx=pos.pos[0]; posy=pos.pos[1];
    if(minpos<pos.min[2]+addOfs) minpos=pos.min[2]+addOfs;
    go.pXPS->MoveAbsolute(XPS::mgroup_XYZ,posx,posy,minpos-addOfs);
    po = go.pXPS->createNewPVTobj(XPS::mgroup_XYZ, "PFindFocus.txt");

    if(len>(pos.max[2]-minpos-2*addOfs))
       len=(pos.max[2]-minpos-2*addOfs);
    po->add(1,          0,0,0,0,addOfs  ,speed);
    po->addAction(XPS::iuScopeLED,true);
    po->add(len/speed  ,0,0,0,0,len     ,speed);
    po->addAction(XPS::iuScopeLED,false);
    po->add(1,          0,0,0,0,addOfs  ,0);
    if(go.pXPS->verifyPVTobj(po).retval!=0)       //this will block and exec after MoveAbsolute is done
        return true;
    go.pXPS->execPVTobj(po, &ret);                //we dont block here, gotta start processing frames

    framequeue->setUserFps(99999);  //set max fps

    for (;;){
        mat=framequeue->getUserMat();
        if (mat!=nullptr){
            mat->copyTo(*lastMat);
            lastTs=framequeue->getUserTimestamp();
            framequeue->freeUserMat();
            break;
        }
    }

}
bool PFindFocus::work(){
    proc_frame();
    if (ret.check_if_done()) return true;
    else return false;
}
void PFindFocus::cleanup(){
    //std::cerr<<framequeue->getFullNumber()<<" = left\n";
    framequeue->setUserFps(0);
    go.pXPS->setGPIO(XPS::iuScopeLED,true);
    go.pXPS->destroyPVTobj(po);

    while (proc_frame());

    double focus=minpos+len/(totalFr)*peakFr[1];
    std::cerr<<"total frames: "<<totalFr<<" peak frame: "<<peakFr[1]<<" focus:"<<focus<<"\n";
    go.pXPS->MoveAbsolute(XPS::mgroup_XYZ,posx,posy,focus);
    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);
    if (recursived>0){
        base_othr* a=go.newThread<PFindFocus>(focus-addOfs/10, 2*addOfs/10, speed/10, threshold, recursived-1);
        while(!a->done) std::this_thread::sleep_for (std::chrono::milliseconds(100));
        go.killThread(a);
    }
}


bool PFindFocus::proc_frame(){
    double val, valCn;
    mat=framequeue->getUserMat();
    if (mat!=nullptr){
        unsigned int newTs=framequeue->getUserTimestamp();
        if(!endPtFl){
            val=cv::mean(*mat)[0];
            if (startPtFl){
                if (val<threshold)
                    endPtFl=true;
                else{
                    valCn=0;
                    cv::addWeighted(*mat, 1, *lastMat, -1, 0, *lastMat);
                    double dif=cv::mean(*lastMat)[0];
                    if(dif>maxDif[1]) if(doTms0.doIt()){
                        maxDif[0]=dif;
                        peakFr[0]=totalFr;
                    }
                    if (maxDif[1]!=maxDif[0]) if(doTms1.doIt()){
                        maxDif[1]=maxDif[0];
                        peakFr[1]=peakFr[0];
                        doTms1.reset();
                    }

                }
                totalFr++;
            }
            else if (val>threshold)
                startPtFl=true;
        }
        try {
            mat->copyTo(*lastMat);
        }
        catch(...)
        {
            std::exception_ptr p = std::current_exception();
            std::cerr <<(p ? p.__cxa_exception_type()->name() : "null") << std::endl;
        }

        lastTs=newTs;
        framequeue->freeUserMat();
    }
    //else std::this_thread::sleep_for (std::chrono::milliseconds(1));
    //std::cerr<<framequeue->getFullNumber()<<" = left\n";
    if(framequeue->getFullNumber()) return true;
    else return false;
}
