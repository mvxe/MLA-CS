#include "calib.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include "PROCEDURES/UTIL/USC/focus.h"
#include "PROCEDURES/UTIL/USC/move.h"
#include "PROCEDURES/UTIL/WRT/bounds.h"
#include "opencv2/core/utils/filesystem.hpp"
#include "GUI/tab_monitor.h"    //for debug purposes
#include <dirent.h>
#include <sys/stat.h>
#include <dlib/optimization.h>
#include <algorithm>
#include <random>

pgCalib::pgCalib(pgScanGUI* pgSGUI, pgBoundsGUI* pgBGUI, pgFocusGUI* pgFGUI, pgMoveGUI* pgMGUI, pgDepthEval* pgDpEv, pgBeamAnalysis* pgBeAn): pgSGUI(pgSGUI), pgBGUI(pgBGUI), pgFGUI(pgFGUI), pgMGUI(pgMGUI), pgDpEv(pgDpEv), pgBeAn(pgBeAn){
    gui_activation=new QWidget;
    alayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);
//    btnGoToNearestFree=new QPushButton("Go to nearest free");
//    btnGoToNearestFree->setToolTip("This adheres to write boundaries!");
//    connect(btnGoToNearestFree, SIGNAL(released()), this, SLOT(onGoToNearestFree()));
//    hcGoToNearestFree=new hidCon(btnGoToNearestFree);
//    alayout->addWidget(hcGoToNearestFree);
//    selRadDilGoToNearestFree=new val_selector(1, "pgCalib_selRadDilGoToNearestFree", "Exclusion Dilation Radius: ", 0, 100, 2, 0, {"um"});
//    selRadSprGoToNearestFree=new val_selector(1, "pgCalib_selRadSprGoToNearestFree", "Random Selection Radius: ", 0, 100, 2, 0, {"um"});
//    hcGoToNearestFree->addWidget(selRadDilGoToNearestFree);
//    hcGoToNearestFree->addWidget(selRadSprGoToNearestFree);

    btnWriteCalib=new HQPushButton("Calibrate Write Focus");
    connect(btnWriteCalib, SIGNAL(released()), this, SLOT(onWCF()));
    connect(btnWriteCalib, SIGNAL(changed(bool)), this, SLOT(onChangeDrawWriteAreaOn(bool)));
    btnWriteCalib->setCheckable(true);
    alayout->addWidget(new twid(btnWriteCalib));

    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);

    calibMethodFindNearest=new vtwid;
        calibMethodFindNearest->addWidget(new QLabel("Selection procedure: Blur+Laplacian+Thresh+Dilate+Exclusion+Border"));
        selWriteCalibFocusDoNMeas=new val_selector(100, "Do this many measurements: ", 1, 100000, 0);
        conf["selWriteCalibFocusDoNMeas"]=selWriteCalibFocusDoNMeas;
        calibMethodFindNearest->addWidget(selWriteCalibFocusDoNMeas);
        selWriteCalibFocusReFocusNth=new val_selector(5, "Refocus every this many measurements: ", 0, 1000, 0);
        conf["selWriteCalibFocusReFocusNth"]=selWriteCalibFocusReFocusNth;
        selWriteCalibFocusReFocusNth->setToolTip("Set 0 to disable refocusing.");
        calibMethodFindNearest->addWidget(selWriteCalibFocusReFocusNth);
        selWriteCalibFocusRadDil=new val_selector(1, "Exclusion Dilation / save ROI Radius: ", 0, 100, 2, 0, {"um"});
        conf["selWriteCalibFocusRadDil"]=selWriteCalibFocusRadDil;
        selWriteCalibFocusRadDil->setToolTip("This should be >3x your beam spot size...");
        calibMethodFindNearest->addWidget(selWriteCalibFocusRadDil);
        selWriteCalibFocusRadSpr=new val_selector(1, "Random Selection Radius: ", 0, 100, 2, 0, {"um"});
        conf["selWriteCalibFocusRadSpr"]=selWriteCalibFocusRadSpr;
        calibMethodFindNearest->addWidget(selWriteCalibFocusRadSpr);
        selWriteCalibFocusBlur=new val_selector(2, "Gaussian Blur Sigma: ", 0, 100, 1, 0, {"px"});
        conf["selWriteCalibFocusBlur"]=selWriteCalibFocusBlur;
        calibMethodFindNearest->addWidget(selWriteCalibFocusBlur);
        selWriteCalibFocusThresh=new val_selector(0.2, "2nd Derivative Exclusion Threshold: ", 0, 1, 4);
        conf["selWriteCalibFocusThresh"]=selWriteCalibFocusThresh;
        selWriteCalibFocusThresh->setToolTip("Try out values in Depth Eval.");
        calibMethodFindNearest->addWidget(selWriteCalibFocusThresh);
        selWriteCalibFocusRange=new val_selector(0, "Measurement range around focus: ", 0, 1000, 3, 0, {"um"});
        conf["selWriteCalibFocusRange"]=selWriteCalibFocusRange;
        selWriteCalibFocusRange->setToolTip("Each measurement will be done at a random write beam focus around the starting focus\u00B1 this parameter.");
        calibMethodFindNearest->addWidget(selWriteCalibFocusRange);
        selWriteCalibFocusPulseIntensity=new val_selector(1000, "Pulse Intensity: ", 0, 8192, 0);
        conf["selWriteCalibFocusPulseIntensity"]=selWriteCalibFocusPulseIntensity;
        calibMethodFindNearest->addWidget(selWriteCalibFocusPulseIntensity);
        selWriteCalibFocusPulseDuration=new val_selector(10, "Pulse Duration: ", 0.008, 1000000, 3, 0, {"us"});
        conf["selWriteCalibFocusPulseDuration"]=selWriteCalibFocusPulseDuration;
        calibMethodFindNearest->addWidget(selWriteCalibFocusPulseDuration);
    calibMethodArray=new vtwid;
        selArrayXsize=new val_selector(10, "Array Size X", 1, 1000, 0);
        conf["selArrayXsize"]=selArrayXsize;
        calibMethodArray->addWidget(selArrayXsize);
        selArrayYsize=new val_selector(10, "Array Size Y", 1, 1000, 0);
        conf["selArrayYsize"]=selArrayYsize;
        calibMethodArray->addWidget(selArrayYsize);
        selArraySpacing=new val_selector(5, "Array Spacing", 0.001, 100, 3, 0, {"um"});
        conf["selArraySpacing"]=selArraySpacing;
        calibMethodArray->addWidget(selArraySpacing);
        selArrayType=new smp_selector("Variable Parameters (X-Y): ", 0, {"Duration(X,Y), no repeat","Focus(X,Y), no repeat","Duration(X)-Focus(Y)", "Duration(X,Y), repeat","Focus(X,Y), repeat"});
        conf["selArrayType"]=selArrayType;
        calibMethodArray->addWidget(selArrayType);
        transposeMat=new checkbox_gs(false,"Transpose matrix.");
        conf["transposeMat"]=transposeMat;
        calibMethodArray->addWidget(transposeMat);
        calibMethodArray->addWidget(new QLabel("The Variable Parameters Will be Within the Specified Range.\nIf a Parameter is not Variable, it Will be Equal to Val A!"));
        selArrayDurA=new val_selector(1, "Duration Value A", 0.001, 1000, 3, 0, {"ms"});
        conf["selArrayDurA"]=selArrayDurA;
        calibMethodArray->addWidget(selArrayDurA);
        selArrayDurB=new val_selector(1, "Duration Value B", 0.001, 1000, 3, 0, {"ms"});
        conf["selArrayDurB"]=selArrayDurB;
        calibMethodArray->addWidget(selArrayDurB);
        selArrayFocA=new val_selector(-1, "Focus Value A", -1000, 1000, 3, 0, {"um"});
        conf["selArrayFocA"]=selArrayFocA;
        calibMethodArray->addWidget(selArrayFocA);
        selArrayFocB=new val_selector(1, "Focus Value B", -1000, 1000, 3, 0, {"um"});
        conf["selArrayFocB"]=selArrayFocB;
        calibMethodArray->addWidget(selArrayFocB);
        selArrayOneScanN=new val_selector(10, "Take and average This Many Scans: ", 1, 1000, 0);
        conf["selArrayOneScanN"]=selArrayOneScanN;
        calibMethodArray->addWidget(selArrayOneScanN);
        selArrayRandomize=new checkbox_gs(false,"Randomize Value Order");
        conf["selArrayRandomize"]=selArrayRandomize;
        calibMethodArray->addWidget(selArrayRandomize);
        saveMats=new checkbox_gs(true,"Extra Save Mats Containing D,F for Convenience.");
        conf["saveMats"]=saveMats;
        calibMethodArray->addWidget(saveMats);
        savePic=new checkbox_gs(true,"Also save direct pictures of measurements.");
        conf["savePic"]=savePic;
        calibMethodArray->addWidget(savePic);

    calibMethodAutoArray=new vtwid;
        calibMethodAutoArray->addWidget(new QLabel("Selection procedure: Exclusion+DilateSQ+Border"));
        selAArrayDoNMes=new val_selector(100, "In total do this many measurements (will decrement): ", 0, 100000000, 0);
        conf["selAArrayDoNMes"]=selAArrayDoNMes;
        calibMethodAutoArray->addWidget(selAArrayDoNMes);
        selAArrayXsize=new val_selector(10, "Array Size X", 1, 1000, 0);
        conf["selAArrayXsize"]=selAArrayXsize;
        calibMethodAutoArray->addWidget(selAArrayXsize);
        selAArrayYsize=new val_selector(10, "Array Size Y", 1, 1000, 0);
        conf["selAArrayYsize"]=selAArrayYsize;
        calibMethodAutoArray->addWidget(selAArrayYsize);
        selAArraySpacing=new val_selector(5, "Array Spacing", 0.001, 100, 3, 0, {"um"});
        conf["selAArraySpacing"]=selAArraySpacing;
        calibMethodAutoArray->addWidget(selAArraySpacing);
        selAArrayAvgN=new val_selector(10, "Average This Many Scans: ", 1, 1000, 0);
        conf["selAArrayAvgN"]=selAArrayAvgN;
        calibMethodAutoArray->addWidget(selAArrayAvgN);
        selAArrayIntA=new val_selector(1000, "Intensity Value A", 1, 8192, 0);
        conf["selAArrayIntA"]=selAArrayIntA;
        calibMethodAutoArray->addWidget(selAArrayIntA);
        selAArrayIntB=new val_selector(1000, "Intensity Value B", 1, 8192, 0);
        conf["selAArrayIntB"]=selAArrayIntB;
        calibMethodAutoArray->addWidget(selAArrayIntB);
        selAArrayDurA=new val_selector(1, "Duration Value A", 0.001, 1000, 3, 0, {"ms"});
        conf["selAArrayDurA"]=selAArrayDurA;
        calibMethodAutoArray->addWidget(selAArrayDurA);
        selAArrayDurB=new val_selector(1, "Duration Value B", 0.001, 1000, 3, 0, {"ms"});
        conf["selAArrayDurB"]=selAArrayDurB;
        calibMethodAutoArray->addWidget(selAArrayDurB);
        selAArrayNGenCand=new val_selector(100, "Consider this many random points as next location: ", 1, 10000, 0);
        conf["selAArrayNGenCand"]=selAArrayNGenCand;
        calibMethodAutoArray->addWidget(selAArrayNGenCand);
        selAArraySetMaskToThisHeight=new val_selector(200, "Bad pixel height (for planning next move): ", 0, 10000, 0, 0, {"nm"});
        conf["selAArraySetMaskToThisHeight"]=selAArraySetMaskToThisHeight;
        selAArraySetMaskToThisHeight->setToolTip("This should be a few times higher than the highest structure you expect to be written.\nIt is used only to plan the next move (the larger it is, the more likely the program will avoid areas with bad pixels).\nIn the end individual scans with"
                                                 " bad pixels are discarded anyway.");
        calibMethodAutoArray->addWidget(selAArraySetMaskToThisHeight);

    calibMethod=new twd_selector("Select method:", "Array", false, false, false);
    calibMethod->addWidget(calibMethodArray, "Array");
    calibMethod->addWidget(calibMethodAutoArray, "Auto Array");
    calibMethod->addWidget(calibMethodFindNearest,"Find Nearest");
    calibMethod->setIndex(0);
    conf["calibMethod"]=calibMethod;
    slayout->addWidget(calibMethod);
    slayout->addWidget(new hline);
    slayout->addWidget(new QLabel("PROCESSING (array method):"));
    slayout->addWidget(new QLabel("Crop individual points (for shifted beams)(setting not saved):"));
    cropTop =new QSpinBox;  cropTop->setRange(0,100);  cropTop->setPrefix("Top: ");  cropTop->setSuffix(" px");
    cropBttm=new QSpinBox; cropBttm->setRange(0,100); cropBttm->setPrefix("Bottom: "); cropBttm->setSuffix(" px");
    cropLeft=new QSpinBox; cropLeft->setRange(0,100); cropLeft->setPrefix("Left: "); cropLeft->setSuffix(" px");
    cropRght=new QSpinBox; cropRght->setRange(0,100); cropRght->setPrefix("Right: "); cropRght->setSuffix(" px");
    slayout->addWidget(new twid(cropTop,cropBttm));
    slayout->addWidget(new twid(cropLeft,cropRght));


    gui_processing=new QWidget;
    playout=new QVBoxLayout;
    gui_processing->setLayout(playout);
    btnProcessFocusMes=new QPushButton("Select Folders to Process Focus Measurements (Array method)");
    connect(btnProcessFocusMes, SIGNAL(released()), this, SLOT(onProcessFocusMes()));
    playout->addWidget(new twid(btnProcessFocusMes));
}

