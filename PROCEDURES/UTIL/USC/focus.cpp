#include "focus.h"
#include "scan.h"
#include "UTIL/img_util.h"
#include "GUI/gui_includes.h"
#include "includes.h"


//GUI

pgFocusGUI::pgFocusGUI(procLockProg& MLP, cv::Rect& sROI, pgScanGUI* pgSGUI, pgMoveGUI* pgMGUI): MLP(MLP), sROI(sROI), pgSGUI(pgSGUI), pgMGUI(pgMGUI){
    init_gui_activation();
    init_gui_settings();
    timer = new QTimer(this);
    timer->setInterval(timer_delay);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(recalculate()));
    connect(pgSGUI, SIGNAL(recalculateCOs()), this, SLOT(recalculate()));
    connect(this, SIGNAL(signalQMessageBoxWarning(QString)), this, SLOT(slotQMessageBoxWarning(QString)));
}
void pgFocusGUI::slotQMessageBoxWarning(QString text){
    wpopup::show(QCursor::pos(),text,60000);
}
void pgFocusGUI::init_gui_activation(){
    gui_activation=new twid(false);
    bFocus=new QPushButton;
    bFocus->setText("ReFocus");
    connect(bFocus, SIGNAL(released()), this, SLOT(onRefocus()));
    gui_activation->addWidget(bFocus);
}
void pgFocusGUI::onRefocus(){
    if(MLP._lock_proc.try_lock()){
        MLP._lock_proc.unlock();
        if(!CORdy) recalculate();
        go.OCL_threadpool.doJob(std::bind(&pgFocusGUI::refocus,this,sROI));
    }
}
bool pgFocusGUI::doRefocus(bool block, cv::Rect ROI, unsigned redoN){
    if(!CORdy) recalculate();
    focusInProgress=true;
    focusFailed=false;
    while(1){
        go.OCL_threadpool.doJob(std::bind(&pgFocusGUI::refocus,this,ROI));
        if(block) while(focusInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        else return false;
        if(focusFailed==true){
            if(redoN<=0) return true;
            redoN--;
            focusFailed=false;
            while(!go.pGCAM->iuScope->connected) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   // if camera has bugged out and has been reset, wait for reconnect
        }else return false;
    }
}
void pgFocusGUI::init_gui_settings(){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    slayout->addWidget(new QLabel("Some settings borrowed from Scan!"));
    selectFocusSetting=new smp_selector("Select focus setting: ", 0, {"Short", "Medium", "Long"});    //should have Nset strings
    conf["selectFocusSetting"]=selectFocusSetting;
    slayout->addWidget(selectFocusSetting);
    for(int i=0;i!=Nset;i++) {
        settingWdg.push_back(new focusSettings(i, this));
        slayout->addWidget(settingWdg.back());
    }
    connect(selectFocusSetting, SIGNAL(changed(int)), this, SLOT(onMenuChange(int)));
    calcL=new QLabel;
    slayout->addWidget(calcL);
    gaussianBlur=new val_selector(21, "Gaussian Blur Sigma: ", 1, 100, 1, 0, {"px"});
    conf["gaussianBlur"]=gaussianBlur;
    slayout->addWidget(gaussianBlur);
    btnSaveNextDebugFocus=new QPushButton("Save Next Debug Values");
    btnSaveNextDebugFocus->setToolTip("Select file for saving debug. The measured values will be saved for the next 'ReFocus'..");
    connect(btnSaveNextDebugFocus, SIGNAL(released()), this, SLOT(onBtnSaveNextDebugFocus()));
    slayout->addWidget(new twid(btnSaveNextDebugFocus));
    onMenuChange(0);
}
void pgFocusGUI::onBtnSaveNextDebugFocus(){
    saveNextFocus=QFileDialog::getSaveFileName(gui_settings, "Select file for saving debug. The measured values will be saved for the next 'ReFocus'..").toStdString();
}

focusSettings::focusSettings(uint num, pgFocusGUI* parent): parent(parent){
    slayout=new QVBoxLayout;
    this->setLayout(slayout);
    range=new val_selector(10., "Scan Range:", 1., 2000., 3, 3 , {"nm","um",QChar(0x03BB),"L"});
    parent->conf[parent->selectFocusSetting->getLabel(num)]["range"]=range;
    connect(range, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(range);
    pphwl=new val_selector(10., "Points Per Period (= half wavelength): ", 0.001, 500., 3);
    parent->conf[parent->selectFocusSetting->getLabel(num)]["pphwl"]=pphwl;
    connect(pphwl, SIGNAL(changed()), parent, SLOT(recalculate()));
    slayout->addWidget(pphwl);
}
void pgFocusGUI::onMenuChange(int index){
    for(int i=0;i!=Nset;i++) settingWdg[i]->setVisible(i==index?true:false);
    range=settingWdg[index]->range;
    pphwl=settingWdg[index]->pphwl;
    recalculate();
}

void pgFocusGUI::recalculate() {
    if(MLP._lock_proc.try_lock()){
        if(MLP._lock_comp.try_lock()){
            std::string report;
            updateCO(report);
            calcL->setText(report.c_str());
            MLP._lock_comp.unlock();
        }
        else timer->start();
        MLP._lock_proc.unlock();
    }
    else timer->start();
}

double pgFocusGUI::vsConv(val_selector* vs){return pgSGUI->vsConv(vs);}

void pgFocusGUI::updateCO(std::string &report){
    if(!go.pGCAM->iuScope->connected || !go.pRPTY->connected) return;
    CORdy=false;
    if(COmeasure==nullptr){
        COmeasure=new CTRL::CO(go.pRPTY);
    }else if(COmeasure->_ctrl!=go.pRPTY){
        delete COmeasure;
        COmeasure=new CTRL::CO(go.pRPTY);
    }else COmeasure->clear();


    double minFPS, expo;
    go.pGCAM->iuScope->get_frame_rate_bounds (&minFPS, &COfps);
    expo=go.pGCAM->iuScope->expo*0.000001; // in s


    readRangeDis=vsConv(range)/1000000;  // in mm
    double velocity=go.pRPTY->getMotionSetting("Z",CTRL::mst_defaultVelocity);
    double acceleration=go.pRPTY->getMotionSetting("Z",CTRL::mst_defaultAcceleration);
    displacementOneFrame=vsConv(pgSGUI->led_wl)/2/vsConv(pphwl)/1000000;       // in mm
    report+=util::toString("One frame displacement = ",displacementOneFrame*1000000," nm.\n");
    totalFrameNum=readRangeDis/displacementOneFrame;   // for simplicity no image taken on one edge
    double timeOneFrame=1./COfps;
    double motionTimeOneFrame;
    bool hasConstantVelocitySegment;
    mcutil::evalAccMotion(displacementOneFrame, acceleration, velocity, &motionTimeOneFrame, &hasConstantVelocitySegment);

    report+=util::toString("One frame time = ",motionTimeOneFrame+expo," s (ideal minimum), as limited by motion time.\n");
    report+=util::toString("One frame time = ",timeOneFrame," s, as limited by framerate.\n");
    report+=util::toString("One frame movement has ",hasConstantVelocitySegment?"a":"no"," constant speed component.\n");

    double timeToEdge;  // the time it takes to move from center(initial position) to edge(scan starting position)
    mcutil::evalAccMotion(readRangeDis/2, acceleration, velocity, &timeToEdge, &hasConstantVelocitySegment);
    report+=util::toString("To scan edge movement time = ",timeToEdge," s.\n");
    report+=util::toString("Total read time (if limited by framerate) = ",2*timeToEdge+timeOneFrame*totalFrameNum," s.\n");
    report+=util::toString("Total number of frames = ",totalFrameNum,".\n");
    report+=util::toString("Read Range Distance = ",readRangeDis," mm.\n");

    COmeasure->addMotion("Z",-readRangeDis/2,0,0,CTRL::MF_RELATIVE);    // move from center to -edge
    COmeasure->addHold("Z",CTRL::he_motion_ontarget);
    for(unsigned i=0;i!=totalFrameNum;i++){
        COmeasure->startTimer("timer",1./COfps+pgSGUI->triggerAdditionalDelay->val*0.001);      // in case motion is completed before the camera is ready for another frame
        COmeasure->pulseGPIO("trigCam",0.00001);            // 10us
        if(expo>0.0001) COmeasure->addDelay(expo-0.00001);  // keep stationary during exposure
        COmeasure->addMotion("Z",displacementOneFrame,0,0,CTRL::MF_RELATIVE);
        COmeasure->addHold("Z",CTRL::he_motion_ontarget);
        COmeasure->addHold("timer",CTRL::he_timer_done);
    }
    COmeasure->addMotion("Z",-readRangeDis/2,0,0,CTRL::MF_RELATIVE);    // move from +edge to center

    CORdy=true;
}

void pgFocusGUI::refocus(cv::Rect ROI){
    if(!go.pGCAM->iuScope->connected || !go.pRPTY->connected) {focusFailed=true; focusInProgress=false; return;}
    if(!CORdy) {focusFailed=true; focusInProgress=false; return;}

    if(ROI.width==0) ROI=sROI;
    if(ROI.width==0) ROI=cv::Rect(0,0,go.pGCAM->iuScope->camCols,go.pGCAM->iuScope->camRows);

    unsigned nFrames=totalFrameNum;
    FQ* framequeue;

    MLP._lock_proc.lock();    //wait for other measurements to complete
    pgMGUI->chooseObj(true);    // switch to mirau
    go.pGCAM->iuScope->set_trigger("Line1");
    framequeue=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
    framequeue->setUserFps(2*COfps);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    while(framequeue->getFullNumber()) framequeue->freeUserMat();   // remove any remaining non-trig frames
    COmeasure->execute();

    std::lock_guard<std::mutex>lock(MLP._lock_comp);

    std::vector<double> mean; mean.reserve(nFrames);
    std::vector<double> stdd; stdd.reserve(nFrames);
    std::vector<double> tmpv; tmpv.reserve(nFrames);
    cv::Scalar smean,sstdd;

    unsigned frames=0;
    while(1) {
        unsigned time=0;
        while(framequeue->getUserMat()==nullptr){
            time+=(frames==0)?10:100;  // for first frame we put 10x timeout
            if(time>=timeout) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if(time>=timeout) break;
        frames++;

        cv::meanStdDev((*framequeue->getUserMat())(ROI), smean, sstdd);
        mean.emplace_back(smean[0]);
        stdd.emplace_back(sstdd[0]);
        framequeue->freeUserMat();

        MLP.progress_proc=100./nFrames*frames;
        MLP.progress_comp=0.9*MLP.progress_proc;
        if(frames==nFrames) break;
    }
    framequeue->setUserFps(0);
    go.pGCAM->iuScope->set_trigger("none");
    MLP._lock_proc.unlock();

    if(frames!=nFrames){
        Q_EMIT signalQMessageBoxWarning(QString::fromStdString(util::toString("ERROR: took ",frames," frames, expected ",nFrames,".")));
        focusFailed=true;
    }else{
        double MEAN=cv::mean(mean)[0];
        cv::subtract(mean,MEAN,tmpv);
        cv::add(tmpv,stdd,tmpv);
        double sigma=gaussianBlur->val;
        int ksize=sigma*5;
        if(!(ksize%2)) ksize++;
        cv::GaussianBlur(tmpv, tmpv, cv::Size(ksize, 1), sigma, 0);
        cv::Point maxLoc;
        cv::minMaxLoc(tmpv, nullptr, nullptr, nullptr, &maxLoc);

        if(!saveNextFocus.empty()) {
            std::ofstream wfile(util::toString(saveNextFocus));
            wfile<<"#frame mean stddev (mean-mean(mean)) (mean-mean(mean)+stddev) blured\n";
            for(int i=0;i!=nFrames;i++){
                wfile<<i<<" "<<mean[i]<<" "<<stdd[i]<<" "<<mean[i]-MEAN<<" ";
                wfile<<mean[i]-MEAN+stdd[i]<<" "<<tmpv[i]<<"\n";
            }
            wfile.close();
        }

        double Zcor=(maxLoc.x-nFrames/2.)*displacementOneFrame;
        if(abs(Zcor)>readRangeDis/2){
            Q_EMIT signalQMessageBoxWarning(QString::fromStdString(util::toString("ERROR: abs of the calculated correction is larger than half the read range distance: ",Zcor," vs ",readRangeDis/2," . Did not apply correction.")));
            focusFailed=true;
        }
        else go.pRPTY->motion("Z",Zcor,0,0,CTRL::MF_RELATIVE);
    }
    MLP.progress_comp=100;
    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);
    focusInProgress=false;
}
