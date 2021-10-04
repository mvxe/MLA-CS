#include "beamanl.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include "GUI/TAB_CAMERA/other_settings.h"
//#include <dlib/optimization.h>

pgBeamAnalysis::pgBeamAnalysis(mesLockProg& MLP, pgMoveGUI* pgMGUI, pgScanGUI* pgSGUI): MLP(MLP), pgMGUI(pgMGUI), pgSGUI(pgSGUI){
    conf["saveWBCX"]=_writeBeamCenterOfsX;
    conf["saveWBCY"]=_writeBeamCenterOfsY;

    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    btnReset=new QPushButton("Reset Center");
    connect(btnReset, SIGNAL(released()), this, SLOT(onBtnReset()));
//    selMaxRoundnessDev=new val_selector(0.1,"pgBeamAnalysis_selMaxRoundnessDev", "Maximum Roundness Deviation:",0.001,1,3);
//    selMaxRoundnessDev->setToolTip("Will exclude all Elipses that are too elliptical: where one arm is this factor larger than the other.");
//    selCannyThreshL=new val_selector( 50,"pgBeamAnalysis_selCannyThreshL", "Canny Lower Threshold:",0,255,0);
//    selCannyThreshU=new val_selector(100,"pgBeamAnalysis_selCannyThreshU", "Canny Upper Threshold:",0,255,0);
//    selMinPixNum=new val_selector(10,"pgBeamAnalysis_selMinPixNum", "Min Ellipse Pixel Number:",1,10000,0);
//    selMinPixNum->setToolTip("Only Connected Pixel Groups (after Canny) With This Many Pixels or More Will be Considered for Ellipse Fitting.");
//    btnSaveNextDebug=new QPushButton("Save Next Debug images");
//    btnSaveNextDebug->setToolTip("Select Folder for Debug. Images of Every Step of the Process Will be Saved For the Next 'Get Writing Beam Center'.");
//    connect(btnSaveNextDebug, SIGNAL(released()), this, SLOT(onBtnSaveNextDebug()));
    extraOffsX=new val_selector( 0, "X Extra Offset:",-100,100,2,0,{"um"});
    conf["extraOffsX"]=extraOffsX;
    extraOffsY=new val_selector( 0, "Y Extra Offset:",-100,100,2,0,{"um"});
    conf["extraOffsY"]=extraOffsY;
    extraOffsX->setToolTip("In case the two beams do not overlap perfectly, this can be used to correct this. Redo beam centering after changing this value.");
    extraOffsY->setToolTip(extraOffsX->toolTip());
//    cameraExposure=new val_selector(1, "pgBeamAnalysis_cameraExposure", "Camera Exposure: ", 0, 9999999, 3, 0, {"us"});
    selThresh=new val_selector( 128, "Threshold:",0,255,0);
    conf["selThresh"]=selThresh;
//    avgNum=new val_selector( 10,"pgBeamAnalysis_avgNum", "Avg. minimum this many images:",1,1000,0);

    wideFrames=new val_selector( 100, "Number of Frames for Wide Scan: ",10,1000,0);
    conf["wideFrames"]=wideFrames;
    wideRange= new val_selector( 0.5, "Scan Range for Wide Scan: ",0.001,1,3,0,{"mm"});
    conf["wideRange"]=wideRange;
    accuFrames=new val_selector( 100, "Number of Frames for Accurate Scan: ",10,1000,0);
    conf["accuFrames"]=accuFrames;
    accuRange= new val_selector( 0.05, "Scan Range for Accurate Scan: ",0.001,1,3,0,{"mm"});
    conf["accuRange"]=accuRange;
    doExtraFocusMesDifDir=new checkbox_gs(false,"Do extra focus measurement from opposite side.");
    conf["doExtraFocusMesDifDir"]=doExtraFocusMesDifDir;
    btnSaveNextDebugFocus=new QPushButton("Save Next Debug Intensities");
    btnSaveNextDebugFocus->setToolTip("Select Folder for Debug. The measured intensities will be saved for the next 'Get Writing Beam Focus'..");
    connect(btnSaveNextDebugFocus, SIGNAL(released()), this, SLOT(onBtnSaveNextDebugFocus()));
    extraFocusOffset=new val_selector( 0,"Extra Focus Offset (Laser Beam Foci differece):",-1,1,4,0,{"mm"});
    conf["extraFocusOffset"]=extraFocusOffset;
    extraFocusOffset->setToolTip("Set this to the value you obtain from calibration: it should be equal to the difference between the reference laser beam focus and the writing laser beam focus.");
    extraFocusOffsetVal=&extraFocusOffset->val;

//    method_selector=new twds_selector("pgBeamAnalysis_methodindex",0,"Select method:", "Simple", false, false, false);
//    methodSimple=new QWidget;
//    methodSimpleLayout=new QVBoxLayout;
//    methodSimple->setLayout(methodSimpleLayout);
//    methodEllipsse=new QWidget;
//    methodEllipsseLayout=new QVBoxLayout;
//    methodEllipsse->setLayout(methodEllipsseLayout);

//    method_selector->addWidget(methodSimple, "Simple");
//    method_selector->addWidget(methodEllipsse, "Ellipses");
//    method_selector->doneAddingWidgets();

//    slayout->addWidget(new QLabel("Get beam center:"));
//    slayout->addWidget(method_selector);
//        methodSimpleLayout->addWidget(selThresh);
//        methodSimpleLayout->addWidget(avgNum);
//        methodEllipsseLayout->addWidget(selCannyThreshL);
//        methodEllipsseLayout->addWidget(selCannyThreshU);
//        methodEllipsseLayout->addWidget(selMinPixNum);
//        methodEllipsseLayout->addWidget(selMaxRoundnessDev);
//    slayout->addWidget(new twid(btnSaveNextDebug));


    slayout->addWidget(wideFrames);
    slayout->addWidget(wideRange);
    slayout->addWidget(accuFrames);
    slayout->addWidget(accuRange);
    slayout->addWidget(selThresh);
    slayout->addWidget(doExtraFocusMesDifDir);
    slayout->addWidget(new twid(btnSaveNextDebugFocus));
    slayout->addWidget(new hline);
    slayout->addWidget(new twid(btnReset));
    slayout->addWidget(extraOffsX);
    slayout->addWidget(extraOffsY);
    slayout->addWidget(extraFocusOffset);

    QLabel *lbl0=new QLabel("Extra Offset calib: ( Set X,Y ex.of. to 0 -> Get Writing Beam Focus -> Write Pulse -> Click Get Offset (below) -> move cursor to written spot -> Click Get Offset (below) again -> Get Writing Beam Focus )"); lbl0->setWordWrap(true);
    slayout->addWidget(lbl0);
    exOfsCalibBtn=new QPushButton;
    exOfsCalibBtn->setText("Get Offset");
    exOfsCalibBtn->setCheckable(true);
    connect(exOfsCalibBtn, SIGNAL(toggled(bool)), this, SLOT(onExOfsCalibBtn(bool)));
    slayout->addWidget(new twid(exOfsCalibBtn));

//    slayout->addWidget(cameraExposure);

    gui_activation=new QWidget;
    alayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);
