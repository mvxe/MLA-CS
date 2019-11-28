﻿#include "focus.h"
#include "scan.h"
#include "UTIL/img_util.h"
#include "GUI/gui_includes.h"
#include "includes.h"


//GUI

pgFocusGUI::pgFocusGUI(mesLockProg& MLP, pgScanGUI* pgSGUI): MLP(MLP), pgSGUI(pgSGUI){
    PVTScan=go.pXPS->createNewPVTobj(XPS::mgroup_XYZF, util::toString("camera_PVTfocus.txt").c_str());
    init_gui_activation();
    init_gui_settings();
    timer = new QTimer(this);
    timer->setInterval(timer_delay);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(recalculate()));
}

void pgFocusGUI::init_gui_activation(){
    gui_activation=new QWidget;
    alayout=new QHBoxLayout;
    gui_activation->setLayout(alayout);
    bFocus=new QPushButton;
    bFocus->setText("ReFocus");
    connect(bFocus, SIGNAL(released()), this, SLOT(onRefocus()));
    alayout->addWidget(new twid(bFocus));
}
void pgFocusGUI::onRefocus(){if(MLP._lock_meas.try_lock()){MLP._lock_meas.unlock();refocus();}}
void pgFocusGUI::refocus(){
    if(!PVTsRdy) recalculate();
    _focusingDone=false;
    go.OCL_threadpool.doJob(std::bind(&pgFocusGUI::_refocus,this));
}

void pgFocusGUI::init_gui_settings(){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    slayout->addWidget(new QLabel("Some settings borrowed from Scan!"));
    selectFocusSetting=new smp_selector("Select focus setting: ", 0, {"Short", "Medium", "Long"});    //should have Nset strings
    slayout->addWidget(selectFocusSetting);
    for(int i=0;i!=Nset;i++) {
        settingWdg.push_back(new focusSettings(i, this));
        slayout->addWidget(settingWdg.back());
    }
    connect(selectFocusSetting, SIGNAL(changed(int)), this, SLOT(onMenuChange(int)));
    calcL=new QLabel;
    slayout->addWidget(calcL);
    onMenuChange(0);
}