//void pgCalib::onGoToNearestFree(){
//    goToNearestFree(selRadDilGoToNearestFree->val, selRadSprGoToNearestFree->val);
//}
bool pgCalib::goToNearestFree(double radDilat, double radRandSpread){
    varShareClient<pgScanGUI::scanRes>* scanRes=pgSGUI->result.getClient();
    while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //if there is a measurement in progress, wait till its done
    XPS::raxis tmp=go.pXPS->getPos(XPS::mgroup_XYZF);       //first check if the current scan result is valid (done oncurrent position)
    bool redoScan=false;
    const pgScanGUI::scanRes* res=scanRes->get();
    if(res!=nullptr){
        for(int i=0;i!=3;i++) if(res->pos[i]!=tmp.pos[i])redoScan=true;
    }else redoScan=true;
    if(redoScan){
        pgSGUI->doOneRound();
        while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
        res=scanRes->get();
    }
    int dil=(radDilat*1000/res->XYnmppx-0.5); if(dil<0) dil=0;
    cv::Mat mask=pgDpEv->getMaskFlatness(res, dil, selWriteCalibFocusThresh->val, selWriteCalibFocusBlur->val);
    int ptX,ptY;
//TODO!        imgAux::getNearestFreePointToCenter(&mask, pgBeAn->writeBeamCenterOfsX, pgBeAn->writeBeamCenterOfsY, ptX, ptY, radRandSpread);
    if(ptX==-1){
        std::cerr<<"No free nearby!\n";
        delete scanRes;
        return true;
    }

    double dXumm, dYumm, dXmm, dYmm;
    dXumm=(ptX-res->depth.cols/2)*res->XYnmppx/1000000;
    dYumm=(ptY-res->depth.rows/2)*res->XYnmppx/1000000;
    dXmm=dXumm*cos(pgMGUI->getAngCamToXMot(0))+dYumm*sin(pgMGUI->getAngCamToXMot(0)+pgMGUI->getYMotToXMot());
    dYmm=dXumm*sin(pgMGUI->getAngCamToXMot(0))+dYumm*cos(pgMGUI->getAngCamToXMot(0)+pgMGUI->getYMotToXMot());

    pgMGUI->move(-dXmm,-dYmm,0);
    delete scanRes;
    return false;
}