//    btnGetCenter=new QPushButton("Get Writing Beam Center");
//    connect(btnGetCenter, SIGNAL(released()), this, SLOT(getWritingBeamCenter()));
//    alayout->addWidget(new twid(btnGetCenter));
//    btnGetCenterFocus=new QPushButton("Get Writing Beam Focus");
//    connect(btnGetCenterFocus, SIGNAL(released()), this, SLOT(correctWritingBeamFocus()));
//    alayout->addWidget(new twid(btnGetCenterFocus));
}
//void pgBeamAnalysis::onBtnSaveNextDebug(){
//    saveNext=QFileDialog::getExistingDirectory(this, "Select Folder for Debug. Images of Every Step of the Process Will be Saved For the Next 'Get Writing Beam Center'.").toStdString();
//}
void pgBeamAnalysis::onBtnSaveNextDebugFocus(){
    saveNextFocus=QFileDialog::getExistingDirectory(this, "Select Folder for Debug. The measured intensities will be saved for the next 'Get Writing Beam Focus'..").toStdString();
    if(!saveNextFocus.empty()) numSave=2;
}
void pgBeamAnalysis::onBtnReset(){
    pgMGUI->move(-_writeBeamCenterOfsX*pgMGUI->getNmPPx(1)/1000000,-_writeBeamCenterOfsY*pgMGUI->getNmPPx(1)/1000000,0);
    _writeBeamCenterOfsX=0;
    _writeBeamCenterOfsY=0;
}

