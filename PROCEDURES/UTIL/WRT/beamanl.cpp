#include "beamanl.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgBeamAnalysis::pgBeamAnalysis(pgMoveGUI* pgMGUI): pgMGUI(pgMGUI){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    btnReset=new QPushButton("Reset Center");
    connect(btnReset, SIGNAL(released()), this, SLOT(onBtnReset()));
    selMaxRoundnessDev=new val_selector(0.1,"pgBeamAnalysis_selMaxRoundnessDev", "Maximum Roundness Deviation:",0.001,1,3);
    selMaxRoundnessDev->setToolTip("Will exclude all Elipses that are too elliptical: where one arm is this factor larger than the other.");
    selCannyThreshL=new val_selector( 50,"pgBeamAnalysis_selCannyThreshL", "Canny Lower Threshold:",0,255,0);
    selCannyThreshU=new val_selector(100,"pgBeamAnalysis_selCannyThreshU", "Canny Upper Threshold:",0,255,0);
    selMinPixNum=new val_selector(10,"pgBeamAnalysis_selMinPixNum", "Min Ellipse Pixel Number:",1,10000,0);
    selMinPixNum->setToolTip("Only Connected Pixel Groups (after Canny) With This Many Pixels or More Will be Considered for Ellipse Fitting.");
    btnSaveNextDebug=new QPushButton("Save Next Debug images");
    btnSaveNextDebug->setToolTip("Select Folder for Debug. Images of Every Step of the Process Will be Saved For the Next 'Get Writing Beam Center'.");
    connect(btnSaveNextDebug, SIGNAL(released()), this, SLOT(onBtnSaveNextDebug()));
    extraOffsX=new val_selector( 0,"pgBeamAnalysis_extraOffsX", "X Extra Offset:",-100,100,2);
    extraOffsY=new val_selector( 0,"pgBeamAnalysis_extraOffsY", "Y Extra Offset:",-100,100,2);
    extraOffsX->setToolTip("In case the two beams do not overlap perfectly, this can be used to correct this. Redo beam centering after changing this value.");
    extraOffsY->setToolTip(extraOffsX->toolTip());
    slayout->addWidget(new twid(btnReset));
    slayout->addWidget(selCannyThreshL);
    slayout->addWidget(selCannyThreshU);
    slayout->addWidget(selMinPixNum);
    slayout->addWidget(selMaxRoundnessDev);
    slayout->addWidget(new twid(btnSaveNextDebug));
    slayout->addWidget(extraOffsX);
    slayout->addWidget(extraOffsY);

    gui_activation=new QWidget;
    alayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);
    btnGetCenter=new QPushButton("Get Writing Beam Center");
    connect(btnGetCenter, SIGNAL(released()), this, SLOT(getWritingBeamCenter()));
    alayout->addWidget(new twid(btnGetCenter));
}
void pgBeamAnalysis::onBtnSaveNextDebug(){
    saveNext=QFileDialog::getExistingDirectory(this, "Select Folder for Debug. Images of Every Step of the Process Will be Saved For the Next 'Get Writing Beam Center'.").toStdString();
}
void pgBeamAnalysis::onBtnReset(){
    pgMGUI->move(-_writeBeamCenterOfsX*pgMGUI->getNmPPx()/1000000,-_writeBeamCenterOfsY*pgMGUI->getNmPPx()/1000000,0,0);
    _writeBeamCenterOfsX=0;
    _writeBeamCenterOfsY=0;
}