focusSettings::focusSettings(uint num, pgFocusGUI* parent): parent(parent){
    slayout=new QVBoxLayout;
    this->setLayout(slayout);
    range=new val_selector(10., util::toString("pgFocusGUI_range",num), "Scan Range:", 1., 2000., 3, 3 , {"nm","um",QChar(0x03BB),"L"});
    connect(range, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(range);
    ppwl=new val_selector(10., util::toString("pgFocusGUI_ppwl",num), "Points Per Wavelength: ", 0.001, 100., 3);
    connect(ppwl, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(ppwl);
}
void pgFocusGUI::onMenuChange(int index){
    for(int i=0;i!=Nset;i++) settingWdg[i]->setVisible(i==index?true:false);
    range=settingWdg[index]->range;
    ppwl=settingWdg[index]->ppwl;
    recalculate();
}

void pgFocusGUI::recalculate() {
    if(MLP._lock_meas.try_lock()){
        if(MLP._lock_comp.try_lock()){
            std::string report;
            updatePVT(report);
            calcL->setText(report.c_str());
            MLP._lock_comp.unlock();
        }
        else timer->start();
        MLP._lock_meas.unlock();
    }
    else timer->start();
}

double pgFocusGUI::vsConv(val_selector* vs){return pgSGUI->vsConv(vs);}
void pgFocusGUI::updatePVT(std::string &report){
    if(!go.pGCAM->iuScope->connected || !go.pXPS->connected) return;
    double minFPS,maxFPS;
    go.pGCAM->iuScope->get_frame_rate_bounds (&minFPS, &maxFPS);
    double readTime=vsConv(range)*vsConv(ppwl)/vsConv(pgSGUI->led_wl)/maxFPS*2;   //s
    double readRangeDis=vsConv(range)/1000000;
    double readVelocity=readRangeDis/readTime;                          //mm/s
    report+=util::toString("Read Time =",readTime," s\nRead Range Distance:",readRangeDis," mm\nRead Velocity:",readVelocity," m/s\n");
    if(readVelocity>vsConv(pgSGUI->max_vel)) {report+="Error: Read Velocity is higher than set max Velocity!\n"; return;}
    double readAccelTime=readVelocity/vsConv(pgSGUI->max_acc);
    double readAccelDis=vsConv(pgSGUI->max_acc)/2*readAccelTime*readAccelTime;
    double Offset=readRangeDis/2+readAccelDis;
    PVTsRdy=false;
    PVTScan->clear();


    double movTime=2*sqrt(2*Offset/vsConv(pgSGUI->max_acc));
    double movMaxVelocity=vsConv(pgSGUI->max_acc)*movTime;

    if(movMaxVelocity > vsConv(pgSGUI->max_vel)){report+="Error: Max move Velocity is higher than set max Velocity (did not implement workaround it cus it was unlikely)!\n"; return;}

    double darkFrameTime=pgSGUI->darkFrameNum/maxFPS;
    totalFrameNum=readTime*maxFPS;
    mmPerFrame=readRangeDis/totalFrameNum;
    report+=util::toString("\nTotal expected number of useful frames for focusing: ",totalFrameNum,"\n");
    report+=util::toString("\nTotal time needed for focusing (+computation): ",2*movTime+2*readAccelTime+darkFrameTime+readTime," s\n");

    PVTScan->addAction(XPS::iuScopeLED,true);
    PVTScan->add(movTime, 0,0, 0,0, -Offset,0, 0,0);
    PVTScan->add(readAccelTime,  0,0, 0,0, readAccelDis,readVelocity, 0,0);
    PVTScan->add(readTime, 0,0, 0,0, readRangeDis,readVelocity, 0,0);
    PVTScan->addAction(XPS::iuScopeLED,false);
    PVTScan->add(readAccelTime,  0,0, 0,0, readAccelDis,0, 0,0);
    if(darkFrameTime>movTime+readAccelTime)
        PVTScan->add(darkFrameTime-movTime-readAccelTime,  0,0, 0,0, 0,0, 0,0);
    PVTScan->add(movTime, 0,0, 0,0, -Offset,0, 0,0);
    PVTScan->addAction(XPS::iuScopeLED,true);
    total_meas_time=2*movTime+2*readAccelTime+readTime+((darkFrameTime>movTime+readAccelTime)?(darkFrameTime-movTime-readAccelTime):0);

    exec_dat ret;
    ret=go.pXPS->verifyPVTobj(PVTScan);
    if(ret.retval!=0) {report+=util::toString("Error: Verify PVTScan failed, retval was",ret.retstr,"\n"); return;}

    PVTsRdy=true;
}

void pgFocusGUI::_refocus(){
    if(!go.pGCAM->iuScope->connected || !go.pXPS->connected) return;
    if(!PVTsRdy) return;
    std::lock_guard<std::mutex>lock(MLP._lock_meas);                       //wait for other measurements to complete
    int nFrames=totalFrameNum;
    FQ* framequeue;
    framequeue=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
    framequeue->setUserFps(99999);
    go.pXPS->execPVTobjB(PVTScan);
    framequeue->setUserFps(0);

    std::lock_guard<std::mutex>lock2(MLP._lock_comp);    //wait till other processing is done

        //TODO: this is copy paste from scan, maybe join them in a function to reuse code?
    if(framequeue->getFullNumber()<nFrames) {std::cerr<<"ERROR: took "<<framequeue->getFullNumber()<<" frames, expected at least "<<nFrames<<".\n";return;}
    double mean=0;
    for(int i=0;i!=10;i++)
        mean+= cv::mean(*framequeue->getUserMat(i))[0];
    mean/=10;
    for(int i=0;i!=framequeue->getFullNumber();i++){
        double iMean= cv::mean(*framequeue->getUserMat(framequeue->getFullNumber()-1-i))[0];
        if(iMean<4*mean/5 && framequeue->getFullNumber()>nFrames){
            framequeue->freeUserMat(framequeue->getFullNumber()-1-i);
            i--;
        } else break;
    }
    if(framequeue->getFullNumber()<nFrames) {std::cerr<<"ERROR: after removing dark frames left with "<<framequeue->getFullNumber()<<" frames, expected at least "<<nFrames<<".\n";return;}
    while(framequeue->getFullNumber()>nFrames)
        framequeue->freeUserMat(0);

    int nRows=framequeue->getUserMat(0)->rows;
    int nCols=framequeue->getUserMat(0)->cols;

    cv::UMat calcMat(nRows, nCols, CV_8U);
    cv::UMat calcMatRed(nRows, 1, CV_32F);
    cv::Mat result(nFrames, 1, CV_32F);
    double min,max;
    for(int i=0;i!=nFrames;i++){
        framequeue->getUserMat(i)->copyTo(calcMat);
        cv::reduce(calcMat, calcMatRed, 0, cv::REDUCE_AVG, CV_32F);
        cv::minMaxLoc(calcMatRed, &min, &max);
        result.at<float>(i)=max-min;
    }
    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);

//    std::ofstream wfile ("focus.txt");
//    for(int i=0;i!=nFrames;i++) wfile<<i<<" "<<result.at<float>(i)<<"\n";
//    wfile.close();
    cv::Point minLoc,maxLoc;
    cv::minMaxLoc(result, &min, &max, &minLoc, &maxLoc);
    double focus=(maxLoc.y-nFrames/2.)*mmPerFrame;
    go.pXPS->MoveRelative(XPS::mgroup_XYZF,0,0,focus,-focus);
    _focusingDone=true;
}