//bool pgBeamAnalysis::sortSpot(spot i,spot j) {return (i.dd<j.dd);}
//void pgBeamAnalysis::solveEllips(cv::Mat& src, int i,std::vector<spot>& spots,int& jobsdone){
//    cv::Mat cmpMat;
//    cv::compare(src, i, cmpMat, cv::CMP_EQ);
//    std::vector<cv::Point> locations;   // output, locations of non-zero pixels
//    cv::findNonZero(cmpMat, locations);
//    if(locations.size()<selMinPixNum->val) {std::lock_guard<std::mutex>lock(spotLock);jobsdone++;return;}
//    cv::RotatedRect tmp=fitEllipse(locations);
//    std::lock_guard<std::mutex>lock(spotLock);
//    if(std::abs(tmp.size.width-tmp.size.height)<=std::min(tmp.size.width,tmp.size.height)*selMaxRoundnessDev->val && std::min(tmp.size.width,tmp.size.height)>=1 && std::max(tmp.size.width,tmp.size.height)<=std::min(cmpMat.cols,cmpMat.rows))
//        spots.push_back({tmp.center.x,tmp.center.y,(tmp.size.width+tmp.size.height)/4,0,0});
//    jobsdone++;
//}
//void pgBeamAnalysis::getWritingBeamCenter(){
//    float x,y,r,dx,dy;
//    std::chrono::time_point<std::chrono::system_clock> A=std::chrono::system_clock::now();
//    getCalibWritingBeam(&r, &dx, &dy);
////    std::cerr<<"X mean: "<<writeBeamCenterOfsX<<", stdDev: "<< dx<<"\n";
////    std::cerr<<"Y mean: "<<writeBeamCenterOfsY<<", stdDev: "<< dy<<"\n";
//    std::cerr<<"X: "<<writeBeamCenterOfsX<<"\n";
//    std::cerr<<"Y: "<<writeBeamCenterOfsY<<"\n";
//    std::cerr<<"Radius: "<<r<<"\n";
//    std::chrono::time_point<std::chrono::system_clock> B=std::chrono::system_clock::now();
//    std::cerr<<"getWritingBeamCenter took "<<std::chrono::duration_cast<std::chrono::microseconds>(B - A).count()<<" microseconds\n";
//}

void pgBeamAnalysis::onExOfsCalibBtn(bool state){
    XPS::raxis tmp=go.pXPS->getPos(XPS::mgroup_XYZF);
    if(state){
        X_start=tmp.pos[0];
        Y_start=tmp.pos[1];
    }else{
        extraOffsX->setValue((X_start-tmp.pos[0])*1000);
        extraOffsY->setValue((Y_start-tmp.pos[1])*1000);
    }
}


void pgBeamAnalysis::ctrlRedLaser(bool state){
    std::vector<uint32_t> commands;
    commands.push_back(CQF::GPIO_MASK(0x80,0,0x00));
    commands.push_back(CQF::GPIO_DIR (0x00,0,0x00));
    commands.push_back(CQF::GPIO_VAL (state?0x80:0x00,0,0x00));
    go.pRPTY->A2F_write(1,commands.data(),commands.size());
    commands.clear();
}
//void pgBeamAnalysis::armRedLaser(){
//    std::vector<uint32_t> commands;
//    commands.push_back(CQF::GPIO_MASK(0x40,0,0));
//    commands.push_back(CQF::GPIO_DIR (0x40,0,0));
//    commands.push_back(CQF::W4TRIG_GPIO(CQF::HIGH,false,0x40,0x00));
//    commands.push_back(CQF::GPIO_MASK(0x80,0,0x00));
//    commands.push_back(CQF::GPIO_DIR (0x00,0,0x00));
//    commands.push_back(CQF::GPIO_VAL (0x80,0,0x00));
//    go.pRPTY->A2F_write(1,commands.data(),commands.size());
//    commands.clear();
//}