void pgCalib::onWCF(){
    if(!btnWriteCalib->isChecked()) return;
    if(!go.pRPTY->connected) {QMessageBox::critical(this, "Error", "Error: Red Pitaya not Connected"); return;}
    saveFolderName=QFileDialog::getExistingDirectory(this, "Select Folder for Saving Calibration Data").toStdString();
    if(saveFolderName.empty()){
        btnWriteCalib->setChecked(false);
        return;
    }

    switch(calibMethod->index){
    case 0: WCFArray();
            break;
    case 1: WCFAArray();
            break;
    case 2: WCFFindNearest();
            break;
    }
}

std::string pgCalib::makeDateTimeFolder(const std::string folder){
    std::time_t time=std::time(nullptr); std::tm ltime=*std::localtime(&time);
    std::stringstream ssfolder; ssfolder<<folder;
    ssfolder<<std::put_time(&ltime, "/%Y-%m-%d/");
    cv::utils::fs::createDirectory(ssfolder.str());
    ssfolder<<std::put_time(&ltime, "%H/");
    cv::utils::fs::createDirectory(ssfolder.str());
    ssfolder<<std::put_time(&ltime, "%M-%S/");
    cv::utils::fs::createDirectory(ssfolder.str());
    return ssfolder.str();
}

void pgCalib::saveMainConf(std::string filename){
    std::ofstream setFile(filename);     //this file contains some settings:
    setFile <<"Objective_displacement_X(mm) Objective_displacement_Y(mm) Objective_displacement_Z(mm) MirauXYmmppx(mm/px)\n";
    setFile << std::setprecision(6);
    setFile <<pgMGUI->objectiveDisplacementX<<" "<<pgMGUI->objectiveDisplacementY<<" "<<pgMGUI->objectiveDisplacementZ<<" "<<pgMGUI->getNmPPx()/1000000<<"\n";
    setFile.close();
}
void pgCalib::saveConf(std::string filename, double duration, double focus){
    std::ofstream setFile(filename);            //this file contains some settings:
    setFile <<"Duration(ms) Focus(um)\n";
    setFile <<duration<<" "<<focus<<"\n";
    setFile.close();
}

void pgCalib::WCFFindNearest(){
//TODO!
    throw std::invalid_argument("gDT still needs to be transitioned to CTRL");
//TODO!

    if(goToNearestFree(selWriteCalibFocusRadDil->val,selWriteCalibFocusRadSpr->val)) {QMessageBox::critical(this, "Error", "No free nearby, stopping."); btnWriteCalib->setChecked(false); return;}
    varShareClient<pgScanGUI::scanRes>* scanRes=pgSGUI->result.getClient();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 500);    //some waiting time for the system to stabilize after a rapid move

    std::string folder=makeDateTimeFolder(saveFolderName);

    if((int)(selWriteCalibFocusReFocusNth->val)!=0)
    if(!(measCounter%((int)(selWriteCalibFocusReFocusNth->val)))){
//        pgFGUI->refocus();
    }

    //this seams to be useless
//    if(selWriteCalibFocusMoveOOTW->val){
//        pgMGUI->move( selWriteCalibFocusMoveOOTWDis->val,0,0,0);
//        QCoreApplication::processEvents(QEventLoop::AllEvents, 500);    //some waiting time for the system to stabilize after a rapid move
//        FQ* framequeueDisp=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
//        framequeueDisp->setUserFps(9999,1);
//        for(int i=0;i!=40;i++) while(framequeueDisp->getUserMat()==nullptr) std::this_thread::sleep_for(std::chrono::milliseconds(10));  //we the first few images
//        cv::imwrite(util::toString(folder.str(),"/beampic.png"), *framequeueDisp->getUserMat());
//        go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeueDisp);
//        pgMGUI->move(-selWriteCalibFocusMoveOOTWDis->val,0,0,0);
//        QCoreApplication::processEvents(QEventLoop::AllEvents, 500);    //some waiting time for the system to stabilize after a rapid move
//    }

    std::mt19937 rnd(std::random_device{}());
    std::uniform_real_distribution<>dist(-selWriteCalibFocusRange->val, selWriteCalibFocusRange->val);
    double wrFocusOffs=dist(rnd);
    wrFocusOffs=round(wrFocusOffs*1000)/1000;   //we round it up to 1 nm precision
    std::cerr<<"wrFocusOffs is: "<<wrFocusOffs<<" um\n";
    double focus=0;// TODO fix,removed focus: pgMGUI->FZdifference;
    std::cerr<<"Focus is: "<<focus<<" mm\n";
