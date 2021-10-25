#include "focus.h"
#include "scan.h"
#include "UTIL/img_util.h"
#include "GUI/gui_includes.h"
#include "includes.h"


//GUI

pgFocusGUI::pgFocusGUI(procLockProg& MLP, pgScanGUI* pgSGUI, pgMoveGUI* pgMGUI): MLP(MLP), pgSGUI(pgSGUI), pgMGUI(pgMGUI){
    init_gui_activation();
    init_gui_settings();
    timer = new QTimer(this);
    timer->setInterval(timer_delay);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(recalculate()));
    connect(pgSGUI, SIGNAL(recalculateCOs()), this, SLOT(recalculate()));
    connect(this, SIGNAL(signalQMessageBoxWarning(QString, QString)), this, SLOT(slotQMessageBoxWarning(QString, QString)));
}
void pgFocusGUI::slotQMessageBoxWarning(QString title, QString text){
    QMessageBox::critical(gui_activation, title, text);
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
        go.OCL_threadpool.doJob(std::bind(&pgFocusGUI::refocus,this));
    }
}
void pgFocusGUI::doRefocus(bool block){
    if(MLP._lock_proc.try_lock()){
        MLP._lock_proc.unlock();
        if(!CORdy) recalculate();
        focusInProgress=true;
        go.OCL_threadpool.doJob(std::bind(&pgFocusGUI::refocus,this));
        if(block) while(focusInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
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
    btnSaveNextDebugFocus->setToolTip("Select Folder for Debug. The measured values will be saved for the next 'ReFocus'..");
    connect(btnSaveNextDebugFocus, SIGNAL(released()), this, SLOT(onBtnSaveNextDebugFocus()));
    slayout->addWidget(new twid(btnSaveNextDebugFocus));
    onMenuChange(0);
}
void pgFocusGUI::onBtnSaveNextDebugFocus(){
    saveNextFocus=QFileDialog::getExistingDirectory(gui_settings, "Select Folder for Debug. The measured values will be saved for the next 'ReFocus'..").toStdString();
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
        COmeasure->pulseGPIO("trigCam",0.0001);             // 100us
        if(expo>0.0001) COmeasure->addDelay(expo-0.0001);   // keep stationary during exposure
        COmeasure->addMotion("Z",displacementOneFrame,0,0,CTRL::MF_RELATIVE);
        COmeasure->addHold("Z",CTRL::he_motion_ontarget);
        COmeasure->addHold("timer",CTRL::he_timer_done);
    }
    COmeasure->addMotion("Z",-readRangeDis/2,0,0,CTRL::MF_RELATIVE);    // move from +edge to center

    CORdy=true;
}

void pgFocusGUI::refocus(){
    if(!go.pGCAM->iuScope->connected || !go.pRPTY->connected) {focusInProgress=false; return;}
    if(!CORdy) {focusInProgress=false; return;}

    cv::Rect scanROI;
    if(pgSGUI->isROI) scanROI=cv::Rect(pgSGUI->ROI[0],pgSGUI->ROI[1],pgSGUI->ROI[2],pgSGUI->ROI[3]);
    else scanROI=cv::Rect(0,0,go.pGCAM->iuScope->camCols,go.pGCAM->iuScope->camRows);

    unsigned nFrames=totalFrameNum;
    FQ* framequeue;
    {   std::lock_guard<std::mutex>lock(MLP._lock_proc);    //wait for other measurements to complete
        pgMGUI->chooseObj(true);    // switch to mirau

        go.pGCAM->iuScope->set_trigger("Line1");
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // let it switch to trigger
        framequeue=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
        framequeue->setUserFps(COfps);
        COmeasure->execute();

        unsigned cframes, lcframes=0;
        unsigned time=0;
        while(1) {
            cframes=framequeue->getFullNumber();
            if(cframes>=nFrames){
                MLP.progress_proc=100;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if(lcframes==cframes){
                if(cframes==0) time+=10;    // for first frame we put 10x timeout
                else time+=100;
            }else time=0;
            lcframes=cframes;
            if(time>=timeout) break;

            MLP.progress_proc=100./nFrames*cframes;
        }

        framequeue->setUserFps(0);
        go.pGCAM->iuScope->set_trigger("none");
    }
    {   std::lock_guard<std::mutex>lock(MLP._lock_comp);    //wait till other processing is done

        if(framequeue->getFullNumber()!=nFrames){
            Q_EMIT signalQMessageBoxWarning("Error", QString::fromStdString(util::toString("ERROR: took ",framequeue->getFullNumber()," frames, expected ",nFrames,".")));
            go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);
            focusInProgress=false;
            return;
        }

        unsigned nRows=(*framequeue->getUserMat(0))(scanROI).rows;
        unsigned nCols=(*framequeue->getUserMat(0))(scanROI).cols;

        cv::UMat pixSum(1, nFrames, CV_64F, cv::Scalar(0));
        cv::Mat mat2D(nFrames, nCols, CV_64F);

        if(!saveNextFocus.empty()) {std::ofstream wfile(util::toString(saveNextFocus,"/","test-avgAllPix.dat")); for(int i=0;i!=nFrames;i++) wfile<<cv::mean((*framequeue->getUserMat(i))(scanROI))[0]<<"\n"; wfile.close();}

        std::chrono::time_point<std::chrono::system_clock> A=std::chrono::system_clock::now();
        MLP.progress_comp=0;
        for(int k=0;k!=nRows;k++){      //Processing row by row
            for(int i=0;i!=nFrames;i++)
                (*framequeue->getUserMat(i))(scanROI).row(k).copyTo(mat2D.row(i));
            cv::UMat Umat2D;                    //row - frameN, col - FrameCol
            cv::UMat temp0, temp1;
            mat2D.copyTo(Umat2D);

            if(k==nRows/2 && !saveNextFocus.empty()) {std::ofstream wfile(util::toString(saveNextFocus,"/","stage0-barePix.dat")); for(int j=0;j!=mat2D.rows;j++) wfile<<mat2D.at<double>(j, nCols/2)<<"\n"; wfile.close();}
            cv::reduce(mat2D, temp0, 0, cv::REDUCE_AVG);
            cv::repeat(temp0, nFrames, 1, temp1);

            cv::subtract(Umat2D,temp1,Umat2D);                      // removed the average
            if(k==nRows/2 && !saveNextFocus.empty()) {Umat2D.copyTo(mat2D); std::ofstream wfile(util::toString(saveNextFocus,"/","stage1-avgRmv.dat")); for(int j=0;j!=mat2D.rows;j++) wfile<<mat2D.at<double>(j, nCols/2)<<"\n"; wfile.close();}
            cv::absdiff(Umat2D, cv::Scalar::all(0), Umat2D);        // get abs
            if(k==nRows/2 && !saveNextFocus.empty()) {Umat2D.copyTo(mat2D); std::ofstream wfile(util::toString(saveNextFocus,"/","stage2-abs.dat")); for(int j=0;j!=mat2D.rows;j++) wfile<<mat2D.at<double>(j, nCols/2)<<"\n"; wfile.close();}

            cv::reduce(Umat2D, temp0, 1, cv::REDUCE_AVG);           //avg pixels
            cv::add(pixSum,temp0.t(),pixSum);
            MLP.progress_comp=99*(k+1)/nRows;
        }
        if(!saveNextFocus.empty()) {pixSum.copyTo(mat2D); std::ofstream wfile(util::toString(saveNextFocus,"/","stage3-avgAll.dat")); for(int j=0;j!=mat2D.cols;j++) wfile<<mat2D.at<double>(0, j)<<"\n"; wfile.close();}
        double sigma=gaussianBlur->val;
        int ksize=sigma*5;
        if(!(ksize%2)) ksize++;
        cv::GaussianBlur(pixSum, pixSum, cv::Size(ksize, 1), sigma, 0);
        if(!saveNextFocus.empty()) {pixSum.copyTo(mat2D); std::ofstream wfile(util::toString(saveNextFocus,"/","stage4-blur.dat")); for(int j=0;j!=mat2D.cols;j++) wfile<<mat2D.at<double>(0, j)<<"\n"; wfile.close();saveNextFocus.clear();}
        cv::Point maxLoc;
        cv::minMaxLoc(pixSum, nullptr, nullptr, nullptr, &maxLoc);
        MLP.progress_comp=100;
        std::cerr<<"res: "<<maxLoc.x<<" "<<nFrames<<"\n";
        std::chrono::time_point<std::chrono::system_clock> B=std::chrono::system_clock::now();
        std::cerr<<"Calculation took "<<std::chrono::duration_cast<std::chrono::microseconds>(B - A).count()<<" microseconds\n";

        go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);

        double Zcor=(maxLoc.x-nFrames/2.)*displacementOneFrame;
        if(abs(Zcor)>readRangeDis/2)
            Q_EMIT signalQMessageBoxWarning("Error", QString::fromStdString(util::toString("ERROR: abs of the calculated correction is larger than half the read range distance: ",Zcor," vs ",readRangeDis/2," . Did not apply correction.")));
        else go.pRPTY->motion("Z",Zcor,0,0,CTRL::MF_RELATIVE);
    }
    focusInProgress=false;
}