bool pgBeamAnalysis::waitTillLEDIsOff(FQ* framequeueDisp){
    // first we take frames until we hit the first one that has the LED turned off This is recognized using criteria below:
    const int borderWidth=10;
    const uchar lIntThresh=20;      // 1/25 of max int
    const float lOuterRatio=0.75;   // at least lOuterRatio of the pixels within the borderWidth pixel width border must be smaller than lIntThresh
//    const uchar uIntThresh=50;      // 1/5 of max int
//    const float uInnerRatio=0.000001; // at least uInnerRatio of the pixels within the borderWidth pixel width border must be larger than uIntThresh
    const int maxTriesFrames=100;  // give up after checking this many frames

    framequeueDisp->setUserFps(9999,maxTriesFrames);
    for(int i=0;i!=maxTriesFrames+1;i++){
        if(i==maxTriesFrames)
            return true;

        while(framequeueDisp->getUserMat()==nullptr) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        cv::Mat mask(framequeueDisp->getUserMat()->rows,framequeueDisp->getUserMat()->cols,CV_8U,cv::Scalar(0));
        cv::rectangle(mask, cv::Point(borderWidth,borderWidth), cv::Point(mask.cols-borderWidth,mask.rows-borderWidth), cv::Scalar(255), cv::FILLED);
        cv::Mat temp;
        cv::compare(*framequeueDisp->getUserMat(),lIntThresh,temp,cv::CMP_LT);
        temp.setTo(0,mask);
        int lNum=cv::countNonZero(temp);
        if((float)lNum/(mask.cols*mask.rows-(mask.cols-2*borderWidth)*(mask.rows-2*borderWidth))<lOuterRatio) {framequeueDisp->freeUserMat(); continue;}
//        cv::compare(*framequeueDisp->getUserMat(),uIntThresh,temp,cv::CMP_GE);
//        cv::bitwise_not(mask,mask);
//        temp.setTo(0,mask);
//        int uNum=cv::countNonZero(temp);
//        if((float)uNum/(mask.cols*mask.rows-(mask.cols-2*borderWidth)*(mask.rows-2*borderWidth))<uInnerRatio) {framequeueDisp->freeUserMat(); continue;}
        break;
    }
    return false;
}

    // replaced by getCalibWritingBeamRange:
//bool pgBeamAnalysis::getCalibWritingBeam(float* r, float* dx, float* dy, bool correct){
//    bool failed{true};

//    go.pXPS->setGPIO(XPS::iuScopeLED,false);
//    ctrlRedLaser(true);
//    FQ* framequeueDisp=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
//    if(waitTillLEDIsOff(framequeueDisp)){
//        framequeueDisp->setUserFps(0,1);
//        ctrlRedLaser(false);
//        go.pXPS->setGPIO(XPS::iuScopeLED,true);
//        go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeueDisp);
//        *r=-1;
//        if(dx!=nullptr) *dx=0;
//        if(dy!=nullptr) *dy=0;

//        if(correct==false)
//            return failed;  //no complain if no correct
//        std::string text=util::toString("pgBeamAnalysis::getCalibWritingBeam failed.\n");
//        QMessageBox::critical(this, "Error", QString::fromStdString(text));
//        std::cerr<<text;
//        return failed;
//    }

//    if(method_selector->index==0) while(framequeueDisp->getFullNumber()<avgNum->val) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
//    framequeueDisp->setUserFps(0,1);
//    ctrlRedLaser(false);
//    go.pXPS->setGPIO(XPS::iuScopeLED,true);

//    //do proc

//    if(method_selector->index==1){      //Ellipses
//        std::vector<cv::Vec3f> circles;
//        cv::Mat src;
//        if(!saveNext.empty()) cv::imwrite(util::toString(saveNext,"/","0_src.png"), *framequeueDisp->getUserMat());
//        cv::Canny(*framequeueDisp->getUserMat(),src, selCannyThreshL->val, selCannyThreshU->val);
//        if(!saveNext.empty()) cv::imwrite(util::toString(saveNext,"/","1_canny.png"), src);
//        int N=cv::connectedComponents(src,src,8,CV_16U);

//        std::vector<spot> spots;
//        int jobsdone=0;
//        for(int i=1;i!=N;i++){  //we skip background 0
//            go.OCL_threadpool.doJob(std::bind(&pgBeamAnalysis::solveEllips,this,src,i,std::ref(spots),std::ref(jobsdone)));
//        }
//        for(;;){
//            QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
//            std::lock_guard<std::mutex>lock(spotLock);
//            if(jobsdone==N-1) break;
//        }

//        bool once{false};
//    start: once=!once;
//        float mean[3]{0,0,0};
//        float stdDev[3]{0,0,0};
//        for(int i=0;i!=spots.size();i++){
//            mean[0]+=spots[i].x;
//            mean[1]+=spots[i].y;
//            mean[2]+=spots[i].r;
//        }
//        mean[0]/=spots.size();
//        mean[1]/=spots.size();
//        mean[2]/=spots.size();
//        for(int i=0;i!=spots.size();i++){
//            spots[i].dx=abs(spots[i].x-mean[0]);
//            spots[i].dy=abs(spots[i].y-mean[1]);
//            spots[i].dd=sqrtf(powf(spots[i].dx,2)+powf(spots[i].dy,2));
//            stdDev[0]+=powf(spots[i].dx,2);
//            stdDev[1]+=powf(spots[i].dy,2);
//        }
//        stdDev[0]=sqrt(stdDev[0]/(spots.size()-1));
//        stdDev[1]=sqrt(stdDev[1]/(spots.size()-1));
//        stdDev[2]=sqrtf(powf(stdDev[0],2)+powf(stdDev[1],2));
//        std::sort(spots.begin(), spots.end(), &pgBeamAnalysis::sortSpot);