//TODO!    pgMGUI->moveZF(focus+wrFocusOffs/1000);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait a bit for movement to complete and stabilize
    std::cerr<<"new focus is: "<<focus+wrFocusOffs/1000<<" mm\n";

    redoA:  pgSGUI->doOneRound();
    while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
    const pgScanGUI::scanRes* res=scanRes->get();
    int roiD=2*selWriteCalibFocusRadDil->val*1000/res->XYnmppx;
//TODO!        if(cv::countNonZero(cv::Mat(res->mask, cv::Rect(res->depth.cols/2-roiD/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-roiD/2+pgBeAn->writeBeamCenterOfsY, roiD, roiD)))>roiD*roiD*(4-M_PI)/4){std::cerr<<"To much non zero mask in ROI; redoing measurement.\n";goto redoA;}      //this is (square-circle)/4
//TODO!        pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-roiD/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-roiD/2+pgBeAn->writeBeamCenterOfsY, roiD, roiD), util::toString(folder,"/before"), true, false);

    uchar selectedavg;
//TODO!    writePulse(selWriteCalibFocusPulseIntensity->val, selWriteCalibFocusPulseDuration->val, util::toString(folder,"/laser.dat"), &selectedavg);

//TODO!    pgMGUI->moveZF(focus);

//TODO!     saveConf(util::toString(folder,"/settings.txt"), focus+wrFocusOffs/1000, selWriteCalibFocusRadDil->val, selWriteCalibFocusPulseIntensity->val, selWriteCalibFocusPulseDuration->val, selectedavg);

    redoB:  pgSGUI->doOneRound();
    while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
    res=scanRes->get();
//TODO!        if(cv::countNonZero(cv::Mat(res->mask, cv::Rect(res->depth.cols/2-roiD/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-roiD/2+pgBeAn->writeBeamCenterOfsY, roiD, roiD)))>roiD*roiD*(4-M_PI)/4){std::cerr<<"To much non zero mask in ROI; redoing measurement.\n";goto redoB;}
//TODO!        pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-roiD/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-roiD/2+pgBeAn->writeBeamCenterOfsY, roiD, roiD), util::toString(folder,"/after"));

    if(btnWriteCalib->isChecked()){
        measCounter++;
        if(measCounter<(int)selWriteCalibFocusDoNMeas->val) {delete scanRes; return WCFFindNearest();}
        else {btnWriteCalib->setChecked(false); measCounter=0;}
    } else measCounter=0;
    delete scanRes;
}

