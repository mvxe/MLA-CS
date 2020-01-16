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
    btnGoToNearestFree=new QPushButton("Go to nearest free");
    btnGoToNearestFree->setToolTip("This adheres to write boundaries!");
    connect(btnGoToNearestFree, SIGNAL(released()), this, SLOT(onGoToNearestFree()));
    hcGoToNearestFree=new hidCon(btnGoToNearestFree);
    alayout->addWidget(hcGoToNearestFree);
    selRadDilGoToNearestFree=new val_selector(1, "pgCalib_selRadDilGoToNearestFree", "Exclusion Dilation Radius: ", 0, 100, 2, 0, {"um"});
    selRadSprGoToNearestFree=new val_selector(1, "pgCalib_selRadSprGoToNearestFree", "Random Selection Radius: ", 0, 100, 2, 0, {"um"});
    hcGoToNearestFree->addWidget(selRadDilGoToNearestFree);
    hcGoToNearestFree->addWidget(selRadSprGoToNearestFree);
    btnSelectSaveFolder=new QPushButton("Select save folder");
    connect(btnSelectSaveFolder, SIGNAL(released()), this, SLOT(onSelSaveF()));
    alayout->addWidget(new twid(btnSelectSaveFolder));

    btnWriteCalibFocus=new QPushButton("Calibrate Write Focus");
    btnWriteCalibFocus->setEnabled(false);
    connect(btnWriteCalibFocus, SIGNAL(released()), this, SLOT(onWCF()));
    btnWriteCalibFocus->setCheckable(true);
    alayout->addWidget(new twid(btnWriteCalibFocus));

    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);

    calibMethod=new twds_selector("pgCalib_calibMethod",0,"Select method:", "Find Nearest", false, false, false);
    calibMethodFindNearest=new vtwid;
        calibMethodFindNearest->addWidget(new QLabel("Write Focus Calibration Measurements"));
        selWriteCalibFocusDoNMeas=new val_selector(100, "pgCalib_selWriteCalibFocusDoNMeas", "Do this many measurements: ", 1, 100000, 0);
        calibMethodFindNearest->addWidget(selWriteCalibFocusDoNMeas);
        selWriteCalibFocusReFocusNth=new val_selector(5, "pgCalib_selWriteCalibFocusReFocusNth", "Refocus every this many measurements: ", 0, 1000, 0);
        selWriteCalibFocusReFocusNth->setToolTip("Set 0 to disable refocusing.");
        calibMethodFindNearest->addWidget(selWriteCalibFocusReFocusNth);
        selWriteCalibFocusRadDil=new val_selector(1, "pgCalib_selWriteCalibFocusRadDil", "Exclusion Dilation / save ROI Radius: ", 0, 100, 2, 0, {"um"});
        selWriteCalibFocusRadDil->setToolTip("This should be >3x your beam spot size...");
        calibMethodFindNearest->addWidget(selWriteCalibFocusRadDil);
        selWriteCalibFocusRadSpr=new val_selector(1, "pgCalib_selWriteCalibFocusRadSpr", "Random Selection Radius: ", 0, 100, 2, 0, {"um"});
        calibMethodFindNearest->addWidget(selWriteCalibFocusRadSpr);
        selWriteCalibFocusBlur=new val_selector(2, "pgCalib_selWriteCalibFocusBlur", "Gaussian Blur Sigma: ", 0, 100, 1, 0, {"px"});
        calibMethodFindNearest->addWidget(selWriteCalibFocusBlur);
        selWriteCalibFocusThresh=new val_selector(0.2, "pgCalib_selWriteCalibFocusThresh", "2nd Derivative Exclusion Threshold: ", 0, 1, 4);
        selWriteCalibFocusThresh->setToolTip("Try out values in Depth Eval.");
        calibMethodFindNearest->addWidget(selWriteCalibFocusThresh);
        selWriteCalibFocusRange=new val_selector(0, "pgCalib_selWriteCalibFocusRange", "Measurement range around focus: ", 0, 1000, 3, 0, {"um"});
        selWriteCalibFocusRange->setToolTip("Each measurement will be done at a random write beam focus around the starting focus\u00B1 this parameter.");
        calibMethodFindNearest->addWidget(selWriteCalibFocusRange);
    //    selWriteCalibFocusMoveOOTW=new checkbox_save(false,"pgCalib_selWriteCalibFocusMoveOOTW","Take direct picture of the beam before each measurement.");
    //    calibMethodFindNearestLayout->addWidget(selWriteCalibFocusMoveOOTW);
    //    selWriteCalibFocusMoveOOTWDis=new val_selector(0, "pgCalib_selWriteCalibFocusMoveOOTWDis", "Move X this much to get out of the way: ", -1000, 1000, 3, 0, {"mm"});
    //    calibMethodFindNearestLayout->addWidget(selWriteCalibFocusMoveOOTWDis);
        selWriteCalibFocusPulseIntensity=new val_selector(1000, "pgCalib_selWriteCalibFocusPulseIntensity", "Pulse Intensity: ", 0, 8192, 0);
        calibMethodFindNearest->addWidget(selWriteCalibFocusPulseIntensity);
        selWriteCalibFocusPulseDuration=new val_selector(10, "pgCalib_selWriteCalibFocusPulseDuration", "Pulse Duration: ", 0.008, 1000000, 3, 0, {"us"});
        calibMethodFindNearest->addWidget(selWriteCalibFocusPulseDuration);
    calibMethod->addWidget(calibMethodFindNearest,"Find Nearest");
    calibMethodArray=new vtwid;
        selArrayXsize=new val_selector(10, "pgCalib_selArrayXsize", "Array Size X", 1, 1000, 0);
        calibMethodArray->addWidget(selArrayXsize);
        selArrayYsize=new val_selector(10, "pgCalib_selArrayYsize", "Array Size Y", 1, 1000, 0);
        calibMethodArray->addWidget(selArrayYsize);
        selArraySpacing=new val_selector(5, "pgCalib_selArraySpacing", "Array Spacing", 0.001, 100, 3, 0, {"um"});
        calibMethodArray->addWidget(selArraySpacing);
        selArrayType=new smp_selector("pgCalib_selArrayType", "Variable Parameters (X-Y): ", 0, {"Intensity","Duration","Focus","Intensity-Duration","Intensity-Focus","Duration-Focus"});
        calibMethodArray->addWidget(selArrayType);
        calibMethodArray->addWidget(new QLabel("The Variable Parameters Will be Within the Specified Range.\nIf a Parameter is not Variable, it Will be Equal to Val A!"));
        selArrayIntA=new val_selector(1000, "pgCalib_selArrayIntA", "Intensity Value A", 1, 8192, 0);
        calibMethodArray->addWidget(selArrayIntA);
        selArrayIntB=new val_selector(1000, "pgCalib_selArrayIntB", "Intensity Value B", 1, 8192, 0);
        calibMethodArray->addWidget(selArrayIntB);
        selArrayDurA=new val_selector(1, "pgCalib_selArrayDurA", "Duration Value A", 0.001, 1000, 3, 0, {"ms"});
        calibMethodArray->addWidget(selArrayDurA);
        selArrayDurB=new val_selector(1, "pgCalib_selArrayDurB", "Duration Value B", 0.001, 1000, 3, 0, {"ms"});
        calibMethodArray->addWidget(selArrayDurB);
        selArrayFocA=new val_selector(-1, "selArrayFocA", "Focus Value A", -1000, 1000, 3, 0, {"um"});
        calibMethodArray->addWidget(selArrayFocA);
        selArrayFocB=new val_selector(1, "selArrayFocB", "Focus Value B", -1000, 1000, 3, 0, {"um"});
        calibMethodArray->addWidget(selArrayFocB);
        selArrayScanType=new smp_selector("pgCalib_selArrayScanType", "Scan Type: ", 0, {"One Scan (Average N Parameter Below Applies)","Multi Scan (One Scan Per Write)"});
        calibMethodArray->addWidget(selArrayScanType);
        selArrayOneScanN=new val_selector(10, "pgCalib_selArrayOneScanN", "Average This Many Scans (For One Scan Setting): ", 1, 1000, 0);
        calibMethodArray->addWidget(selArrayOneScanN);
        selArrayRandomize=new checkbox_save(false,"pgCalib_selArrayRandomize","Randomize Value Order");
        calibMethodArray->addWidget(selArrayRandomize);
        showLimits=new checkbox_save(false,"pgCalib_showLimits","Show Limits on Display");
        calibMethodArray->addWidget(showLimits);
        saveMats=new checkbox_save(true,"pgCalib_saveMats","Extra Save Mats Containing I,D,F for Convenience.");
        calibMethodArray->addWidget(saveMats);
    calibMethod->addWidget(calibMethodArray, "Array");
    calibMethod->doneAddingWidgets();
    slayout->addWidget(calibMethod);

    gui_processing=new QWidget;
    playout=new QVBoxLayout;
    gui_processing->setLayout(playout);
    btnProcessFocusMes=new QPushButton("Select Folders to Process Focus Measurements");
    connect(btnProcessFocusMes, SIGNAL(released()), this, SLOT(onProcessFocusMes()));
    playout->addWidget(new twid(btnProcessFocusMes));

    scanRes=pgSGUI->result.getClient();
}
pgCalib::~pgCalib(){
    delete scanRes;
}