//        const float SDR=1;    //within 2SD is fine
//        while(!spots.empty()) if(spots.back().dd>SDR*stdDev[2]) spots.pop_back(); else break;
//        if(!spots.empty() && once) goto start;

//    //    std::chrono::time_point<std::chrono::system_clock> B=std::chrono::system_clock::now();

//        cv::Mat pol, pol1D;
//        cv::linearPolar(*framequeueDisp->getUserMat(), pol, cv::Point(mean[0],mean[1]), framequeueDisp->getUserMat()->rows, cv::INTER_AREA + cv::WARP_FILL_OUTLIERS);
//        cv::reduce(pol, pol1D, 0, cv::REDUCE_AVG, CV_32F);
//        cv::Point maxp; double ignore; cv::Point ignore2;
//        int ofs=((int)mean[2]<pol1D.cols-1)?((int)mean[2]):(pol1D.cols-1);
//        cv::minMaxLoc(pol1D.colRange(ofs,pol1D.cols-1),&ignore,&ignore,&ignore2,&maxp);     //we ignore the peaks near the center, we want the outer bright ring as the reference

//    //    std::chrono::time_point<std::chrono::system_clock> C=std::chrono::system_clock::now();

//        if(!saveNext.empty()){
//            cv::Mat img;
//            cv::cvtColor(*framequeueDisp->getUserMat(), img, cv::COLOR_GRAY2BGR);
//            for(int i=0;i!=spots.size();i++){
//                cv::circle( img, cv::Point(spots[i].x,spots[i].y), 3, cv::Scalar(0,255,0), -1, 8, 0 );
//                cv::circle( img, cv::Point(spots[i].x,spots[i].y), spots[i].r, cv::Scalar(0,0,255), 1, 8, 0 );
//            }
//            cv::imwrite(util::toString(saveNext,"/","2_final.png"), img);
//            saveNext.clear();
//        }

//        mean[0]-=src.cols/2;
//        mean[1]-=src.rows/2;
//        if(r!=nullptr) *r=ofs+maxp.x;
////        if(dx!=nullptr) *dx=stdDev[0];
////        if(dy!=nullptr) *dy=stdDev[1];
//        if(!saveNext.empty()){
//            std::ofstream wfile(util::toString(saveNext,"/","rad.dat"));
//            for(int i=0;i!=pol1D.cols;i++)
//                wfile.write(reinterpret_cast<char*>(&(pol1D.at<float>(i))),sizeof(float));
//            wfile.close();
//        }

//        //    std::chrono::time_point<std::chrono::system_clock> D=std::chrono::system_clock::now();
//        //    std::cerr<<"Total operation took "<<std::chrono::duration_cast<std::chrono::microseconds>(D - A).count()<<" microseconds\n";
//        //    std::cerr<<"First operation took "<<std::chrono::duration_cast<std::chrono::microseconds>(B - A).count()<<" microseconds\n";
//        //    std::cerr<<"Second operation took "<<std::chrono::duration_cast<std::chrono::microseconds>(C - B).count()<<" microseconds\n";
//        //    std::cerr<<"Third operation took "<<std::chrono::duration_cast<std::chrono::microseconds>(D - C).count()<<" microseconds\n";

//        failed=!((ofs+maxp.x)>0 && mean[0]>-src.cols/2 && mean[0]<src.cols/2 && mean[1]>-src.rows/2 && mean[1]<src.rows/2);   //coparisons with NAN are allways false!
//        if(correct && !failed){
//            pgMGUI->move((mean[0]+extraOffsX->val-_writeBeamCenterOfsX)*pgMGUI->getNmPPx()/1000000,(mean[1]+extraOffsY->val-_writeBeamCenterOfsY)*pgMGUI->getNmPPx()/1000000,0,0);        //correct position
//            _writeBeamCenterOfsX=mean[0]+extraOffsX->val;
//            _writeBeamCenterOfsY=mean[1]+extraOffsY->val;
//        }
//        if(dx!=nullptr) *dx=mean[0]+extraOffsX->val;
//        if(dy!=nullptr) *dy=mean[1]+extraOffsY->val;


