#include "profile_beam.h"
#include "includes.h"

pProfileBeam::pProfileBeam(double minpos, double maxpos, double step, double speed, double trigdelay, std::vector<cv::Mat>* mats, std::mutex* matlk):
            minpos(minpos), maxpos(maxpos), step(step), speed(speed), trigdelay(trigdelay), mats(mats), matlk(matlk){
    mat=new cv::Mat;
}
pProfileBeam::~pProfileBeam(){}

void pProfileBeam::run(){
    {std::lock_guard<std::mutex>lock(*matlk);
    if(!mats->empty()) mats->clear();}

    int picnum=0;
    int exppicnum=0;

    if(!go.pGCAM->utilCam->connected || !go.pCNC->connected) return;
    go.pGCAM->utilCam->set_trigger("Line1");

    framequeue=go.pGCAM->utilCam->FQsPCcam.getNewFQ();
    framequeue->setUserFps(99999);

    while(1){   //clear out leftover frames
        mat=framequeue->getUserMat();
        if(mat==nullptr) break;
        framequeue->freeUserMat();
    }

    go.pCNC->execCommand("G28 X\n");
    for(double pos=minpos; pos<=maxpos; pos+=step){
        go.pCNC->execCommand("G0 X",pos," F",speed,"\n");
        go.pCNC->execCommand("M400\n");     //wait for current moves to finish
        go.pCNC->execCommand("M42 P3 S255\n");
        go.pCNC->execCommand("G4 P",trigdelay,"\n");   //wait in ms
        go.pCNC->execCommand("M400\n");
        go.pCNC->execCommand("M42 P3 S0\n");
        exppicnum++;
    }

    while(picnum!=exppicnum){
        if(end) {{std::lock_guard<std::mutex>lock(*matlk);mats->clear();} goto cleanup;}
        mat=framequeue->getUserMat();
        if(mat!=nullptr){
            std::lock_guard<std::mutex>lock(*matlk);
            mats->emplace_back();
            mat->copyTo(mats->back());
            framequeue->freeUserMat();
            picnum++;
        }
        else std::this_thread::sleep_for (std::chrono::milliseconds(10));
        //std::cerr<<picnum<<" out of "<<exppicnum<<" done\n";
    }


cleanup:
    go.pGCAM->utilCam->FQsPCcam.deleteFQ(framequeue);   //cleanup
    go.pGCAM->utilCam->set_trigger();
    done=true;
    end=true;
}