void pgCalib::onGoToNearestFree(){
    goToNearestFree(selRadDilGoToNearestFree->val, selRadSprGoToNearestFree->val);
}
bool pgCalib::goToNearestFree(double radDilat, double radRandSpread){
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
    cv::Mat mask=pgDpEv->getMaskFlatness(res, res->XYnmppx, dil, selWriteCalibFocusThresh->val, selWriteCalibFocusBlur->val);
    int ptX,ptY;
    imgAux::getNearestFreePointToCenter(&mask, pgBeAn->writeBeamCenterOfsX, pgBeAn->writeBeamCenterOfsY, ptX, ptY, radRandSpread);
    if(ptX==-1){
        std::cerr<<"No free nearby!\n";
        return true;
    }

    double dXumm, dYumm, dXmm, dYmm;
    dXumm=(ptX-res->depth.cols/2)*res->XYnmppx/1000000;
    dYumm=(ptY-res->depth.rows/2)*res->XYnmppx/1000000;
    dXmm=dXumm*cos(pgMGUI->getAngCamToXMot())+dYumm*sin(pgMGUI->getAngCamToXMot()+pgMGUI->getYMotToXMot());
    dYmm=dXumm*sin(pgMGUI->getAngCamToXMot())+dYumm*cos(pgMGUI->getAngCamToXMot()+pgMGUI->getYMotToXMot());

    pgMGUI->move(-dXmm,-dYmm,0,0);
    return false;
}
void pgCalib::onSelSaveF(){
    std::string temp=QFileDialog::getExistingDirectory(this, "Select Folder for Saving Calibration Data").toStdString();
    if(temp.empty()) return;
    saveFolderName=temp;
    btnWriteCalibFocus->setEnabled(true);
}