//    }else if(method_selector->index==0){        //Simple
//        double avgOffsX{0}, avgOffsY{0}, avgCen{0};
//        int N=0;

//        int cols{0},rows{0};
//        while(framequeueDisp->getUserMat()!=nullptr){
//            cv::Mat src;
//            cv::threshold(*framequeueDisp->getUserMat(), src, selThresh->val, 255, cv::THRESH_TOZERO);   //we first set all that are below threshold to 0
//            cv::Moments m=cv::moments(src,true);

//            if(cols==0){cols=src.cols; rows=src.rows;}
//            framequeueDisp->freeUserMat();
//            N++;
//            avgOffsX+=m.m10/m.m00-src.cols/2;
//            avgOffsY+=m.m01/m.m00-src.rows/2;
//            avgCen+=m.m00;
//        }
//        double offsX=avgOffsX/N;
//        double offsY=avgOffsY/N;
//        if(r!=nullptr) *r=avgCen/N;
////        std::cerr<<"Num "<<N<< "\n";    //area
////        std::cerr<< offsX<<" "<<offsY<< "\n";   //X,Y
////        std::cerr<<avgCen/N<< "\n";    //area
//        failed=!(avgCen/N>0 && offsX>-cols/2 && offsX<cols/2 && offsY>-rows/2 && offsY<rows/2);   //coparisons with NAN are allways false!
//        if(correct && !failed){
//            pgMGUI->move((offsX+extraOffsX->val-_writeBeamCenterOfsX)*pgMGUI->getNmPPx()/1000000,(offsY+extraOffsY->val-_writeBeamCenterOfsY)*pgMGUI->getNmPPx()/1000000,0,0);
//            _writeBeamCenterOfsX=offsX+extraOffsX->val;
//            _writeBeamCenterOfsY=offsY+extraOffsY->val;
//        }
//        if(dx!=nullptr) *dx=offsX+extraOffsX->val;
//        if(dy!=nullptr) *dy=offsY+extraOffsY->val;

//    }

//    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeueDisp);
//    return failed;
//}