void pgCalib::WCFArray(){
    varShareClient<pgScanGUI::scanRes>* scanRes=pgSGUI->result.getClient();
    cv::Mat WArray(selArrayYsize->val,selArrayXsize->val,CV_64FC2,cv::Scalar(0,0));         //Duration(ms), Focus(um)
    int arraySizeDur{1}, arraySizeFoc{1};
    int index=selArrayType->index;
    switch(index){
    case 0: arraySizeDur=selArrayYsize->val*selArrayXsize->val;     //Duration (XY), no repeat
            break;
    case 1: arraySizeFoc=selArrayYsize->val*selArrayXsize->val;     //Focus (XY), no repeat
            break;
    case 2: arraySizeDur=selArrayXsize->val;                        //Duration-Focus (X,Y)
            arraySizeFoc=selArrayYsize->val;
            break;
    case 3: arraySizeDur=selArrayXsize->val;                        //Duration (XY), repeat
            break;
    case 4: arraySizeFoc=selArrayXsize->val;                        //Focus (XY), repeat
            break;
    }
    std::vector<double> arrayDur; arrayDur.reserve(arraySizeDur);
    std::vector<double> arrayFoc; arrayFoc.reserve(arraySizeFoc);
    if(arraySizeDur==1) arrayDur.push_back(selArrayDurA->val);
    else for(int i=0;i!=arraySizeDur; i++) arrayDur.push_back(selArrayDurA->val*(1-(double)i/(arraySizeDur-1))+selArrayDurB->val*((double)i/(arraySizeDur-1)));
    if(arraySizeFoc==1) arrayFoc.push_back(selArrayFocA->val);
    else for(int i=0;i!=arraySizeFoc; i++) arrayFoc.push_back(selArrayFocA->val*(1-(double)i/(arraySizeFoc-1))+selArrayFocB->val*((double)i/(arraySizeFoc-1)));

    if(selArrayRandomize->val){                         //randomize if chosen
        std::mt19937 rnd(std::random_device{}());
        std::shuffle(arrayDur.begin(), arrayDur.end(), rnd);
        std::shuffle(arrayFoc.begin(), arrayFoc.end(), rnd);
    }
    for(int i=0;i!=WArray.cols; i++) for(int j=0;j!=WArray.rows; j++){              //populate 3D x2 array
        if(index==0) WArray.at<cv::Vec2d>(j,i)[0]=arrayDur[i+j*WArray.cols];
        else if(index==2 || index==3) WArray.at<cv::Vec2d>(j,i)[0]=arrayDur[i];
        else WArray.at<cv::Vec2d>(j,i)[0]=arrayDur[0];
        if(index==1) WArray.at<cv::Vec2d>(j,i)[1]=arrayFoc[i+j*WArray.cols];
        else if(index==2)  WArray.at<cv::Vec2d>(j,i)[1]=arrayFoc[j];
        else if(index==4)  WArray.at<cv::Vec2d>(j,i)[1]=arrayFoc[i];
        else WArray.at<cv::Vec2d>(j,i)[1]=arrayFoc[0];
    }
    if(transposeMat->val)
        cv::transpose(WArray,WArray);

    std::string folder=makeDateTimeFolder(saveFolderName);
    if(saveMats->val){      //export values as matrices, for convenience
        std::string names[2]={"Duration","Focus"};
        for(int k=0;k!=2;k++){
            std::ofstream wfile(util::toString(folder,"/", names[k],".txt"));
            for(int i=0;i!=WArray.cols; i++){
                for(int j=0;j!=WArray.rows; j++)
                    wfile<<WArray.at<cv::Vec2d>(j,i)[k]<<" ";
                wfile<<"\n";
            }
            wfile.close();
        }
    }

    double xOfs=((WArray.cols-1)*selArraySpacing->val)/2000;         //in mm
    double yOfs=((WArray.rows-1)*selArraySpacing->val)/2000;
    double xSize=WArray.cols*pgMGUI->mm2px(selArraySpacing->val/1000);
    double ySize=WArray.rows*pgMGUI->mm2px(selArraySpacing->val/1000);

    // TODO: if larger calibration matrices are needed expand this to support - chop up the calibration matrix into submatrices that fit ROI

    // set ROI
    bool err=false;
    cv::Rect ROI, sROI;
    sROI.width=pgMGUI->mm2px(selArraySpacing->val/1000);
    sROI.height=sROI.width;

    if(go.pGCAM->iuScope->camCols/2-xSize/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX)<0 || go.pGCAM->iuScope->camCols/2-xSize/2-pgBeAn->writeBeamCenterOfsX+xSize>=go.pGCAM->iuScope->camCols){
        err=true;
    }else{
        ROI.x=go.pGCAM->iuScope->camCols/2-xSize/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX);
        ROI.width=xSize;
    }
    if(go.pGCAM->iuScope->camRows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY)<0 || go.pGCAM->iuScope->camRows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY)+ySize>=go.pGCAM->iuScope->camRows){
        err=true;
    }else{
        ROI.y=go.pGCAM->iuScope->camRows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY);
        ROI.height=ySize;
    }
    if(err){
        QMessageBox::critical(gui_activation, "Error", "The calibration ROI does not fit the viewport, aborting calibration.\n");
        btnWriteCalib->setChecked(false);
        delete scanRes;
        return;
    }

    // TODO add selector to force a certain refocus setting
    pgFGUI->doRefocus(true, ROI);

    saveMainConf(util::toString(folder,"/main-settings.txt"));

    const pgScanGUI::scanRes* res;
    // TODO add selector to force a certain scan setting
    pgSGUI->doNRounds((int)selArrayOneScanN->val, ROI, discardMaskRoiThresh, maxRedoScanTries);

    res=scanRes->get();
    pgScanGUI::saveScan(res, util::toString(folder,"/before"), false, true, savePic->val?1:0);


    for(int j=0;j!=WArray.rows; j++) for(int i=0;i!=WArray.cols; i++){   // separate them into individual scans
        cv::utils::fs::createDirectory(util::toString(folder,"/",i+j*WArray.cols));
        sROI.x=pgMGUI->mm2px(i*selArraySpacing->val/1000);
        sROI.y=pgMGUI->mm2px(j*selArraySpacing->val/1000);
        pgScanGUI::saveScan(res, sROI, util::toString(folder,"/",i+j*WArray.cols,"/before"), true, savePic->val?1:0);
    }

    {std::lock_guard<std::mutex>lock(pgSGUI->MLP._lock_proc);
        pgMGUI->chooseObj(false);
        pgMGUI->move(-xOfs,-yOfs,0);
        pgSGUI->MLP.progress_proc=0;
        CTRL::CO CO(go.pRPTY);
        CO.clear(true);
        for(int j=0;j!=WArray.rows; j++){
            for(int i=0;i!=WArray.cols; i++){
                if(!btnWriteCalib->isChecked()){   //abort
                    QMessageBox::critical(gui_activation, "Error", "Calibration aborted.\n");
                    delete scanRes;
                    return;
                }

                pgMGUI->corCOMove(CO,0,0,WArray.at<cv::Vec2d>(j,i)[1]/1000);
                CO.addHold("X",CTRL::he_motion_ontarget);
                CO.addHold("Y",CTRL::he_motion_ontarget);
                CO.addHold("Z",CTRL::he_motion_ontarget);
                CO.pulseGPIO("wrLaser",WArray.at<cv::Vec2d>(j,i)[0]/1000);
                pgMGUI->corCOMove(CO,0,0,-WArray.at<cv::Vec2d>(j,i)[1]/1000);
                saveConf(util::toString(folder,"/",i+j*WArray.cols,"/settings.txt"), WArray.at<cv::Vec2d>(j,i)[0], WArray.at<cv::Vec2d>(j,i)[1]);

                CO.execute();
                CO.clear(true);

                while(CO.getProgress()<0.5) QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
                pgSGUI->MLP.progress_proc=100./(WArray.rows*WArray.cols)*(j*WArray.cols+i);
                if(i!=WArray.cols-1) pgMGUI->corCOMove(CO,selArraySpacing->val/1000,0,0);
            }
            if(j!=WArray.rows-1) pgMGUI->corCOMove(CO,-2*xOfs,selArraySpacing->val/1000,0);
        }
        pgSGUI->MLP.progress_proc=100;
        pgMGUI->move(-xOfs,-yOfs,0);
        pgMGUI->chooseObj(true);
    }

    pgSGUI->doNRounds((int)selArrayOneScanN->val, ROI, discardMaskRoiThresh, maxRedoScanTries,0,savePic->val?1:0);
    res=scanRes->get();
    pgScanGUI::saveScan(res, util::toString(folder,"/after"), false, true, savePic->val?1:0);

    for(int j=0;j!=WArray.rows; j++) for(int i=0;i!=WArray.cols; i++){   // separate them into individual scans
        sROI.x=pgMGUI->mm2px(i*selArraySpacing->val/1000);
        sROI.y=pgMGUI->mm2px(j*selArraySpacing->val/1000);
        pgScanGUI::saveScan(res, sROI, util::toString(folder,"/",i+j*WArray.cols,"/after"), true, savePic->val?1:0);
    }

    btnWriteCalib->setChecked(false);
    delete scanRes;
}

struct pgCalib::_pw{
    int it;
    double weight;
};
bool pgCalib::_pwsort(_pw i,_pw j){return (i.weight>j.weight);}