void pgCalib::onWCF(){
    if(!btnWriteCalibFocus->isChecked()) return;
    if(!go.pRPTY->connected) {QMessageBox::critical(this, "Error", "Error: Red Pitaya not Connected"); return;}
    if(!go.pXPS->connected) {QMessageBox::critical(this, "Error", "Error: XPS not Connected"); return;}
    switch(calibMethod->index){
    case 0: WCFFindNearest();
            break;
    case 1: WCFArray();
            break;
    }
}

void pgCalib::writePulse(int intensity, double duration, const std::string filename, uchar* usedAvg, const int cmdQueue, const int recQueue){
    uchar selectedavg=0;
    int domax=go.pRPTY->getNum(RPTY::A2F_RSMax,recQueue)*0.99;  //we make it a bit smaller to make sure all fits in
    double pulsedur=duration*1.1;                               //we make it longer in order to catch the last part of the waveform too
    while(domax*(8e-3)*(1<<selectedavg)<pulsedur) selectedavg++;
    std::vector<uint32_t> commands;    //do actual writing
    commands.push_back(CQF::W4TRIG_INTR());
    commands.push_back(CQF::TRIG_OTHER(1<<tab_monitor::RPTY_A2F_queue));    //RPTY_A2F_queue for debugging purposes
    if(!filename.empty()) commands.push_back(CQF::ACK(1<<recQueue, selectedavg, CQF::fADC_A__fADC_B, true));
    else commands.push_back(CQF::WAIT(1));
    commands.push_back(CQF::SG_SAMPLE(CQF::O0td, intensity, 0));
    commands.push_back(CQF::WAIT(duration/8e-3 - 1));
    commands.push_back(CQF::SG_SAMPLE(CQF::O0td, 0, 0));
    commands.push_back(CQF::WAIT(0.1*duration/8e-3 - 1));
    if(!filename.empty()) commands.push_back(CQF::ACK(1<<recQueue, selectedavg, CQF::fADC_A__fADC_B, false));
    go.pRPTY->A2F_write(cmdQueue,commands.data(),commands.size());
    go.pRPTY->trig(1<<cmdQueue);
    commands.clear();

    if(usedAvg!=nullptr) *usedAvg=selectedavg;
    if(!filename.empty()){  //save writing beam waveform
        while(go.pRPTY->getNum(RPTY::A2F_RSCur,cmdQueue)!=0)  QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        uint32_t toread=go.pRPTY->getNum(RPTY::F2A_RSCur,recQueue);
        std::vector<uint32_t> read;
        read.reserve(toread);
        go.pRPTY->F2A_read(recQueue,read.data(),toread);
        std::ofstream wfile(util::toString(filename));
        //gnuplot: plot "laser.dat" binary format='%int16%int16' using 0:1 with lines, "laser.dat" binary format='%int16%int16' using 0:2 with lines
        int16_t tmp;
        for(int i=0; i!=toread; i++){
            tmp=AQF::getChMSB(read[i]);
            wfile.write(reinterpret_cast<const char*>(&(tmp)),sizeof(tmp));
            tmp=AQF::getChLSB(read[i]);
            wfile.write(reinterpret_cast<const char*>(&(tmp)),sizeof(tmp));
        }
        wfile.close();
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

void pgCalib::saveConf(std::string filename, double focus, double exclusionOrSpacing, int intensity, double duration, uchar averaging, double focusBeamRad){
    std::ofstream setFile(filename);            //this file contains some settings:
    setFile <<focus<<"\n"                       // line0:   the focus in mm (stage)
            <<exclusionOrSpacing<<"\n"          // line1:   Exclusion radius / ROI radius in um
            <<intensity<<"\n"                   // line2:   Pulse Intensity
            <<duration<<"\n"                    // line3:   Pulse Duration in ms
            <<(int)averaging<<"\n"              // line4:   Averaging Used in Pulse Raw Data (ie. the time difference between datapoints will be selectedavg*8e-9 sec)
            <<focusBeamRad<<"\n";               // line5:   The Somewhat Arbitrary but Self Consistent Measured Focus Beam Radius
    setFile.close();
}

void pgCalib::saveConfMain(std::string filename, double focus, double extraFocusOffset, double focusBeamRad){
    std::ofstream setFile(filename);            //this file contains some settings:
    setFile <<focus<<"\n"                       // line0:   the focus in mm (stage)
            <<extraFocusOffset<<"\n"            // line1:   Extra focus offset - the difference between the calibration(red) beam focus and the writing(green) beam focus
            <<focusBeamRad<<"\n";               // line2:   The Somewhat Arbitrary but Self Consistent Measured Focus Beam Radius
    setFile.close();
}

void pgCalib::WCFFindNearest(){
    if(goToNearestFree(selWriteCalibFocusRadDil->val,selWriteCalibFocusRadSpr->val)) {QMessageBox::critical(this, "Error", "No free nearby, stopping."); btnWriteCalibFocus->setChecked(false); return;}
    QCoreApplication::processEvents(QEventLoop::AllEvents, 500);    //some waiting time for the system to stabilize after a rapid move

    std::string folder=makeDateTimeFolder(saveFolderName);

    if((int)(selWriteCalibFocusReFocusNth->val)!=0)
    if(!(measCounter%((int)(selWriteCalibFocusReFocusNth->val)))){
        pgFGUI->refocus();
        while(!pgFGUI->focusingDone) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
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

    int didF{0};
    redoF: std::mt19937 rnd(std::random_device{}());
    std::uniform_real_distribution<>dist(-selWriteCalibFocusRange->val, selWriteCalibFocusRange->val);
    double wrFocusOffs=dist(rnd);
    wrFocusOffs=round(wrFocusOffs*1000)/1000;   //we round it up to 1 nm precision
    std::cerr<<"wrFocusOffs is: "<<wrFocusOffs<<" um\n";
    double focus=pgMGUI->FZdifference;
    std::cerr<<"Focus is: "<<focus<<" mm\n";
    pgMGUI->moveZF(focus+wrFocusOffs/1000);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait a bit for movement to complete and stabilize
    std::cerr<<"new focus is: "<<focus+wrFocusOffs/1000<<" mm\n";

    float focusRad; // in reality this is not a radius, but weight : all pixels intensities integrated
    if(pgBeAn->getCalibWritingBeam(&focusRad)){     // recenter writing beam and get radius
        std::cerr<<"Failed to analyse the read beam, retrying...\n";
        if(pgBeAn->getCalibWritingBeam(&focusRad)){ // retry if failed
            std::cerr<<"Cannot analyse the read beam at this focus, trying another...\n";
            pgMGUI->moveZF(focus);
            didF++;
            if(didF<10) goto redoF;
            else{
                std::cerr<<"Failed"<<didF<<"times. Aborting.\n";
                btnWriteCalibFocus->setChecked(false);
                return;
            }
        }
    }

    redoA:  pgSGUI->doOneRound();
    while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
    const pgScanGUI::scanRes* res=scanRes->get();
    int roiD=2*selWriteCalibFocusRadDil->val*1000/res->XYnmppx;
    if(cv::countNonZero(cv::Mat(res->mask, cv::Rect(res->depth.cols/2-roiD/2, res->depth.rows/2-roiD/2, roiD, roiD)))>roiD*roiD*(4-M_PI)/4){std::cerr<<"To much non zero mask in ROI; redoing measurement.\n";goto redoA;}      //this is (square-circle)/4
    pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-roiD/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-roiD/2+pgBeAn->writeBeamCenterOfsY, roiD, roiD), util::toString(folder,"/before"));

    uchar selectedavg;
    writePulse(selWriteCalibFocusPulseIntensity->val, selWriteCalibFocusPulseDuration->val, util::toString(folder,"/laser.dat"), &selectedavg);

    pgMGUI->moveZF(focus);

    saveConf(util::toString(folder,"/settings.txt"), focus+wrFocusOffs/1000, selWriteCalibFocusRadDil->val, selWriteCalibFocusPulseIntensity->val, selWriteCalibFocusPulseDuration->val, selectedavg, focusRad);

    redoB:  pgSGUI->doOneRound();
    while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
    res=scanRes->get();
    if(cv::countNonZero(cv::Mat(res->mask, cv::Rect(res->depth.cols/2-roiD/2, res->depth.rows/2-roiD/2, roiD, roiD)))>roiD*roiD*(4-M_PI)/4){std::cerr<<"To much non zero mask in ROI; redoing measurement.\n";goto redoB;}
    pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-roiD/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-roiD/2+pgBeAn->writeBeamCenterOfsY, roiD, roiD), util::toString(folder,"/after"));

    if(btnWriteCalibFocus->isChecked()){
        measCounter++;
        if(measCounter<(int)selWriteCalibFocusDoNMeas->val) return WCFFindNearest();
        else {btnWriteCalibFocus->setChecked(false); measCounter=0;}
    } else measCounter=0;
}