//typedef dlib::matrix<double,1,1> input_vector;
//typedef dlib::matrix<double,4,1> parameter_vector;
//double gaussResidual(const std::pair<input_vector, double>& data, const parameter_vector& params){
//    double x0=params(0);  double x=data.first(0);
//    double a=params(1);
//    double w=params(2);
//    double i0=params(3);
//    double model=i0+a*exp(-(pow(x-x0,2))/2/pow(w,2));
//    return model-data.second;
//}
void pgBeamAnalysis::getCalibWritingBeamRange(double* rMinLoc, double *xMin, double *yMin, int frames, double range, bool flipDir){
    PVTobj* PVTScan=go.pXPS->createNewPVTobj(XPS::mgroup_XYZF, util::toString("camera_getCalibWritingBeamRange.txt").c_str());
    exec_ret PVTret;

    if(!go.pGCAM->iuScope->connected || !go.pXPS->connected) return;
    double minFPS,maxFPS;
    go.pGCAM->iuScope->get_frame_rate_bounds (&minFPS, &maxFPS);

    double readTime=frames/maxFPS;
    double readVelocity=2*range/readTime;
    double readAccelTime=0;//readVelocity/pgSGUI->vsConv(pgSGUI->max_acc);
    double readAccelDis=0;//pgSGUI->vsConv(pgSGUI->max_acc)/2*readAccelTime*readAccelTime;
    double Offset=range+readAccelDis;
    double movTime=0;//2*sqrt(2*Offset/pgSGUI->vsConv(pgSGUI->max_acc));

    int dir=flipDir?(-1):1;
    PVTScan->add(movTime, 0,0, 0,0, 0,0, -dir*Offset,0);
    PVTScan->add(readAccelTime,  0,0, 0,0, 0,0 ,dir*readAccelDis,dir*readVelocity);
    PVTScan->addAction(XPS::iuScopeLED,false);
    PVTScan->add(readTime, 0,0, 0,0, 0,0, 2*dir*range,dir*readVelocity);
    PVTScan->add(readAccelTime,  0,0, 0,0, 0,0, dir*readAccelDis,0);
    PVTScan->add(movTime, 0,0, 0,0, 0,0, -dir*Offset,0);
    PVTScan->addAction(XPS::iuScopeLED,true);

    exec_dat ret;
    ret=go.pXPS->verifyPVTobj(PVTScan);
    if(ret.retval!=0) {
        std::string text=util::toString("Error: Verify PVTScan failed, retval was",ret.retstr,"\n");
        QMessageBox::critical(this, "Error", QString::fromStdString(text));
        return;
    }

    std::lock_guard<std::mutex>lock(MLP._lock_meas);
    ctrlRedLaser(true);
    FQ* framequeueDisp=go.pGCAM->iuScope->FQsPCcam.getNewFQ();

    while(!go.pXPS->isQueueEmpty()) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);  //wait till all motion is done, else motion and camera will be way out of sync
    go.pXPS->execPVTobj(PVTScan, &PVTret);
    if(waitTillLEDIsOff(framequeueDisp)){
        framequeueDisp->setUserFps(0,1);
        ctrlRedLaser(false);
        go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeueDisp);
        while(!PVTret.check_if_done()) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        std::string text=util::toString("pgBeamAnalysis::getCalibWritingBeamRange failed.\n");
        QMessageBox::critical(this, "Error", QString::fromStdString(text));
        std::cerr<<text;
        return;
    }
    framequeueDisp->setUserFps(9999,frames);
    while(!PVTret.check_if_done() || framequeueDisp->getFullNumber()<frames) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
    framequeueDisp->setUserFps(0,frames);
    while(framequeueDisp->getFullNumber()>frames) framequeueDisp->freeUserMat(); //get rid of exra frames
    ctrlRedLaser(false);

    //if(method_selector->index==0){        //only simple method is used here

    double focus=0;// TODO fix,removed focus: pgMGUI->FZdifference;

    cv::Mat dataX(1,frames,CV_32F, cv::Scalar{0});
    cv::Mat dataY(1,frames,CV_32F, cv::Scalar{0});
    cv::Mat dataR(1,frames,CV_32F, cv::Scalar{0});

    for(int N=0; N<frames; N++){
        cv::Mat src;
        cv::threshold(*framequeueDisp->getUserMat(), src, selThresh->val, 255, cv::THRESH_TOZERO);   //we first set all that are below threshold to 0
        cv::Moments m=cv::moments(src,true);
        //if(!saveNextFocus.empty()) imwrite(util::toString(saveNextFocus,"/", 2-numSave,"-",N,".png"), *framequeueDisp->getUserMat(),{cv::IMWRITE_PNG_COMPRESSION,9});
        framequeueDisp->freeUserMat();
        double x,y,r;
        dataX.at<float>(N)=m.m10/m.m00-src.cols/2;
        dataY.at<float>(N)=m.m01/m.m00-src.rows/2;
        dataR.at<float>(N)=m.m00;
        //dataR.at<float>(N)=m.m11;
    }

    cv::GaussianBlur(dataR,dataR,{0,0},4);             //blur it to get a smooth gaussian peak

    if(!saveNextFocus.empty()){
        std::ofstream wfile(util::toString(saveNextFocus,"/getCalibWritingBeamRange-", 2-numSave,".dat"));
        for(int N=0; N!=frames; N++) wfile<<focus+dir*(N-(frames-1)/2.)*2*range/frames<<" "<<dataR.at<float>(N)<<" "<<dataX.at<float>(N)<<" "<<dataY.at<float>(N)<<"\n";
        wfile.close();
        numSave--;
        if(numSave<=0) saveNextFocus.clear();
    }

    float max{0};       //find max
    int loc{0};
    for(int i=0; i!=frames; i++)
            if(dataR.at<float>(i)>max){
                max=dataR.at<float>(i);
                loc=i;
            }
    if(rMinLoc!=nullptr) *rMinLoc=focus+dir*(loc-(frames-1)/2.)*2*range/frames;
    if(xMin!=nullptr) *xMin=dataX.at<float>(loc);
    if(yMin!=nullptr) *yMin=dataY.at<float>(loc);