void pgCalib::WCFAArray(){
    int Nth{0};
    varShareClient<pgScanGUI::scanRes>* scanResPre=pgSGUI->result.getClient();
    varShareClient<pgScanGUI::scanRes>* scanResPost=pgSGUI->result.getClient();
    //first find a good place to go next
    pgSGUI->doOneRound({0,0,0,0},-1);
    while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    const pgScanGUI::scanRes* resPre=scanResPre->get();
    cv::Mat WArray(selAArrayYsize->val,selAArrayXsize->val,CV_64FC2,cv::Scalar(0,0));   //contains Intensities, Durations
    int xSize=selAArrayXsize->val*selAArraySpacing->val*1000/resPre->XYnmppx;   //in px
    int ySize=selAArrayYsize->val*selAArraySpacing->val*1000/resPre->XYnmppx;

redo:
    if(!btnWriteCalib->isChecked()){   //abort
        std::cerr<<"Aborting calibration.\n";
        delete scanResPre;
        delete scanResPost;
        return;
    }
    cv::Mat mask=pgDpEv->getMaskBoundary(resPre, xSize/2, ySize/2);
    cv::bitwise_not(mask,mask);
    std::vector<cv::Point> validPts;
    cv::findNonZero(mask,validPts);
    if(validPts.empty()){     //no valid points
        QMessageBox::critical(this, "Error", "No visible valid points, aborting calibration.\n");
        btnWriteCalib->setChecked(false);
        delete scanResPre;
        delete scanResPost;
        return;
    }
    int toCheck=selAArrayNGenCand->val<validPts.size()?(long)(selAArrayNGenCand->val):validPts.size();
    std::mt19937 rnd(std::random_device{}());
    std::uniform_int_distribution<>dist(0,validPts.size()-1);


    std::vector<_pw> validPtsW;
    validPtsW.reserve(toCheck);
    for(int i=0;i!=toCheck;i++){            // sure, it may get the same point multiple times, but most of the time validPts.size() >> selAArrayNGenCand->val so this chance is too negligible to bother implementing a workaround that would have some computational cost
        int it=dist(rnd);
        cv::Rect roi(validPts[it].x-xSize/2,validPts[it].y-ySize/2,xSize,ySize);
        double weight=cv::mean(cv::Mat(resPre->depth,roi),cv::Mat(resPre->maskN,roi))[0];
        double corRatio=(double)cv::countNonZero(cv::Mat(resPre->mask,roi))/xSize/ySize;
        weight=(1-corRatio)*weight+corRatio*selAArraySetMaskToThisHeight->val;
        validPtsW.push_back({it,weight});
    }
    std::sort(validPtsW.begin(),validPtsW.end(),_pwsort);
    //for(int i=0;i!=toCheck;i++) std::cerr<<"chose "<<validPtsW[i].it<<" "<<validPts[validPtsW[i].it]<<" with weight "<<validPtsW[i].weight<<"\n";
    std::uniform_real_distribution<>dist2(0,1);
    int chosen=sqrt(dist2(rnd))*(toCheck-1);    //this gives a linear distribution with last element being the most likely
    //std::cerr<<"Finally chose "<<validPtsW[chosen].it<<" "<<validPts[validPtsW[chosen].it]<<" with weight "<<validPtsW[chosen].weight<<"\n";

        //go to measurement point
//TODO!        pgMGUI->move((resPre->depth.cols/2+pgBeAn->writeBeamCenterOfsX-validPts[validPtsW[chosen].it].x)*resPre->XYnmppx/1000000,(resPre->depth.rows/2+pgBeAn->writeBeamCenterOfsY-validPts[validPtsW[chosen].it].y)*resPre->XYnmppx/1000000,0);
    validPts.clear(); validPtsW.clear();

        //prepare Int/Dur matrix
    std::uniform_int_distribution<>dist3(selAArrayIntA->val,selAArrayIntB->val);
    std::uniform_real_distribution<>dist4(selAArrayDurA->val,selAArrayDurB->val);
    for(int i=0;i!=WArray.cols; i++) for(int j=0;j!=WArray.rows; j++){
        WArray.at<cv::Vec2d>(j,i)[0]=dist3(rnd);
        WArray.at<cv::Vec2d>(j,i)[1]=dist4(rnd);
    }
    while(!go.pXPS->isQueueEmpty()) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);  //wait for motion to complete

        //do pre writing measurements and save int/dur
    std::string folder=makeDateTimeFolder(saveFolderName);
//TODO!        pgSGUI->doNRounds((int)selAArrayAvgN->val, discardMaskRoiThresh, maxRedoScanTries, cv::Rect(resPre->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX, resPre->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY, xSize, ySize));
    resPre=scanResPre->get();

    double xOfs=((WArray.cols-1)*selAArraySpacing->val)/2000;         //in mm
    double yOfs=((WArray.rows-1)*selAArraySpacing->val)/2000;
    pgMGUI->move(xOfs,yOfs,0);
    for(int j=0;j!=WArray.rows; j++){
        for(int i=0;i!=WArray.cols; i++){
            while(!go.pXPS->isQueueEmpty()) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
 //TODO!           writePulse(WArray.at<cv::Vec2d>(j,i)[0], WArray.at<cv::Vec2d>(j,i)[1]*1000);
            if(i!=WArray.cols-1) pgMGUI->move(-selAArraySpacing->val/1000,0,0);
        }
        if(j!=WArray.rows-1) pgMGUI->move(2*xOfs,-selAArraySpacing->val/1000,0);
    }
    pgMGUI->move(xOfs,yOfs,0);
    while(!go.pXPS->isQueueEmpty()) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);

        //do post writing measurements
 //TODO   pgSGUI->doNRounds((int)selAArrayAvgN->val, discardMaskRoiThresh, maxRedoScanTries, cv::Rect(resPre->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX, resPre->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY, xSize, ySize));
    const pgScanGUI::scanRes* resPost=scanResPost->get();


    std::ofstream wfile(util::toString(folder,"/params.txt"));      //has the following format: <int> <dur(ms)>, is plain text, separated by spaces
    int k=0;
    for(int j=0;j!=WArray.rows; j++) for(int i=0;i!=WArray.cols; i++){   // separate them into individual scans
//TODO!            if(cv::countNonZero(cv::Mat(resPre->mask,cv::Rect(resPre->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX+i*selAArraySpacing->val*1000/resPre->XYnmppx, resPre->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY+j*selAArraySpacing->val*1000/resPre->XYnmppx,
//TODO!                                                              selAArraySpacing->val*1000/resPre->XYnmppx, selAArraySpacing->val*1000/resPre->XYnmppx))) ||
//TODO!               cv::countNonZero(cv::Mat(resPost->mask,cv::Rect(resPost->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX+i*selAArraySpacing->val*1000/resPost->XYnmppx, resPost->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY+j*selAArraySpacing->val*1000/resPost->XYnmppx,
//TODO!                                                               selAArraySpacing->val*1000/resPost->XYnmppx, selAArraySpacing->val*1000/resPost->XYnmppx)))) continue;       //there were bad pixels so we skip
//TODO!            pgScanGUI::saveScan(resPre, cv::Rect(resPre->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX+i*selAArraySpacing->val*1000/resPre->XYnmppx, resPre->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY+j*selAArraySpacing->val*1000/resPre->XYnmppx,
//TODO!                                                  selAArraySpacing->val*1000/resPre->XYnmppx, selAArraySpacing->val*1000/resPre->XYnmppx), util::toString(folder,"/",k,"-pre"),false,false);
//TODO!            pgScanGUI::saveScan(resPost, cv::Rect(resPost->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX+i*selAArraySpacing->val*1000/resPost->XYnmppx, resPost->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY+j*selAArraySpacing->val*1000/resPost->XYnmppx,
//TODO!                                                  selAArraySpacing->val*1000/resPost->XYnmppx, selAArraySpacing->val*1000/resPost->XYnmppx), util::toString(folder,"/",k,"-post"),false,false);
        wfile<<WArray.at<cv::Vec2d>(j,i)[0]<<" "<<WArray.at<cv::Vec2d>(j,i)[1]<<"\n";
        k++;
    }
    wfile.close();

    if(selAArrayDoNMes->val>0) selAArrayDoNMes->setValue(selAArrayDoNMes->val-1);
    std::cerr<<"\nDone #"<<Nth<<", "<<selAArrayDoNMes->val<<" more to go\n\n";
    if(selAArrayDoNMes->val>0) goto redo;

    btnWriteCalib->setChecked(false);
    delete scanResPre;
    delete scanResPost;
}