void pgCalib::WCFArray(){
    cv::Mat WArray(selArrayYsize->val,selArrayXsize->val,CV_64FC4,cv::Scalar(0,0,0,0));   //Intensity, Duration(ms), Focus(um)
    int arraySizeInt{1}, arraySizeDur{1}, arraySizeFoc{1};
    int index=selArrayType->index;
    switch(index){
    case 0: arraySizeInt=selArrayYsize->val*selArrayXsize->val;     //Intensity
            break;
    case 1: arraySizeDur=selArrayYsize->val*selArrayXsize->val;     //Duration
            break;
    case 2: arraySizeFoc=selArrayYsize->val*selArrayXsize->val;     //Focus
            break;
    case 3: arraySizeInt=selArrayXsize->val;                        //Intensity-Duration
            arraySizeDur=selArrayYsize->val;
            break;
    case 4: arraySizeInt=selArrayXsize->val;                        //Intensity-Focus
            arraySizeFoc=selArrayYsize->val;
            break;
    case 5: arraySizeDur=selArrayXsize->val;                        //Duration-Focus
            arraySizeFoc=selArrayYsize->val;
            break;
    }
    std::vector<double> arrayInt; arrayInt.reserve(arraySizeInt);
    std::vector<double> arrayDur; arrayDur.reserve(arraySizeDur);
    std::vector<double> arrayFoc; arrayFoc.reserve(arraySizeFoc);
    if(arraySizeInt==1) arrayInt.push_back(selArrayIntA->val);
    else for(int i=0;i!=arraySizeInt; i++) arrayInt.push_back(selArrayIntA->val*(1-(double)i/(arraySizeInt-1))+selArrayIntB->val*((double)i/(arraySizeInt-1)));
    if(arraySizeDur==1) arrayDur.push_back(selArrayDurA->val);
    else for(int i=0;i!=arraySizeDur; i++) arrayDur.push_back(selArrayDurA->val*(1-(double)i/(arraySizeDur-1))+selArrayDurB->val*((double)i/(arraySizeDur-1)));
    if(arraySizeFoc==1) arrayFoc.push_back(selArrayFocA->val);
    else for(int i=0;i!=arraySizeFoc; i++) arrayFoc.push_back(selArrayFocA->val*(1-(double)i/(arraySizeFoc-1))+selArrayFocB->val*((double)i/(arraySizeFoc-1)));
    if(selArrayRandomize->val){                         //randomize if chosen
        std::mt19937 rnd(std::random_device{}());
        std::shuffle(arrayInt.begin(), arrayInt.end(), rnd);
        std::shuffle(arrayDur.begin(), arrayDur.end(), rnd);
        std::shuffle(arrayFoc.begin(), arrayFoc.end(), rnd);
    }
    for(int i=0;i!=WArray.cols; i++) for(int j=0;j!=WArray.rows; j++){              //populate 3D x3 array
        if(index==0) WArray.at<cv::Vec3d>(j,i)[0]=arrayInt[i+j*WArray.cols];
        else if(index==3 || index==4) WArray.at<cv::Vec3d>(j,i)[0]=arrayInt[i];
        else WArray.at<cv::Vec3d>(j,i)[0]=arrayInt[0];
        if(index==1) WArray.at<cv::Vec3d>(j,i)[1]=arrayDur[i+j*WArray.cols];
        else if(index==3)  WArray.at<cv::Vec3d>(j,i)[1]=arrayDur[j];
        else if(index==5) WArray.at<cv::Vec3d>(j,i)[1]=arrayDur[i];
        else WArray.at<cv::Vec3d>(j,i)[1]=arrayDur[0];
        if(index==2) WArray.at<cv::Vec3d>(j,i)[2]=arrayFoc[i+j*WArray.cols];
        else if(index==4 || index==5)  WArray.at<cv::Vec3d>(j,i)[2]=arrayFoc[j];
        else WArray.at<cv::Vec3d>(j,i)[2]=arrayFoc[0];
    }

    std::string folder=makeDateTimeFolder(saveFolderName);
    if(saveMats->val){      //export values as matrices, for convinience
        std::string names[3]={"Intensity","Duration","Focus"};
        for(int k=0;k!=3;k++){
            std::ofstream wfile(util::toString(folder,"/", names[k],".txt"));
            for(int i=0;i!=WArray.cols; i++){
                for(int j=0;j!=WArray.rows; j++)
                    wfile<<WArray.at<cv::Vec3d>(j,i)[k]<<" ";
                wfile<<"\n";
            }
            wfile.close();
        }
    }

    float focusRad;
    if(pgBeAn->getCalibWritingBeam(&focusRad)) if(pgBeAn->getCalibWritingBeam(&focusRad)){      // recenter writing beam and get radius
        std::cerr<<"Failed to analyse/center the read beam after two tries.\n";
        btnWriteCalibFocus->setChecked(false);
        return;
    }
    pgBeAn->getWritingBeamFocus();
    double focus=pgMGUI->FZdifference;

    saveConfMain(util::toString(folder,"/main-settings.txt"), focus, *pgBeAn->extraFocusOffsetVal, focusRad);

    double xSize=WArray.cols*selArraySpacing->val/1000;         //in mm
    double ySize=WArray.rows*selArraySpacing->val/1000;
    double xOfs=((WArray.cols-1)*selArraySpacing->val)/2000;
    double yOfs=((WArray.rows-1)*selArraySpacing->val)/2000;

//    if(selArrayScanType->index==0){ //single(or multiple averaged) mesurement
//        redoA:  pgSGUI->doOneRound(-1);
//        while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
//        const pgScanGUI::scanRes* res=scanRes->get();
//        if(cv::countNonZero(cv::Mat(res->mask, cv::Rect(res->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY, xSize, ySize)))>xSize*ySize*0.0001){std::cerr<<"To much non zero mask in ROI (>0.1%); redoing measurement.\n";goto redoA;}

//        while(res->avgNum!=(int)selArrayOneScanN->val){
//            pgSGUI->doOneRound(1);

//            while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
//            res=scanRes->get();
//        }
//        pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY, xSize, ySize), util::toString(folder,"/before"));
//    }

    //PROBLEM HERE WAS xOfs are in mm, and ySize needs to be in px

    pgMGUI->move(xOfs,yOfs,0,0);

    for(int j=0;j!=WArray.rows; j++){
        for(int i=0;i!=WArray.cols; i++){
            pgMGUI->moveZF(focus+WArray.at<cv::Vec3d>(j,i)[2]/1000);
            while(!go.pXPS->isQueueEmpty()) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);  //wait for motion to complete

            if(pgBeAn->getCalibWritingBeam(&focusRad,nullptr,nullptr,false)) if(pgBeAn->getCalibWritingBeam(&focusRad,nullptr,nullptr,false)){      // recenter writing beam and get radius
                focusRad=-1;
            }

            uchar selectedavg;
            cv::utils::fs::createDirectory(util::toString(folder,"/",i+j*WArray.cols));

            if(selArrayScanType->index==1){ //multiple mesurement
                redoA:  pgSGUI->doOneRound(-1);
                while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
                const pgScanGUI::scanRes* res=scanRes->get();
                int roiD=selArraySpacing->val*1000/res->XYnmppx;
                if(cv::countNonZero(cv::Mat(res->mask, cv::Rect(res->depth.cols/2-roiD/2, res->depth.rows/2-roiD/2, roiD, roiD)))>roiD*roiD*(4-M_PI)/4){std::cerr<<"To much non zero mask in ROI; redoing measurement.\n";goto redoA;}      //this is (square-circle)/4
                pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-roiD/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-roiD/2+pgBeAn->writeBeamCenterOfsY, roiD, roiD), util::toString(folder,"/",i+j*WArray.cols,"/before"));
            }

            //todo abort when overflow
            //todo abort midway
            writePulse(WArray.at<cv::Vec3d>(j,i)[0], WArray.at<cv::Vec3d>(j,i)[1]*1000, util::toString(folder,"/",i+j*WArray.cols,"/laser.dat"), &selectedavg);

            if(selArrayScanType->index==1){ //multiple mesurement
                redoB:  pgSGUI->doOneRound(-1);
                while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
                const pgScanGUI::scanRes* res=scanRes->get();
                int roiD=selArraySpacing->val*1000/res->XYnmppx;
                if(cv::countNonZero(cv::Mat(res->mask, cv::Rect(res->depth.cols/2-roiD/2, res->depth.rows/2-roiD/2, roiD, roiD)))>roiD*roiD*(4-M_PI)/4){std::cerr<<"To much non zero mask in ROI; redoing measurement.\n";goto redoB;}      //this is (square-circle)/4
                pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-roiD/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-roiD/2+pgBeAn->writeBeamCenterOfsY, roiD, roiD), util::toString(folder,"/",i+j*WArray.cols,"/after"));
            }

            saveConf(util::toString(folder,"/",i+j*WArray.cols,"/settings.txt"), focus+WArray.at<cv::Vec3d>(j,i)[2]/1000, selArraySpacing->val, WArray.at<cv::Vec3d>(j,i)[0], WArray.at<cv::Vec3d>(j,i)[1], selectedavg, focusRad);

            if(i!=WArray.cols-1) pgMGUI->move(-selArraySpacing->val/1000,0,0,0);
        }
        if(j!=WArray.rows-1) pgMGUI->move(2*xOfs,-selArraySpacing->val/1000,0,0);
    }
    pgMGUI->move(xOfs,yOfs,0,0);
    pgMGUI->moveZF(focus);
    while(!go.pXPS->isQueueEmpty()) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);  //wait for motion to complete

