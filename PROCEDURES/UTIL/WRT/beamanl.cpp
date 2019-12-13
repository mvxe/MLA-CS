#include "beamanl.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgBeamAnalysis::pgBeamAnalysis(){
    gui_activation=new QWidget;
    alayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);
    btnGetCenter=new QPushButton("Get Writing Beam Center");
    connect(btnGetCenter, SIGNAL(released()), this, SLOT(getWritingBeamCenter()));
    alayout->addWidget(new twid(btnGetCenter));
}



bool pgBeamAnalysis::sortSpot(spot i,spot j) {return (i.dd<j.dd);}
void pgBeamAnalysis::solveEllips(cv::Mat& src, int i,std::vector<spot>& spots,int& jobsdone){
    cv::Mat cmpMat;
    cv::compare(src, i, cmpMat, cv::CMP_EQ);
    std::vector<cv::Point> locations;   // output, locations of non-zero pixels
    cv::findNonZero(cmpMat, locations);
    if(locations.size()<10) {std::lock_guard<std::mutex>lock(spotLock);jobsdone++;return;}
    cv::RotatedRect tmp=fitEllipse(locations);
    std::lock_guard<std::mutex>lock(spotLock);
    if(std::abs(tmp.size.width-tmp.size.height)<=std::min(tmp.size.width,tmp.size.height)*maxRoundnessDev && std::min(tmp.size.width,tmp.size.height)>=1 && std::max(tmp.size.width,tmp.size.height)<=std::min(cmpMat.cols,cmpMat.rows))
        spots.push_back({tmp.center.x,tmp.center.y,(tmp.size.width+tmp.size.height)/4,0,0});
    jobsdone++;
}
void pgBeamAnalysis::getWritingBeamCenter(){
    float x,y,r,dx,dy;
    std::chrono::time_point<std::chrono::system_clock> A=std::chrono::system_clock::now();
    getCalibWritingBeam(&x, &y, &r, "", &dx, &dy);
    std::cerr<<"X mean: "<<x<<", stdDev: "<< dx<<"\n";
    std::cerr<<"Y mean: "<<y<<", stdDev: "<< dy<<"\n";
    std::cerr<<"Radius: "<<r<<"\n";
    std::chrono::time_point<std::chrono::system_clock> B=std::chrono::system_clock::now();
    std::cerr<<"getWritingBeamCenter took "<<std::chrono::duration_cast<std::chrono::microseconds>(B - A).count()<<" microseconds\n";
}
void pgBeamAnalysis::getCalibWritingBeam(float* x, float* y, float* r, std::string radHistSaveFName, float* dx, float* dy){
    go.pXPS->setGPIO(XPS::iuScopeLED, false);
    std::vector<uint32_t> commands;
    commands.push_back(CQF::GPIO_MASK(0x80,0,0x00));
    commands.push_back(CQF::GPIO_DIR (0x00,0,0x00));
    commands.push_back(CQF::GPIO_VAL (0x80,0,0x00));
    go.pRPTY->A2F_write(1,commands.data(),commands.size());
    commands.clear();

    FQ* framequeueDisp=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
    framequeueDisp->setUserFps(9999,10);
    while(framequeueDisp->getFullNumber()<10) QCoreApplication::processEvents(QEventLoop::AllEvents, 10);  //we skip the first 9 images
    for(int i=0;i!=9;i++) framequeueDisp->freeUserMat();

    commands.push_back(CQF::GPIO_VAL (0x00,0,0x00));
    go.pRPTY->A2F_write(1,commands.data(),commands.size());
    go.pXPS->setGPIO(XPS::iuScopeLED, true);

//    std::chrono::time_point<std::chrono::system_clock> A=std::chrono::system_clock::now();
    //do proc
    cv::Mat img;
    std::vector<cv::Vec3f> circles;
    cv::Mat src;
//        cv::imwrite("64755464beampic0.png", *framequeueDisp->getUserMat());
    cv::Canny(*framequeueDisp->getUserMat(),src, 100/2, 100);
//        cv::imwrite("64755464beampic2.png", src);
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

    cv::cvtColor(*framequeueDisp->getUserMat(), img, cv::COLOR_GRAY2BGR);
    for(int i=0;i!=spots.size();i++){
        cv::circle( img, cv::Point(spots[i].x,spots[i].y), 3, cv::Scalar(0,255,0), -1, 8, 0 );
        cv::circle( img, cv::Point(spots[i].x,spots[i].y), spots[i].r, cv::Scalar(0,0,255), 1, 8, 0 );
    }

//    cv::imwrite("64755464beampic3.png", img);
    *x=mean[0];
    *y=mean[1];
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
}