typedef dlib::matrix<double,2,1> input_vector;
typedef dlib::matrix<double,7,1> parameter_vector;
double gaussResidual(const std::pair<input_vector, double>& data, const parameter_vector& params){
    double x0=params(0);  double x=data.first(0);
    double y0=params(1);  double y=data.first(1);
    double a=params(2);
    double wx=params(3);
    double wy=params(4);
    double an=params(5);
    double i0=params(6);
    //double model=i0+a*exp(-(pow(x-x0,2)+pow(y-y0,2))/2/pow(w,2));
    //double model=i0+a*exp(-(  pow(x-x0,2)/2*(cos(an)/pow(wx,2)+sin(an)/pow(wy,2)) +  pow(y-y0,2)/2*(cos(an)/pow(wy,2)+sin(an)/pow(wx,2)) ));  //seems to be wrong
    double A=pow(cos(an),2)/2/pow(wx,2)+pow(sin(an),2)/2/pow(wy,2);
    double B=sin(2*an)/2/pow(wx,2)-sin(2*an)/2/pow(wy,2);
    double C=pow(sin(an),2)/2/pow(wx,2)+pow(cos(an),2)/2/pow(wy,2);
    double model=i0+a*exp(-A*pow(x-x0,2)-B*(x-x0)*(y-y0)-C*pow(y-y0,2));
    //if(i0<0 || x0<0 || y0<0 || a<0 || a>255) return 9999;
    return model-data.second;
}
bool folderSort(std::string i,std::string j){
    size_t posi=i.find_last_of("/");
    size_t posj=j.find_last_of("/");
    return (std::stoi(i.substr(posi+1,9))<std::stoi(j.substr(posj+1,9)));
}
void pgCalib::onProcessFocusMes(){
    std::vector<std::string> readFolders;   //folders still yet to be checked
    std::vector<std::string> measFolders;   //folders that contain the expected measurement files

    readFolders.emplace_back(QFileDialog::getExistingDirectory(this, "Select Folder Contatining Measurements. It will be Searched Recursively.").toStdString());
    std::string saveName=readFolders.back()+"/proc.txt";
    DIR *wp;
    struct dirent *entry;
    struct stat filetype;
    std::string curFile;
    std::string curFolder;
    bool dirHasMes[5]{false,false,false,false};
    while(!readFolders.empty()){
        curFolder=readFolders.back();
        readFolders.pop_back();
        wp=opendir(curFolder.c_str());
        if(wp!=nullptr) while(entry=readdir(wp)){
            curFile=entry->d_name;
            if (curFile!="." && curFile!=".."){
                curFile=curFolder+'/'+entry->d_name;

                stat(curFile.data(),&filetype);
                if(filetype.st_mode&S_IFDIR) readFolders.push_back(curFile);
                else if (filetype.st_mode&S_IFREG){
                    if(strcmp(entry->d_name,"settings.txt")==0) dirHasMes[0]=true;
                    else if(strcmp(entry->d_name,"before.pfm")==0) dirHasMes[1]=true;
                    else if(strcmp(entry->d_name,"after.pfm")==0) dirHasMes[2]=true;
                    else if(strcmp(entry->d_name,"after-RF.png")==0) dirHasMes[3]=true;       //nonessential for backward compatibility
                }
            }
        }
        closedir(wp);
        if(dirHasMes[0]&dirHasMes[1]&dirHasMes[2]) measFolders.push_back(curFolder);
    }
    std::lock_guard<std::mutex>lock(pgSGUI->MLP._lock_comp);
    pgSGUI->MLP.progress_comp=0;

    //sort folders by number
    std::sort(measFolders.begin(), measFolders.end(), folderSort);

    std::ofstream wfile(saveName);
    int n=0;
    wfile<<"# 1: valid measurement(=1)\n";
    wfile<<"# 2: focus distance(um)\n";
    wfile<<"# 3: peak height(nm)\n";
    wfile<<"# 4: X width (1/e^2)(um)\n";
    wfile<<"# 5: Y width (1/e^2)(um)\n";
    wfile<<"# 6: ellipse angle(rad)\n";
    wfile<<"# 7: XY width (1/e^2)(um)\n";
    wfile<<"# 8: X offset(um)\n";
    wfile<<"# 9: Y offset(um)\n";
    wfile<<"# 10: XY offset(um)\n";
    wfile<<"# 11: not used\n";
    wfile<<"# 12: duration(ms)\n";
    wfile<<"# 13: MeanAbs.ReflectivityDeriv.(a.u.)\n";
    wfile<<"# 14: Max absolute 1st der. (nm/um)\n";
    wfile<<"# 15: Min Laplacian (nm/um^2)\n";
    wfile<<"# 16: Max Laplacian (nm/um^2)\n";
    wfile<<"# 17: peak height(nm)(max)\n";
    wfile<<"# 18: peak height(nm)(max-min)\n";
    wfile<<"# 19: X width (FWHM)(um)\n";
    wfile<<"# 20: Y width (FWHM)(um)\n";
    wfile<<"# 21: XY width (FWHM)(um)\n";

    for(auto& fldr:measFolders){ n++;
        double focus;
        double duration;
        std::ifstream ifs(util::toString(fldr,"/settings.txt"));
        ifs.ignore(std::numeric_limits<std::streamsize>::max(),'\n');       // ignore header
        ifs>>duration;
        ifs>>focus;
        ifs.close();

        pgScanGUI::scanRes scanBefore, scanAfter;
        if(!pgScanGUI::loadScan(&scanBefore, util::toString(fldr,"/before.pfm"))) continue;
        if(!pgScanGUI::loadScan(&scanAfter, util::toString(fldr,"/after.pfm"))) continue;
        pgScanGUI::scanRes scanDif=pgSGUI->difScans(&scanBefore, &scanAfter);
        if(cropTop->value()!=0 || cropBttm->value()!=0 || cropLeft->value()!=0 || cropRght->value()!=0){
            if(cropTop->value()+cropBttm->value()>=scanDif.depth.rows || cropRght->value()+cropLeft->value()>=scanDif.depth.cols) {std::cerr<<"Cropped dimensions are larger than scan sizes. Aborting processing.\n"; return;}
            scanDif.mask =scanDif.mask (cv::Rect(cropLeft->value(), cropTop->value(), scanDif.depth.cols-cropLeft->value()-cropRght->value(), scanDif.depth.rows-cropTop->value()-cropBttm->value()));
            scanDif.depth=scanDif.depth(cv::Rect(cropLeft->value(), cropTop->value(), scanDif.depth.cols-cropLeft->value()-cropRght->value(), scanDif.depth.rows-cropTop->value()-cropBttm->value()));
            scanDif.maskN=scanDif.maskN(cv::Rect(cropLeft->value(), cropTop->value(), scanDif.depth.cols-cropLeft->value()-cropRght->value(), scanDif.depth.rows-cropTop->value()-cropBttm->value()));
        }
//        pgScanGUI::saveScan(&scanDif, util::toString(fldr,"/scandif.pfm"));

//        cv::Mat rescaleDepth, rescaleMask;                                    //this can be done to increase performance
//        cv::resize(scanDif.depth,rescaleDepth,cv::Size(),0.2,0.2);            //dont forget to rescale the measured widths
//        cv::resize(scanDif.mask,rescaleMask,cv::Size(),0.2,0.2);

        std::vector<std::pair<input_vector, double>> data;
        for(int i=0;i!=scanDif.depth.cols;i++) for(int j=0;j!=scanDif.depth.rows;j++)
            if(scanDif.mask.at<uchar>(j,i)==0) data.push_back(std::make_pair(input_vector{(double)i,(double)j},scanDif.depth.at<float>(j,i)));

        parameter_vector res{(double)scanDif.depth.cols/2,(double)scanDif.depth.rows/2,scanDif.max-scanDif.min,(double)scanDif.depth.rows, (double)scanDif.depth.rows, 0.01, scanDif.min};

        dlib::solve_least_squares_lm(dlib::objective_delta_stop_strategy(1e-7,100), gaussResidual, derivative(gaussResidual), data, res);
        while(res(5)>M_PI) res(5)-=M_PI;
        while(res(5)<0) res(5)+=M_PI;
        if(res(5)>=M_PI/2){
           res(5)-= M_PI/2;
           double tmp=res(3);
           res(3)=res(4);
           res(4)=tmp;
        }

        double intReflDeriv{-1};
        if(dirHasMes[4]){
            cv::Mat refl=imread(util::toString(fldr,"/after-RF.png"), cv::IMREAD_GRAYSCALE);
            if(refl.empty()) refl=imread(util::toString(fldr,"/pic.png"), cv::IMREAD_GRAYSCALE);         // pic.png for backward compatibility
            cv::Mat reflS;
            cv::Mat derv, dervy;
            cv::bilateralFilter(refl, reflS, -1, 4, 4);  //smooth it a bit
            cv::Sobel(reflS, derv, CV_32F,1,0);          //first derivative
            cv::Sobel(reflS, dervy, CV_32F,0,1);
            cv::add(derv, dervy, derv);
            intReflDeriv=cv::mean(cv::abs(derv))[0];
        }

        //find max abs first derivative of depth (nm/um):
        cv::Mat com, derv, dervy;
        cv::bilateralFilter(scanDif.depth, com, -1, 3, 3);  //smooth it a bit
        cv::Sobel(com, derv, CV_32F,1,0);                   //first derivative
        cv::Sobel(com, dervy, CV_32F,0,1);                  //first derivative
        double maxDepthDer;
        cv::add(derv, dervy, derv);
        cv::minMaxIdx(derv, nullptr, &maxDepthDer);
        //find min, max laplacian of depth (nm/um^2):
        cv::Laplacian(com, derv, CV_32F);
        double minDepthLaplacian, maxDepthLaplacian;
        cv::minMaxIdx(derv, &minDepthLaplacian, &maxDepthLaplacian);

        double XYumppx=scanDif.XYnmppx/1000;
        maxDepthDer/=XYumppx;
        minDepthLaplacian/=XYumppx;
        maxDepthLaplacian/=XYumppx;

        double Xwidth=2*abs(res(3))*XYumppx;
        double Ywidth=2*abs(res(4))*XYumppx;
        double XYwidth=(Xwidth+Ywidth)/2;
        double Xofs=res(0)*XYumppx;
        double Yofs=res(1)*XYumppx;
        double XYofs=sqrt(pow(Xofs,2)+pow(Yofs,2));
        const double toFWHM=sqrt(2*log(2));

        int valid=1;
        if(res(0)<0 || res(0)>scanDif.depth.cols || res(1)<0 || res(1)>scanDif.depth.rows || res(2)<=0) valid=0;    //center of the fit is out of frame or other things that indicate fit faliure
        wfile<<valid<<" ";                      // 1: valid measurement(=1)
        wfile<<focus<<" ";                      // 2: focus distance(um)
        wfile<<res(2)<<" ";                     // 3: peak height(nm)
        wfile<<Xwidth<<" ";                     // 4: X width (1/e^2)(um)
        wfile<<Ywidth<<" ";                     // 5: Y width (1/e^2)(um)
        wfile<<res(5)<<" ";                     // 6: ellipse angle(rad)
        wfile<<XYwidth<<" ";                    // 7: XY width (1/e^2)(um)
        wfile<<Xofs<<" ";                       // 8: X offset(um)
        wfile<<Yofs<<" ";                       // 9: Y offset(um)
        wfile<<XYofs<<" ";                      // 10: XY offset(um)
        wfile<<0<<" ";                          // 11: not used
        wfile<<duration<<" ";                   // 12: duration(ms)
        wfile<<intReflDeriv<<" ";               // 13: MeanAbs.ReflectivityDeriv.(a.u.)
        wfile<<maxDepthDer<<" ";                // 14: Max absolute 1st der. (nm/um)
        wfile<<minDepthLaplacian<<" ";          // 15: Min Laplacian (nm/um^2)
        wfile<<maxDepthLaplacian<<" ";          // 16: Max Laplacian (nm/um^2)
        wfile<<scanDif.max<<" ";                // 17: peak height(nm)(max)
        wfile<<scanDif.max-scanDif.min<<" ";    // 18: peak height(nm)(max-min)
        wfile<<Xwidth*toFWHM<<" ";              // 19: X width (FWHM)(um)
        wfile<<Ywidth*toFWHM<<" ";              // 20: Y width (FWHM)(um)
        wfile<<XYwidth*toFWHM<<"\n";            // 21: XY width (FWHM)(um)

        pgSGUI->MLP.progress_comp=100./measFolders.size()*n;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
    }
    wfile.close();
    pgSGUI->MLP.progress_comp=100;
}