// // This commented block contains the gaussian fit method, which doesnt help here as the +-1 pixel error in the method above is equal or smaller to the error introduced by being unable to time the camera trigger with motion start anyway
//    //get the two maxima
//    int maxCoord[2]{0,frames-1};
//    for(int i=1; i<frames-1; i++)
//        if( dataR.at<float>(i-1) < dataR.at<float>(i) && dataR.at<float>(i+1) <= dataR.at<float>(i) ){
//            if(maxCoord[0]==0){
//                maxCoord[0]=i;
//            }else{
//                maxCoord[1]=i;
//                break;
//            }
//        }
//    //std::cerr<<"maxima were at "<<focus+(maxCoord[0]-(frames-1)/2)*range/frames<<" "<<focus+(maxCoord[1]-(frames-1)/2)*range/frames<<"\n";
//    cv::Mat peakData;
//    dataR.colRange(maxCoord[0],maxCoord[1]+1).copyTo(peakData);
// //    cv::multiply(peakData,-1,peakData);
//    cv::normalize(peakData,peakData,0,1,cv::NORM_MINMAX);

//    std::vector<std::pair<input_vector, double>> data;      //we fit gaussian
//    for(int i=0;i!=peakData.cols;i++)
//        data.push_back(std::make_pair(input_vector{(double)i},peakData.at<float>(i)));
//    parameter_vector res{peakData.cols/2.,1,peakData.cols/4.,0};
//    dlib::solve_least_squares_lm(dlib::objective_delta_stop_strategy(1e-7,100), gaussResidual, derivative(gaussResidual), data, res);
//    //std::cerr<<"Fitted gaussian center was at "<<focus+(res(0)+maxCoord[0]-(frames-1)/2)*range/frames<<"\n";       //sligthly more precise than the (commented) method above
//    *rMinLoc=focus+(res(0)+maxCoord[0]-(frames-1)/2)*range/frames;

    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeueDisp);
    go.pXPS->destroyPVTobj(PVTScan);
}

//bool pgBeamAnalysis::correctWritingBeamFocus(bool reCenter){
//    double r{9999},x,y;
//    std::chrono::time_point<std::chrono::system_clock> A=std::chrono::system_clock::now();
//    getCalibWritingBeamRange(&r, &x, &y, wideFrames->val, wideRange->val);
//    std::cerr<<"it. 1: Red Beam Focus difference is "<<r<<"\n";
//    if(r!=9999) pgMGUI->moveZF(r);
//    else return true;
//    getCalibWritingBeamRange(&r, &x, &y, accuFrames->val, accuRange->val);                    //changing the direction does not affect the result much
//    std::cerr<<"it. 2: Red Beam Focus difference is "<<r<<"\n";
//    if(r==9999) return true;
//    double ro=r, xo=x, yo=y;
//    if(doExtraFocusMesDifDir->val){
//        getCalibWritingBeamRange(&ro, &xo, &yo, accuFrames->val, accuRange->val,true);
//        std::cerr<<"it. 3: Red Beam Focus difference is "<<ro<<"\n";
//        if(ro==9999) return true;
//    }

//    if(r!=9999){
//        pgMGUI->moveZF((r+ro)/2-extraFocusOffset->val);
//        if(reCenter){
//            double ol[2]{_writeBeamCenterOfsX,_writeBeamCenterOfsY};
//            _writeBeamCenterOfsX=(x+xo)/2+extraOffsX->val*1000/pgMGUI->getNmPPx();
//            _writeBeamCenterOfsY=(y+yo)/2+extraOffsY->val*1000/pgMGUI->getNmPPx();
//            pgMGUI->move((_writeBeamCenterOfsX-ol[0])*pgMGUI->getNmPPx()/1000000,(_writeBeamCenterOfsY-ol[1])*pgMGUI->getNmPPx()/1000000,0,0);
//        }
//    }
//    else return true;
//    std::cerr<<"Calibration Beam Focus: "<<(r+ro)/2<<" mm\nX offset: "<<(x+xo)/2*pgMGUI->getNmPPx()/1000<<" um\nY offset: "<<(y+yo)/2*pgMGUI->getNmPPx()/1000<<" um\n";
//    std::cerr<<"New Focus (Corrected for Ref/Wr laser offset) "<<(r+ro)/2-extraFocusOffset->val<<" mm\n";
//    std::cerr<<"New X offset (Corrected for Ref/Wr laser offset) "<<_writeBeamCenterOfsX*pgMGUI->getNmPPx()/1000<<" um\n";
//    std::cerr<<"New Y offset (Corrected for Ref/Wr laser offset) "<<_writeBeamCenterOfsY*pgMGUI->getNmPPx()/1000<<" um\n";

//    std::chrono::time_point<std::chrono::system_clock> B=std::chrono::system_clock::now();
//    std::cerr<<"getWritingBeamCenter took "<<std::chrono::duration_cast<std::chrono::milliseconds>(B - A).count()<<" milliseconds\n";
//    return false;
//}