//    if(selArrayScanType->index==0){ //single(or multiple averaged) mesurement
//        redoB:  pgSGUI->doOneRound(-1);
//        while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
//        const pgScanGUI::scanRes* res=scanRes->get();
//        if(cv::countNonZero(cv::Mat(res->mask, cv::Rect(res->depth.cols/2-xSize/2, res->depth.rows/2-ySize/2, xSize, ySize)))>xSize*ySize*0.0001){std::cerr<<"To much non zero mask in ROI (>0.1%); redoing measurement.\n";goto redoB;}

//        while(res->avgNum!=(int)selArrayOneScanN->val){
//            pgSGUI->doOneRound(1);
//            while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
//            res=scanRes->get();
//        }
//        pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY, xSize, ySize), util::toString(folder,"/after"));
//    }

    //TODO separate if single/avg

    btnWriteCalibFocus->setChecked(false);
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
    //double model=i0+a*exp(-(  pow(x-x0,2)/2*(cos(an)/pow(wx,2)+sin(an)/pow(wy,2)) +  pow(y-y0,2)/2*(cos(an)/pow(wy,2)+sin(an)/pow(wx,2)) ));  //seams to be wrong
    double A=pow(cos(an),2)/2/pow(wx,2)+pow(sin(an),2)/2/pow(wy,2);
    double B=sin(2*an)/2/pow(wx,2)-sin(2*an)/2/pow(wy,2);
    double C=pow(sin(an),2)/2/pow(wx,2)+pow(cos(an),2)/2/pow(wy,2);
    double model=i0+a*exp(-A*pow(x-x0,2)-B*(x-x0)*(y-y0)-C*pow(y-y0,2));
    //if(i0<0 || x0<0 || y0<0 || a<0 || a>255) return 9999;
    return model-data.second;
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
    while(!readFolders.empty()){
        curFolder=readFolders.back();
        readFolders.pop_back();
        wp=opendir(curFolder.c_str());
        bool dirHasMes[4]{false,false,false,false};
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
                    else if(strcmp(entry->d_name,"laser.dat")==0) dirHasMes[3]=true;
                }
            }
        }
        closedir(wp);
        if(dirHasMes[0]&dirHasMes[1]&dirHasMes[2]&dirHasMes[3]) measFolders.push_back(curFolder);
    }

    std::ofstream wfile(saveName);
    for(auto& fldr:measFolders){
        double FZdif;
        double Frad;
        std::ifstream ifs(util::toString(fldr,"/settings.txt"));
        ifs>>FZdif;
        for(int i=0;i!=5;i++)ifs>>Frad;
        ifs.close();

        //std::cerr<<fldr<<"\n";
        pgScanGUI::scanRes scanBefore, scanAfter;
        if(!pgScanGUI::loadScan(&scanBefore, util::toString(fldr,"/before.pfm"))) continue;
        if(!pgScanGUI::loadScan(&scanAfter, util::toString(fldr,"/after.pfm"))) continue;
        pgScanGUI::scanRes scanDif=pgScanGUI::difScans(&scanBefore, &scanAfter);
        //pgScanGUI::saveScan(&scanDif, util::toString(fldr,"/scandif.pfm"));

//        cv::Mat rescaleDepth, rescaleMask;
//        cv::resize(scanDif.depth,rescaleDepth,cv::Size(),0.2,0.2);            //dont forget to rescale the measured widths
//        cv::resize(scanDif.mask,rescaleMask,cv::Size(),0.2,0.2);

        std::vector<std::pair<input_vector, double>> data;
//        for(int i=0;i!=scanDif.depth.cols;i++) for(int j=0;j!=scanDif.depth.rows;j++)
//            if(scanDif.mask.at<uchar>(j,i)==0) data.push_back(std::make_pair(input_vector{(double)i,(double)j},scanDif.depth.at<float>(j,i)));
        for(int i=0;i!=scanDif.depth.cols;i++) for(int j=0;j!=scanDif.depth.rows;j++)
            if(scanDif.mask.at<uchar>(j,i)==0) data.push_back(std::make_pair(input_vector{(double)i,(double)j},scanDif.depth.at<float>(j,i)));
        //std::cout<<"size= "<<data.size()<<"\n";

        parameter_vector res{(double)scanDif.depth.cols/2,(double)scanDif.depth.rows/2,scanDif.max-scanDif.min,(double)scanDif.depth.rows, (double)scanDif.depth.rows, 0.01, scanDif.min};
        //dlib::solve_least_squares_lm(dlib::objective_delta_stop_strategy(1e-7).be_verbose(), gaussResidual, derivative(gaussResidual), data, res);

        dlib::solve_least_squares_lm(dlib::objective_delta_stop_strategy(1e-7,100), gaussResidual, derivative(gaussResidual), data, res);
        //std::cout << "inferred parameters: "<< dlib::trans(res) << "\n";
        while(res(5)>M_PI) res(5)-=M_PI;
        while(res(5)<0) res(5)+=M_PI;
        if(res(5)>=M_PI/2){
           res(5)-= M_PI/2;
           double tmp=res(3);
           res(3)=res(4);
           res(4)=tmp;
        }

        wfile<<Frad<<" "<<FZdif<<" "<<res(2)<<" "<<abs(res(3))<<" "<<abs(res(4))<<" "<<res(5)<<" "<<sqrt(res(3)*res(3)+res(4)*res(4))<<" "<<res(0)<<" "<<res(1)<<" "<<sqrt(res(0)*res(0)+res(1)*res(1))<<"\n";
        std::cerr<<Frad<<" "<<FZdif<<" "<<res(2)<<" "<<abs(res(3))<<" "<<abs(res(4))<<" "<<res(5)<<" "<<sqrt(res(3)*res(3)+res(4)*res(4))<<" "<<res(0)<<" "<<res(1)<<" "<<sqrt(res(0)*res(0)+res(1)*res(1))<<"\n";

//        double min,max;
//        cv::Point  ignore;
//        cv::minMaxLoc(scanDif.depth, &min, &max, &ignore, &ignore);
//        std::cout<<Frad<<" "<<FZdif<<" "<<max-min<<"\n";

    }
    wfile.close();
}