void pgCalib::onChangeDrawWriteAreaOn(bool status){
    drawWriteAreaOn=status;
}
void pgCalib::drawWriteArea(cv::Mat* img){
    if(!drawWriteAreaOn) return;
    double xSize;
    double ySize;
    switch(calibMethod->index){
    case 0: xSize=pgMGUI->mm2px(selArrayXsize->val*selArraySpacing->val/1000);
            ySize=pgMGUI->mm2px(selArrayYsize->val*selArraySpacing->val/1000);
            if(transposeMat->val) std::swap(xSize,ySize);
            break;
    case 1: xSize=pgMGUI->mm2px(selAArrayXsize->val*selAArraySpacing->val/1000);
            ySize=pgMGUI->mm2px(selAArrayYsize->val*selAArraySpacing->val/1000);
            break;
    case 2: xSize=pgMGUI->mm2px(selWriteCalibFocusRadDil->val/1000);
            ySize=pgMGUI->mm2px(selWriteCalibFocusRadDil->val/1000);
            break;
    }
    double clr[2]={0,255}; int thck[2]={3,1};
    for(int i=0;i!=2;i++)
        cv::rectangle(*img,  cv::Rect(img->cols/2-xSize/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX), img->rows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY), xSize, ySize), {clr[i]}, thck[i], cv::LINE_AA);
}