bool pgBeamAnalysis::sortSpot(spot i,spot j) {return (i.dd<j.dd);}
void pgBeamAnalysis::solveEllips(cv::Mat& src, int i,std::vector<spot>& spots,int& jobsdone){
    cv::Mat cmpMat;
    cv::compare(src, i, cmpMat, cv::CMP_EQ);
    std::vector<cv::Point> locations;   // output, locations of non-zero pixels
    cv::findNonZero(cmpMat, locations);
    if(locations.size()<selMinPixNum->val) {std::lock_guard<std::mutex>lock(spotLock);jobsdone++;return;}
    cv::RotatedRect tmp=fitEllipse(locations);
    std::lock_guard<std::mutex>lock(spotLock);
    if(std::abs(tmp.size.width-tmp.size.height)<=std::min(tmp.size.width,tmp.size.height)*selMaxRoundnessDev->val && std::min(tmp.size.width,tmp.size.height)>=1 && std::max(tmp.size.width,tmp.size.height)<=std::min(cmpMat.cols,cmpMat.rows))
        spots.push_back({tmp.center.x,tmp.center.y,(tmp.size.width+tmp.size.height)/4,0,0});
    jobsdone++;
}
void pgBeamAnalysis::getWritingBeamCenter(){
    float x,y,r,dx,dy;
    std::chrono::time_point<std::chrono::system_clock> A=std::chrono::system_clock::now();
    getCalibWritingBeam(&r, "", &dx, &dy);
    std::cerr<<"X mean: "<<writeBeamCenterOfsX<<", stdDev: "<< dx<<"\n";
    std::cerr<<"Y mean: "<<writeBeamCenterOfsY<<", stdDev: "<< dy<<"\n";
    std::cerr<<"Radius: "<<r<<"\n";
    std::chrono::time_point<std::chrono::system_clock> B=std::chrono::system_clock::now();
    std::cerr<<"getWritingBeamCenter took "<<std::chrono::duration_cast<std::chrono::microseconds>(B - A).count()<<" microseconds\n";
}
void pgBeamAnalysis::getCalibWritingBeam(float* r, std::string radHistSaveFName, float* dx, float* dy){
    go.pXPS->setGPIO(XPS::iuScopeLED, false);
    std::vector<uint32_t> commands;
    commands.push_back(CQF::GPIO_MASK(0x80,0,0x00));
    commands.push_back(CQF::GPIO_DIR (0x00,0,0x00));
    commands.push_back(CQF::GPIO_VAL (0x80,0,0x00));
    go.pRPTY->A2F_write(1,commands.data(),commands.size());
    commands.clear();


    // first we take frames until we hit the first one that has the LED turned off and the laser turned on. This is recognized using criteria below:
    const int borderWidth=10;
    const uchar lIntThresh=25;      // 1/10 of max int
    const float lOuterRatio=0.75;   // at least lOuterRatio of the pixels within the borderWidth pixel width border must be smaller than lIntThresh
    const uchar uIntTHresh=128;     // 1/2 of max int
    const float uInnerRatio=0.01;   // at least uInnerRatio of all the other pixels must be larger than uIntTHresh
    const int maxTriesFrames=100;   // give up after checking this many frames

    FQ* framequeueDisp=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
    framequeueDisp->setUserFps(9999,maxTriesFrames);
    for(int i=0;i!=maxTriesFrames+1;i++){
        if(i==maxTriesFrames){
            std::string text=util::toString("pgBeamAnalysis::getCalibWritingBeam failed after ",maxTriesFrames,"tries.");
            QMessageBox::critical(this, "Error", QString::fromStdString(text));
            std::cerr<<text;
            return;
        }

        while(framequeueDisp->getUserMat()==nullptr) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        cv::Mat mask(framequeueDisp->getUserMat()->rows,framequeueDisp->getUserMat()->cols,CV_8U,cv::Scalar(0));
        cv::rectangle(mask, cv::Point(borderWidth,borderWidth), cv::Point(mask.cols-borderWidth,mask.rows-borderWidth), cv::Scalar(255), cv::FILLED);
        cv::Mat temp;
        cv::compare(*framequeueDisp->getUserMat(),lIntThresh,temp,cv::CMP_LT);
        temp.setTo(0,mask);
        int lNum=cv::countNonZero(temp);
        //std::cerr<<"i= "<<i<<", lNum= "<<lNum<<", tot= "<<(mask.cols*mask.rows-(mask.cols-2*borderWidth)*(mask.rows-2*borderWidth))<<", ratio: "<<(float)lNum/(mask.cols*mask.rows-(mask.cols-2*borderWidth)*(mask.rows-2*borderWidth))<<"\n";
        if((float)lNum/(mask.cols*mask.rows-(mask.cols-2*borderWidth)*(mask.rows-2*borderWidth))<lOuterRatio) {framequeueDisp->freeUserMat(); continue;}
        cv::compare(*framequeueDisp->getUserMat(),uIntTHresh,temp,cv::CMP_GT);
        cv::bitwise_not(mask,mask);
        temp.setTo(0,mask);
        int uNum=cv::countNonZero(temp);
        //std::cerr<<"i= "<<i<<", uNum= "<<uNum<<", tot= "<<((mask.cols-2*borderWidth)*(mask.rows-2*borderWidth))<<", ratio: "<<(float)uNum/((mask.cols-2*borderWidth)*(mask.rows-2*borderWidth))<<"\n";
        if((float)uNum/((mask.cols-2*borderWidth)*(mask.rows-2*borderWidth))<uInnerRatio) {framequeueDisp->freeUserMat(); continue;}
        break;
    }

    framequeueDisp->setUserFps(0,maxTriesFrames);
    commands.push_back(CQF::GPIO_VAL (0x00,0,0x00));
    go.pRPTY->A2F_write(1,commands.data(),commands.size());
    go.pXPS->setGPIO(XPS::iuScopeLED, true);

//    std::chrono::time_point<std::chrono::system_clock> A=std::chrono::system_clock::now();
    //do proc
    std::vector<cv::Vec3f> circles;
    cv::Mat src;
    if(!saveNext.empty()) cv::imwrite(util::toString(saveNext,"/","0_src.png"), *framequeueDisp->getUserMat());
    cv::Canny(*framequeueDisp->getUserMat(),src, selCannyThreshL->val, selCannyThreshU->val);
    if(!saveNext.empty()) cv::imwrite(util::toString(saveNext,"/","1_canny.png"), src);
    int N=cv::connectedComponents(src,src,8,CV_16U);

    std::vector<spot> spots;
    int jobsdone=0;
    for(int i=1;i!=N;i++){  //we skip background 0
        go.OCL_threadpool.doJob(std::bind(&pgBeamAnalysis::solveEllips,this,src,i,std::ref(spots),std::ref(jobsdone)));
    }
    for(;;){
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        std::lock_guard<std::mutex>lock(spotLock);
        if(jobsdone==N-1) break;
    }

    bool once{false};
start: once=!once;
    float mean[3]{0,0,0};
    float stdDev[3]{0,0,0};
    for(int i=0;i!=spots.size();i++){
        mean[0]+=spots[i].x;
        mean[1]+=spots[i].y;
        mean[2]+=spots[i].r;
    }
    mean[0]/=spots.size();
    mean[1]/=spots.size();
    mean[2]/=spots.size();
    for(int i=0;i!=spots.size();i++){
        spots[i].dx=abs(spots[i].x-mean[0]);
        spots[i].dy=abs(spots[i].y-mean[1]);
        spots[i].dd=sqrtf(powf(spots[i].dx,2)+powf(spots[i].dy,2));
        stdDev[0]+=powf(spots[i].dx,2);
        stdDev[1]+=powf(spots[i].dy,2);
    }
    stdDev[0]=sqrt(stdDev[0]/(spots.size()-1));
    stdDev[1]=sqrt(stdDev[1]/(spots.size()-1));
    stdDev[2]=sqrtf(powf(stdDev[0],2)+powf(stdDev[1],2));
    std::sort(spots.begin(), spots.end(), &pgBeamAnalysis::sortSpot);

    const float SDR=1;    //within 2SD is fine
    while(!spots.empty()) if(spots.back().dd>SDR*stdDev[2]) spots.pop_back(); else break;
    if(!spots.empty() && once) goto start;

//    std::chrono::time_point<std::chrono::system_clock> B=std::chrono::system_clock::now();

    cv::Mat pol, pol1D;
    cv::linearPolar(*framequeueDisp->getUserMat(), pol, cv::Point(mean[0],mean[1]), framequeueDisp->getUserMat()->rows, cv::INTER_AREA + cv::WARP_FILL_OUTLIERS);
    cv::reduce(pol, pol1D, 0, cv::REDUCE_AVG, CV_32F);
    cv::Point maxp; double ignore; cv::Point ignore2;
    int ofs=((int)mean[2]<pol1D.cols-1)?((int)mean[2]):(pol1D.cols-1);
    cv::minMaxLoc(pol1D.colRange(ofs,pol1D.cols-1),&ignore,&ignore,&ignore2,&maxp);     //we ignore the peaks near the center, we want the outer bright ring as the reference

//    std::chrono::time_point<std::chrono::system_clock> C=std::chrono::system_clock::now();

    if(!saveNext.empty()){
        cv::Mat img;
        cv::cvtColor(*framequeueDisp->getUserMat(), img, cv::COLOR_GRAY2BGR);
        for(int i=0;i!=spots.size();i++){
            cv::circle( img, cv::Point(spots[i].x,spots[i].y), 3, cv::Scalar(0,255,0), -1, 8, 0 );
            cv::circle( img, cv::Point(spots[i].x,spots[i].y), spots[i].r, cv::Scalar(0,0,255), 1, 8, 0 );
        }
        cv::imwrite(util::toString(saveNext,"/","2_final.png"), img);
        saveNext.clear();
    }

    mean[0]-=src.cols/2;
    mean[1]-=src.rows/2;
    *r=ofs+maxp.x;
    if(dx!=nullptr) *dx=stdDev[0];
    if(dy!=nullptr) *dy=stdDev[1];
    if(!radHistSaveFName.empty()){
        std::cerr<<"Saving writing laser calibration radial histogram to "<<radHistSaveFName<<"\n";
        std::ofstream wfile(radHistSaveFName);
        for(int i=0;i!=pol1D.cols;i++)
            wfile.write(reinterpret_cast<char*>(&(pol1D.at<float>(i))),sizeof(float));
        wfile.close();
    }

//    std::chrono::time_point<std::chrono::system_clock> D=std::chrono::system_clock::now();
//    std::cerr<<"Total operation took "<<std::chrono::duration_cast<std::chrono::microseconds>(D - A).count()<<" microseconds\n";
//    std::cerr<<"First operation took "<<std::chrono::duration_cast<std::chrono::microseconds>(B - A).count()<<" microseconds\n";
//    std::cerr<<"Second operation took "<<std::chrono::duration_cast<std::chrono::microseconds>(C - B).count()<<" microseconds\n";
//    std::cerr<<"Third operation took "<<std::chrono::duration_cast<std::chrono::microseconds>(D - C).count()<<" microseconds\n";

    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeueDisp);

    pgMGUI->move((mean[0]+extraOffsX->val-_writeBeamCenterOfsX)*pgMGUI->getNmPPx()/1000000,(mean[1]+extraOffsY->val-_writeBeamCenterOfsY)*pgMGUI->getNmPPx()/1000000,0,0);        //correct position

    _writeBeamCenterOfsX=mean[0]+extraOffsX->val;
    _writeBeamCenterOfsY=mean[1]+extraOffsY->val;
}
