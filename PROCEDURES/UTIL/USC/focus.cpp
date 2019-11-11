#include "focus.h"
#include "scan.h"
#include "UTIL/img_util.h"
#include "GUI/gui_includes.h"
#include "includes.h"


//GUI

pgFocusGUI::pgFocusGUI(std::mutex& _lock_mes, std::mutex& _lock_comp, pgScanGUI* pgSGUI, pgTiltGUI* pgTGUI): _lock_mes(_lock_mes), _lock_comp(_lock_comp), pgSGUI(pgSGUI), pgTGUI(pgTGUI) {
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
    QWidget* twid=new QWidget; QHBoxLayout* tlay=new QHBoxLayout; twid->setLayout(tlay);
    bFocus=new QPushButton;
    bFocus->setText("ReFocus");
    connect(bFocus, SIGNAL(released()), this, SLOT(onRefocus()));
    tlay->addWidget(bFocus); tlay->addStretch(0); tlay->setMargin(0);
    alayout->addWidget(twid);
}
void pgFocusGUI::onRefocus(){if(_lock_mes.try_lock()){_lock_mes.unlock();refocus();}}
void pgFocusGUI::refocus(){
    if(!PVTsRdy) recalculate();
    go.OCL_threadpool.doJob(std::bind(&pgFocusGUI::_refocus,this));
}

void pgFocusGUI::init_gui_settings(){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    QLabel* tlabel=new QLabel("Some settings borrowed from Scan!");
    slayout->addWidget(tlabel);
    range=new val_selector(10., "pgFocusGUI_range", "Scan Range:", 1., 2000., 3, 3 , {"nm","um",QChar(0x03BB),"L"});
    connect(range, SIGNAL(changed()), this, SLOT(recalculate()));
    slayout->addWidget(range);
    ppwl=new val_selector(1., "pgFocusGUI_ppwl", "Points Per Wavelength: ", 0.001, 100., 3);
    connect(ppwl, SIGNAL(changed()), this, SLOT(recalculate()));
    slayout->addWidget(ppwl);
    tilt=new val_selector(1., "pgFocusGUI_tilt", "Ammount of tilt: ", 1, 10000., 0);
    connect(tilt, SIGNAL(changed()), this, SLOT(recalculate()));
    slayout->addWidget(tilt);
    calcL=new QLabel;
    slayout->addWidget(calcL);
    QWidget* twid=new QWidget; QHBoxLayout* tlay=new QHBoxLayout; twid->setLayout(tlay);
    testTilt=new QPushButton;
    testTilt->setText("Test Tilt");
    testTilt->setCheckable(true);
    connect(testTilt, SIGNAL(toggled(bool)), this, SLOT(onTestTilt(bool)));
    tlay->addWidget(testTilt); tlay->addStretch(0); tlay->setMargin(0);
    slayout->addWidget(twid);
}

void pgFocusGUI::recalculate() {
    if(_lock_mes.try_lock()){
        if(_lock_comp.try_lock()){
            std::string report;
            updatePVT(report);
            calcL->setText(report.c_str());
            _lock_comp.unlock();
        }
        else timer->start();
        _lock_mes.unlock();
    }
    else timer->start();
}

double pgFocusGUI::vsConv(val_selector* vs){return pgSGUI->vsConv(vs);}
void pgFocusGUI::updatePVT(std::string &report){
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
    report+=util::toString("\nTime needed to tilt (one way): ",tilt->val/pgTGUI->tilt_motor_speed->val*60," s\n");

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

    exec_dat ret;
    ret=go.pXPS->verifyPVTobj(PVTScan);
    if(ret.retval!=0) {report+=util::toString("Error: Verify PVTScan failed, retval was",ret.retstr,"\n"); return;}

    PVTsRdy=true;
}

void pgFocusGUI::onTestTilt(bool state){
    if(state) pgTGUI->doTilt(tilt->val,0,false);
    else pgTGUI->doTilt(-tilt->val,0,false);
}

void pgFocusGUI::_refocus(){
    if(!PVTsRdy) return;
    _lock_mes.lock();                       //wait for other measurements to complete
    pgTGUI->doTilt(tilt->val, 0, false);
    std::this_thread::sleep_for (std::chrono::milliseconds(int(tilt->val/pgTGUI->tilt_motor_speed->val*60*1000)));      //wait till it tilts
    int nFrames=totalFrameNum;
    FQ* framequeue;
    framequeue=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
    framequeue->setUserFps(99999);
    go.pXPS->execPVTobj(PVTScan, &PVTret);
    PVTret.block_till_done();
    framequeue->setUserFps(0);
    pgTGUI->doTilt(-tilt->val, 0, false); //we dont wait for it to tilt back
    _lock_mes.unlock();
    std::lock_guard<std::mutex>lock(_lock_comp);    //wait till other processing is done

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

    std::ofstream wfile ("focus.txt");
    for(int i=0;i!=nFrames;i++) wfile<<i<<" "<<result.at<float>(i)<<"\n";
    wfile.close();
    cv::Point minLoc,maxLoc;
    cv::minMaxLoc(result, &min, &max, &minLoc, &maxLoc);
    double focus=(maxLoc.y-nFrames/2.)*mmPerFrame;
    std::cout<<"Focus is at:"<<focus<<"\n";
    go.pXPS->MoveRelative(XPS::mgroup_XYZF,0,0,focus,-focus);
}
