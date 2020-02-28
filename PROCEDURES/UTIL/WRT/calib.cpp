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
    btnSelectSaveFolder=new QPushButton("Select save folder");
    connect(btnSelectSaveFolder, SIGNAL(released()), this, SLOT(onSelSaveF()));
    alayout->addWidget(new twid(btnSelectSaveFolder));

    btnWriteCalib=new HQPushButton("Calibrate Write Focus");
    btnWriteCalib->setEnabled(false);
    connect(btnWriteCalib, SIGNAL(released()), this, SLOT(onWCF()));
    connect(btnWriteCalib, SIGNAL(changed(bool)), this, SLOT(onChangeDrawWriteAreaOn(bool)));
    btnWriteCalib->setCheckable(true);
    QLabel* tmpl=new QLabel("First refocus microscope and laser!"); tmpl->setEnabled(false);
    alayout->addWidget(new twid(btnWriteCalib, tmpl));

    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);

    calibMethod=new twds_selector("pgCalib_calibMethod",0,"Select method:", "Find Nearest", false, false, false);
    calibMethodFindNearest=new vtwid;
        calibMethodFindNearest->addWidget(new QLabel("Selection procedure: Blur+Laplacian+Thresh+Dilate+Exclusion+Border"));
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
        selArrayType=new smp_selector("pgCalib_selArrayType", "Variable Parameters (X-Y): ", 0, {"Intensity(XY)","Duration(XY)","Focus(XY)","Intensity(X)-Duration(Y)","Intensity(X)-Focus(Y)","Duration(X)-Focus(Y)", "Intensity(X)","Duration(X)","Focus(X)"});
        calibMethodArray->addWidget(selArrayType);
        transposeMat=new checkbox_save(false,"pgCalib_transposeMat","Transpose matrix.");
        calibMethodArray->addWidget(transposeMat);
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
        saveMats=new checkbox_save(true,"pgCalib_saveMats","Extra Save Mats Containing I,D,F for Convenience.");
        calibMethodArray->addWidget(saveMats);
        savePic=new checkbox_save(true,"pgCalib_savePic","Also save direct pictures of measurements.");
        calibMethodArray->addWidget(savePic);
//        doRefocusUScope=new checkbox_save(true,"pgCalib_doRefocusUScope","Automatically refocus microscope (needed for good ref. beam refocus).");
//        calibMethodArray->addWidget(doRefocusUScope);
//        doRedFocusCenter=new checkbox_save(true,"pgCalib_doRedFocusCenter","Automatically recenter and refocus reference beam.");
//        calibMethodArray->addWidget(doRedFocusCenter);
    calibMethod->addWidget(calibMethodArray, "Array");
    calibMethodAutoArray=new vtwid;
        calibMethodAutoArray->addWidget(new QLabel("Selection procedure: Exclusion+DilateSQ+Border"));
        selAArrayDoNMes=new val_selector(100, "pgCalib_selAArrayDoNMes", "In total do this many measurements (will decrement): ", 0, 100000000, 0);
        calibMethodAutoArray->addWidget(selAArrayDoNMes);
        selAArrayXsize=new val_selector(10, "pgCalib_selAArrayXsize", "Array Size X", 1, 1000, 0);
        calibMethodAutoArray->addWidget(selAArrayXsize);
        selAArrayYsize=new val_selector(10, "pgCalib_selAArrayYsize", "Array Size Y", 1, 1000, 0);
        calibMethodAutoArray->addWidget(selAArrayYsize);
        selAArraySpacing=new val_selector(5, "pgCalib_selAArraySpacing", "Array Spacing", 0.001, 100, 3, 0, {"um"});
        calibMethodAutoArray->addWidget(selAArraySpacing);
        selAArrayAvgN=new val_selector(10, "pgCalib_selAArrayAvgN", "Average This Many Scans: ", 1, 1000, 0);
        calibMethodAutoArray->addWidget(selAArrayAvgN);
        selAArrayIntA=new val_selector(1000, "pgCalib_selAArrayIntA", "Intensity Value A", 1, 8192, 0);
        calibMethodAutoArray->addWidget(selAArrayIntA);
        selAArrayIntB=new val_selector(1000, "pgCalib_selAArrayIntB", "Intensity Value B", 1, 8192, 0);
        calibMethodAutoArray->addWidget(selAArrayIntB);
        selAArrayDurA=new val_selector(1, "pgCalib_selAArrayDurA", "Duration Value A", 0.001, 1000, 3, 0, {"ms"});
        calibMethodAutoArray->addWidget(selAArrayDurA);
        selAArrayDurB=new val_selector(1, "pgCalib_selAArrayDurB", "Duration Value B", 0.001, 1000, 3, 0, {"ms"});
        calibMethodAutoArray->addWidget(selAArrayDurB);
        selAArrayNGenCand=new val_selector(100, "pgCalib_selAArrayNGenCand", "Consider this many random points as next location: ", 1, 10000, 0);
        calibMethodAutoArray->addWidget(selAArrayNGenCand);
        selAArraySetMaskToThisHeight=new val_selector(200, "pgCalib_selAArraySetMaskToThisHeight", "Bad pixel height (for planning next move): ", 0, 10000, 0, 0, {"nm"});
        selAArraySetMaskToThisHeight->setToolTip("This should be a few times higher than the highest structure you expect to be written.\nIt is used only to plan the next move (the larger it is, the more likely the program will avoid areas with bad pixels).\nIn the end individual scans with"
                                                 " bad pixels are discarded anyway.");
        calibMethodAutoArray->addWidget(selAArraySetMaskToThisHeight);
    calibMethod->addWidget(calibMethodAutoArray, "Auto Array");
    calibMethod->doneAddingWidgets();
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
    imgAux::getNearestFreePointToCenter(&mask, pgBeAn->writeBeamCenterOfsX, pgBeAn->writeBeamCenterOfsY, ptX, ptY, radRandSpread);
    if(ptX==-1){
        std::cerr<<"No free nearby!\n";
        delete scanRes;
        return true;
    }

    double dXumm, dYumm, dXmm, dYmm;
    dXumm=(ptX-res->depth.cols/2)*res->XYnmppx/1000000;
    dYumm=(ptY-res->depth.rows/2)*res->XYnmppx/1000000;
    dXmm=dXumm*cos(pgMGUI->getAngCamToXMot())+dYumm*sin(pgMGUI->getAngCamToXMot()+pgMGUI->getYMotToXMot());
    dYmm=dXumm*sin(pgMGUI->getAngCamToXMot())+dYumm*cos(pgMGUI->getAngCamToXMot()+pgMGUI->getYMotToXMot());

    pgMGUI->move(-dXmm,-dYmm,0,0);
    delete scanRes;
    return false;
}
void pgCalib::onSelSaveF(){
    std::string temp=QFileDialog::getExistingDirectory(this, "Select Folder for Saving Calibration Data").toStdString();
    if(temp.empty()) return;
    saveFolderName=temp;
    btnWriteCalib->setEnabled(true);
}

void pgCalib::onWCF(){
    if(!btnWriteCalib->isChecked()) return;
    if(!go.pRPTY->connected) {QMessageBox::critical(this, "Error", "Error: Red Pitaya not Connected"); return;}
    if(!go.pXPS->connected) {QMessageBox::critical(this, "Error", "Error: XPS not Connected"); return;}
    switch(calibMethod->index){
    case 0: WCFFindNearest();
            break;
    case 1: WCFArray();
            break;
    case 2: WCFAArray();
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

void pgCalib::saveConf(std::string filename, double focus, double exclusionOrSpacing, int intensity, double duration, uchar averaging){
    std::ofstream setFile(filename);            //this file contains some settings:
    setFile <<focus<<"\n"                       // line0:   the focus in mm (stage)
            <<exclusionOrSpacing<<"\n"          // line1:   Exclusion radius / ROI radius in um
            <<intensity<<"\n"                   // line2:   Pulse Intensity
            <<duration<<"\n"                    // line3:   Pulse Duration in ms
            <<(int)averaging<<"\n"              // line4:   Averaging Used in Pulse Raw Data (ie. the time difference between datapoints will be selectedavg*8e-9 sec)
            <<-1<<"\n";                         // line5:   Empty for now (backward compatibility)
    setFile.close();
}

void pgCalib::saveConfMain(std::string filename, double focus, double extraFocusOffset){
    std::ofstream setFile(filename);            //this file contains some settings:
    setFile <<focus<<"\n"                       // line0:   the focus in mm (stage)
            <<extraFocusOffset<<"\n";           // line1:   Extra focus offset - the difference between the calibration(red) beam focus and the writing(green) beam focus
    setFile.close();
}

void pgCalib::WCFFindNearest(){
    if(goToNearestFree(selWriteCalibFocusRadDil->val,selWriteCalibFocusRadSpr->val)) {QMessageBox::critical(this, "Error", "No free nearby, stopping."); btnWriteCalib->setChecked(false); return;}
    varShareClient<pgScanGUI::scanRes>* scanRes=pgSGUI->result.getClient();
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

    std::mt19937 rnd(std::random_device{}());
    std::uniform_real_distribution<>dist(-selWriteCalibFocusRange->val, selWriteCalibFocusRange->val);
    double wrFocusOffs=dist(rnd);
    wrFocusOffs=round(wrFocusOffs*1000)/1000;   //we round it up to 1 nm precision
    std::cerr<<"wrFocusOffs is: "<<wrFocusOffs<<" um\n";
    double focus=pgMGUI->FZdifference;
    std::cerr<<"Focus is: "<<focus<<" mm\n";
    pgMGUI->moveZF(focus+wrFocusOffs/1000);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait a bit for movement to complete and stabilize
    std::cerr<<"new focus is: "<<focus+wrFocusOffs/1000<<" mm\n";

    redoA:  pgSGUI->doOneRound();
    while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
    const pgScanGUI::scanRes* res=scanRes->get();
    int roiD=2*selWriteCalibFocusRadDil->val*1000/res->XYnmppx;
    if(cv::countNonZero(cv::Mat(res->mask, cv::Rect(res->depth.cols/2-roiD/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-roiD/2+pgBeAn->writeBeamCenterOfsY, roiD, roiD)))>roiD*roiD*(4-M_PI)/4){std::cerr<<"To much non zero mask in ROI; redoing measurement.\n";goto redoA;}      //this is (square-circle)/4
    pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-roiD/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-roiD/2+pgBeAn->writeBeamCenterOfsY, roiD, roiD), util::toString(folder,"/before"));

    uchar selectedavg;
    writePulse(selWriteCalibFocusPulseIntensity->val, selWriteCalibFocusPulseDuration->val, util::toString(folder,"/laser.dat"), &selectedavg);

    pgMGUI->moveZF(focus);

    saveConf(util::toString(folder,"/settings.txt"), focus+wrFocusOffs/1000, selWriteCalibFocusRadDil->val, selWriteCalibFocusPulseIntensity->val, selWriteCalibFocusPulseDuration->val, selectedavg);

    redoB:  pgSGUI->doOneRound();
    while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
    res=scanRes->get();
    if(cv::countNonZero(cv::Mat(res->mask, cv::Rect(res->depth.cols/2-roiD/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-roiD/2+pgBeAn->writeBeamCenterOfsY, roiD, roiD)))>roiD*roiD*(4-M_PI)/4){std::cerr<<"To much non zero mask in ROI; redoing measurement.\n";goto redoB;}
    pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-roiD/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-roiD/2+pgBeAn->writeBeamCenterOfsY, roiD, roiD), util::toString(folder,"/after"));

    if(btnWriteCalib->isChecked()){
        measCounter++;
        if(measCounter<(int)selWriteCalibFocusDoNMeas->val) {delete scanRes; return WCFFindNearest();}
        else {btnWriteCalib->setChecked(false); measCounter=0;}
    } else measCounter=0;
    delete scanRes;
}

void pgCalib::WCFArray(){
    varShareClient<pgScanGUI::scanRes>* scanRes=pgSGUI->result.getClient();
    cv::Mat WArray(selArrayYsize->val,selArrayXsize->val,CV_64FC4,cv::Scalar(0,0,0,0));   //Intensity, Duration(ms), Focus(um)
    int arraySizeInt{1}, arraySizeDur{1}, arraySizeFoc{1};
    int index=selArrayType->index;
    switch(index){
    case 0: arraySizeInt=selArrayYsize->val*selArrayXsize->val;     //Intensity (XY)
            break;
    case 1: arraySizeDur=selArrayYsize->val*selArrayXsize->val;     //Duration (XY)
            break;
    case 2: arraySizeFoc=selArrayYsize->val*selArrayXsize->val;     //Focus (XY)
            break;
    case 3: arraySizeInt=selArrayXsize->val;                        //Intensity-Duration (X,Y)
            arraySizeDur=selArrayYsize->val;
            break;
    case 4: arraySizeInt=selArrayXsize->val;                        //Intensity-Focus (X,Y)
            arraySizeFoc=selArrayYsize->val;
            break;
    case 5: arraySizeDur=selArrayXsize->val;                        //Duration-Focus (X,Y)
            arraySizeFoc=selArrayYsize->val;
            break;
    case 6: arraySizeInt=selArrayYsize->val;                        //Intensity (X)
            break;
    case 7: arraySizeDur=selArrayYsize->val;                        //Duration (X)
            break;
    case 8: arraySizeFoc=selArrayYsize->val;                        //Focus (X)
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
        else if(index==3 || index==4 || index==6) WArray.at<cv::Vec3d>(j,i)[0]=arrayInt[i];
        else WArray.at<cv::Vec3d>(j,i)[0]=arrayInt[0];
        if(index==1) WArray.at<cv::Vec3d>(j,i)[1]=arrayDur[i+j*WArray.cols];
        else if(index==3)  WArray.at<cv::Vec3d>(j,i)[1]=arrayDur[j];
        else if(index==5 || index==7) WArray.at<cv::Vec3d>(j,i)[1]=arrayDur[i];
        else WArray.at<cv::Vec3d>(j,i)[1]=arrayDur[0];
        if(index==2) WArray.at<cv::Vec3d>(j,i)[2]=arrayFoc[i+j*WArray.cols];
        else if(index==8)  WArray.at<cv::Vec3d>(j,i)[2]=arrayFoc[i];
        else if(index==4 || index==5)  WArray.at<cv::Vec3d>(j,i)[2]=arrayFoc[j];
        else WArray.at<cv::Vec3d>(j,i)[2]=arrayFoc[0];
    }
    if(transposeMat->val){
        cv::Mat temp=WArray.clone();
        WArray=cv::Mat(selArrayXsize->val,selArrayYsize->val,CV_64FC4,cv::Scalar(0,0,0,0));
        for(int j=0;j!=WArray.rows; j++) for(int i=0;i!=WArray.cols; i++) for(int k=0;k!=4; k++)        // I do this manually because for some reason cv::transpose and mat.t() and even if I separate channels it does not work
            WArray.at<cv::Vec3d>(j,i)[k]=temp.at<cv::Vec3d>(i,j)[k];
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

//    if(doRefocusUScope->val)
//        pgFGUI->refocus();
//    if(doRedFocusCenter->val)
//        pgBeAn->correctWritingBeamFocus();

    double focus=pgMGUI->FZdifference;

    saveConfMain(util::toString(folder,"/main-settings.txt"), focus, *pgBeAn->extraFocusOffsetVal);

    double xOfs=((WArray.cols-1)*selArraySpacing->val)/2000;         //in mm
    double yOfs=((WArray.rows-1)*selArraySpacing->val)/2000;
    double xSize=WArray.cols*selArraySpacing->val*1000/pgMGUI->getNmPPx();
    double ySize=WArray.rows*selArraySpacing->val*1000/pgMGUI->getNmPPx();
    const pgScanGUI::scanRes* res;
    if(selArrayScanType->index==0){ //single(or multiple averaged) mesurement
            //first we check if ROI is fine
        if(go.pGCAM->iuScope->camCols/2-xSize/2+pgBeAn->writeBeamCenterOfsX<0                                 || go.pGCAM->iuScope->camRows/2-ySize/2+pgBeAn->writeBeamCenterOfsY<0 ||          //we do not use res here because it may not be initialized, if the user has made no scans prior to this
           go.pGCAM->iuScope->camCols/2-xSize/2+pgBeAn->writeBeamCenterOfsX+xSize>=go.pGCAM->iuScope->camCols || go.pGCAM->iuScope->camRows/2-ySize/2+pgBeAn->writeBeamCenterOfsY+ySize>=go.pGCAM->iuScope->camRows){
            QMessageBox::critical(this, "Error", "The calibration ROI does not fit the viewport, aborting calibration.\n");
            btnWriteCalib->setChecked(false);
            delete scanRes;
            return;
        }

        pgSGUI->doNRounds((int)selArrayOneScanN->val, discardMaskRoiThresh, maxRedoScanTries, cv::Rect(go.pGCAM->iuScope->camCols/2-xSize/2+pgBeAn->writeBeamCenterOfsX, go.pGCAM->iuScope->camRows/2-ySize/2+pgBeAn->writeBeamCenterOfsY, xSize, ySize));

        res=scanRes->get();
        pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY, xSize, ySize), util::toString(folder,"/before"));

        for(int j=0;j!=WArray.rows; j++) for(int i=0;i!=WArray.cols; i++){   // separate them into individual scans
            cv::utils::fs::createDirectory(util::toString(folder,"/",i+j*WArray.cols));
            pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX+i*selArraySpacing->val*1000/pgMGUI->getNmPPx(), res->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY+j*selArraySpacing->val*1000/pgMGUI->getNmPPx(),
                                                  selArraySpacing->val*1000/pgMGUI->getNmPPx(), selArraySpacing->val*1000/pgMGUI->getNmPPx()), util::toString(folder,"/",i+j*WArray.cols,"/before"));
        }
    }

    pgMGUI->move(xOfs,yOfs,0,0);

    for(int j=0;j!=WArray.rows; j++){
        for(int i=0;i!=WArray.cols; i++){
            if(!btnWriteCalib->isChecked()){   //abort
                std::cerr<<"Aborting calibration.\n";
                pgMGUI->moveZF(focus);
                delete scanRes;
                return;
            }

            pgMGUI->moveZF(focus+WArray.at<cv::Vec3d>(j,i)[2]/1000);
            while(!go.pXPS->isQueueEmpty()) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);  //wait for motion to complete

            uchar selectedavg;
            cv::utils::fs::createDirectory(util::toString(folder,"/",i+j*WArray.cols));

            if(selArrayScanType->index==1){ //multiple mesurement
                int tryA{0};
                redoA:  tryA++; pgSGUI->doOneRound(-1);
                while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
                const pgScanGUI::scanRes* res=scanRes->get();
                int roiD=selArraySpacing->val*1000/res->XYnmppx;
                if(cv::countNonZero(cv::Mat(res->mask, cv::Rect(res->depth.cols/2-roiD/2, res->depth.rows/2-roiD/2, roiD, roiD)))>roiD*roiD*discardMaskRoiThresh && tryA<maxRedoScanTries){std::cerr<<"To much non zero mask in ROI; redoing measurement.\n";goto redoA;}      //this is (square-circle)/4
                pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-roiD/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-roiD/2+pgBeAn->writeBeamCenterOfsY, roiD, roiD), util::toString(folder,"/",i+j*WArray.cols,"/before"));
            }

            writePulse(WArray.at<cv::Vec3d>(j,i)[0], WArray.at<cv::Vec3d>(j,i)[1]*1000, util::toString(folder,"/",i+j*WArray.cols,"/laser.dat"), &selectedavg);

            if(selArrayScanType->index==1){ //multiple mesurement
                int tryB{0};
                redoB:  tryB++; pgSGUI->doOneRound(-1);
                while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
                const pgScanGUI::scanRes* res=scanRes->get();
                int roiD=selArraySpacing->val*1000/res->XYnmppx;
                if(cv::countNonZero(cv::Mat(res->mask, cv::Rect(res->depth.cols/2-roiD/2, res->depth.rows/2-roiD/2, roiD, roiD)))>roiD*roiD*discardMaskRoiThresh && tryB<maxRedoScanTries){std::cerr<<"To much non zero mask in ROI; redoing measurement.\n";goto redoB;}      //this is (square-circle)/4
                pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-roiD/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-roiD/2+pgBeAn->writeBeamCenterOfsY, roiD, roiD), util::toString(folder,"/",i+j*WArray.cols,"/after"));

                if(savePic->val){
                    FQ* fq=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
                    fq->setUserFps(999,1);
                    while(fq->getUserMat()==nullptr)QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
                    imwrite(util::toString(folder,"/",i+j*WArray.cols,"/pic.png"), cv::Mat(*fq->getUserMat(),cv::Rect(res->depth.cols/2-roiD/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-roiD/2+pgBeAn->writeBeamCenterOfsY, roiD, roiD)),{cv::IMWRITE_PNG_COMPRESSION,9});
                    go.pGCAM->iuScope->FQsPCcam.deleteFQ(fq);
                }
            }

            saveConf(util::toString(folder,"/",i+j*WArray.cols,"/settings.txt"), focus+WArray.at<cv::Vec3d>(j,i)[2]/1000, selArraySpacing->val, WArray.at<cv::Vec3d>(j,i)[0], WArray.at<cv::Vec3d>(j,i)[1], selectedavg);

            std::cerr<<"\nDone "<<1+i+j*WArray.cols<<"/"<<WArray.rows*WArray.cols<<"\n\n";
            if(i!=WArray.cols-1) pgMGUI->move(-selArraySpacing->val/1000,0,0,0);
        }
        if(j!=WArray.rows-1) pgMGUI->move(2*xOfs,-selArraySpacing->val/1000,0,0);
    }
    pgMGUI->move(xOfs,yOfs,0,0);
    pgMGUI->moveZF(focus);
    while(!go.pXPS->isQueueEmpty()) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);  //wait for motion to complete

    if(selArrayScanType->index==0){ //single(or multiple averaged) mesurement
        pgSGUI->doNRounds((int)selArrayOneScanN->val, discardMaskRoiThresh, maxRedoScanTries, cv::Rect(res->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY, xSize, ySize));
        res=scanRes->get();
        pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX, res->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY, xSize, ySize), util::toString(folder,"/after"));

        for(int j=0;j!=WArray.rows; j++) for(int i=0;i!=WArray.cols; i++)   // separate them into individual scans
            pgScanGUI::saveScan(res, cv::Rect(res->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX+i*selArraySpacing->val*1000/pgMGUI->getNmPPx(), res->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY+j*selArraySpacing->val*1000/pgMGUI->getNmPPx(),
                                                  selArraySpacing->val*1000/pgMGUI->getNmPPx(), selArraySpacing->val*1000/pgMGUI->getNmPPx()), util::toString(folder,"/",i+j*WArray.cols,"/after"));
        if(savePic->val){
            FQ* fq=go.pGCAM->iuScope->FQsPCcam.getNewFQ();
            fq->setUserFps(999,1);
            while(fq->getUserMat()==nullptr)QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
            for(int j=0;j!=WArray.rows; j++) for(int i=0;i!=WArray.cols; i++)
                imwrite(util::toString(folder,"/",i+j*WArray.cols,"/pic.png"), cv::Mat(*fq->getUserMat(),
                                       cv::Rect(res->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX+i*selArraySpacing->val*1000/pgMGUI->getNmPPx(), res->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY+j*selArraySpacing->val*1000/pgMGUI->getNmPPx(),
                                       selArraySpacing->val*1000/pgMGUI->getNmPPx(), selArraySpacing->val*1000/pgMGUI->getNmPPx())),{cv::IMWRITE_PNG_COMPRESSION,9});
            go.pGCAM->iuScope->FQsPCcam.deleteFQ(fq);
        }
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
    pgSGUI->doOneRound(-1);
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
    int toCheck=selAArrayNGenCand->val<validPts.size()?selAArrayNGenCand->val:validPts.size();
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
    pgMGUI->move((resPre->depth.cols/2+pgBeAn->writeBeamCenterOfsX-validPts[validPtsW[chosen].it].x)*resPre->XYnmppx/1000000,(resPre->depth.rows/2+pgBeAn->writeBeamCenterOfsY-validPts[validPtsW[chosen].it].y)*resPre->XYnmppx/1000000,0,0);
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
    pgSGUI->doNRounds((int)selAArrayAvgN->val, discardMaskRoiThresh, maxRedoScanTries, cv::Rect(resPre->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX, resPre->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY, xSize, ySize));
    resPre=scanResPre->get();

    double xOfs=((WArray.cols-1)*selAArraySpacing->val)/2000;         //in mm
    double yOfs=((WArray.rows-1)*selAArraySpacing->val)/2000;
    pgMGUI->move(xOfs,yOfs,0,0);
    for(int j=0;j!=WArray.rows; j++){
        for(int i=0;i!=WArray.cols; i++){
            while(!go.pXPS->isQueueEmpty()) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
            writePulse(WArray.at<cv::Vec2d>(j,i)[0], WArray.at<cv::Vec2d>(j,i)[1]*1000);
            if(i!=WArray.cols-1) pgMGUI->move(-selAArraySpacing->val/1000,0,0,0);
        }
        if(j!=WArray.rows-1) pgMGUI->move(2*xOfs,-selAArraySpacing->val/1000,0,0);
    }
    pgMGUI->move(xOfs,yOfs,0,0);
    while(!go.pXPS->isQueueEmpty()) QCoreApplication::processEvents(QEventLoop::AllEvents, 1);

        //do post writing measurements
    pgSGUI->doNRounds((int)selAArrayAvgN->val, discardMaskRoiThresh, maxRedoScanTries, cv::Rect(resPre->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX, resPre->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY, xSize, ySize));
    const pgScanGUI::scanRes* resPost=scanResPost->get();


    std::ofstream wfile(util::toString(folder,"/params.txt"));      //has the following format: <int> <dur(ms)>, is plain text, separated by spaces
    int k=0;
    for(int j=0;j!=WArray.rows; j++) for(int i=0;i!=WArray.cols; i++){   // separate them into individual scans
        if(cv::countNonZero(cv::Mat(resPre->mask,cv::Rect(resPre->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX+i*selAArraySpacing->val*1000/resPre->XYnmppx, resPre->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY+j*selAArraySpacing->val*1000/resPre->XYnmppx,
                                                          selAArraySpacing->val*1000/resPre->XYnmppx, selAArraySpacing->val*1000/resPre->XYnmppx))) ||
           cv::countNonZero(cv::Mat(resPost->mask,cv::Rect(resPost->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX+i*selAArraySpacing->val*1000/resPost->XYnmppx, resPost->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY+j*selAArraySpacing->val*1000/resPost->XYnmppx,
                                                           selAArraySpacing->val*1000/resPost->XYnmppx, selAArraySpacing->val*1000/resPost->XYnmppx)))) continue;       //there were bad pixels so we skip
        pgScanGUI::saveScan(resPre, cv::Rect(resPre->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX+i*selAArraySpacing->val*1000/resPre->XYnmppx, resPre->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY+j*selAArraySpacing->val*1000/resPre->XYnmppx,
                                              selAArraySpacing->val*1000/resPre->XYnmppx, selAArraySpacing->val*1000/resPre->XYnmppx), util::toString(folder,"/",k,"-pre"),false);
        pgScanGUI::saveScan(resPost, cv::Rect(resPost->depth.cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX+i*selAArraySpacing->val*1000/resPost->XYnmppx, resPost->depth.rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY+j*selAArraySpacing->val*1000/resPost->XYnmppx,
                                              selAArraySpacing->val*1000/resPost->XYnmppx, selAArraySpacing->val*1000/resPost->XYnmppx), util::toString(folder,"/",k,"-post"),false);
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
    //double model=i0+a*exp(-(  pow(x-x0,2)/2*(cos(an)/pow(wx,2)+sin(an)/pow(wy,2)) +  pow(y-y0,2)/2*(cos(an)/pow(wy,2)+sin(an)/pow(wx,2)) ));  //seams to be wrong
    double A=pow(cos(an),2)/2/pow(wx,2)+pow(sin(an),2)/2/pow(wy,2);
    double B=sin(2*an)/2/pow(wx,2)-sin(2*an)/2/pow(wy,2);
    double C=pow(sin(an),2)/2/pow(wx,2)+pow(cos(an),2)/2/pow(wy,2);
    double model=i0+a*exp(-A*pow(x-x0,2)-B*(x-x0)*(y-y0)-C*pow(y-y0,2));
    //if(i0<0 || x0<0 || y0<0 || a<0 || a>255) return 9999;
    return model-data.second;
}
bool folderSort(std::string i,std::string j){
    size_t posi=i.find_last_of("/");
    size_t posj=i.find_last_of("/");
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
    bool dirHasMes[5]{false,false,false,false,false};
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
                    else if(strcmp(entry->d_name,"laser.dat")==0) dirHasMes[3]=true;
                    else if(strcmp(entry->d_name,"pic.png")==0) dirHasMes[4]=true;      //nonessential for backward compatibility
                }
            }
        }
        closedir(wp);
        if(dirHasMes[0]&dirHasMes[1]&dirHasMes[2]&dirHasMes[3]) measFolders.push_back(curFolder);
    }

    //sort folders by number
    std::sort(measFolders.begin(), measFolders.end(), folderSort);

    std::ofstream wfile(saveName);
    int n=0;
    wfile<<"# <1: not used> <2: focus distance(mm)> <3: peak height(nm)> <4: X width (1/e^2)(um)> <5: Y width (1/e^2)(um)> <6: ellipse angle(rad)>\n";
    wfile<<"# <7: XY width (1/e^2)(um)> <8: X offset(um)> <9: Y offset(um)> <10: XY offset(um)> <11: Intensity (a.u.)> <12: duration(ms)> <13: MeanAbs.ReflectivityDeriv.(a.u.)> <14: Max absolute 1st der. (px/um)> <15: Min Laplacian (px/um^2)> <16: Max Laplacian (px/um^2)>\n";
    for(auto& fldr:measFolders){ n++;
        double FZdif;
        double none;        //for backward compat., not used for now
        double intensity;
        double duration;
        std::ifstream ifs(util::toString(fldr,"/settings.txt"));
        ifs>>FZdif;
        for(int i=0;i!=2;i++) ifs>>intensity;
        ifs>>duration;
        for(int i=0;i!=2;i++)ifs>>none;
        ifs.close();

        //std::cerr<<fldr<<"\n";
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
//        for(int i=0;i!=scanDif.depth.cols;i++) for(int j=0;j!=scanDif.depth.rows;j++)
//            if(scanDif.mask.at<uchar>(j,i)==0) data.push_back(std::make_pair(input_vector{(double)i,(double)j},scanDif.depth.at<float>(j,i)));
        for(int i=0;i!=scanDif.depth.cols;i++) for(int j=0;j!=scanDif.depth.rows;j++)
            if(scanDif.mask.at<uchar>(j,i)==0) data.push_back(std::make_pair(input_vector{(double)i,(double)j},scanDif.depth.at<float>(j,i)));
        //std::cout<<"size= "<<data.size()<<"\n";

        parameter_vector res{(double)scanDif.depth.cols/2,(double)scanDif.depth.rows/2,scanDif.max-scanDif.min,(double)scanDif.depth.rows, (double)scanDif.depth.rows, 0.01, scanDif.min};

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

        double intReflDeriv{-1};
        if(dirHasMes[4]){
            cv::Mat refl=imread(util::toString(fldr,"/pic.png"), cv::IMREAD_GRAYSCALE);
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
        cv::Laplacian(com, derv, CV_32F);
        double minDepthLaplacian, maxDepthLaplacian;
        cv::minMaxIdx(derv, &minDepthLaplacian, &maxDepthLaplacian);

        if(res(0)<0 || res(0)>scanDif.depth.cols || res(1)<0 || res(1)>scanDif.depth.rows || res(2)<=0){   //center of the fit is out of frame or other things that indicate fit faliure
            wfile<<none<<" "<<FZdif<<" 0 nan nan nan nan 0 0 0 "<<intensity<<" "<<duration<<" "<<intReflDeriv<<" "<<maxDepthDer<<" "<<minDepthLaplacian<<" "<<maxDepthLaplacian<<"\n";
            std::cerr<<"("<<n<<"/"<<measFolders.size()<<") "
                 <<none<<" "<<FZdif<<" 0 nan nan nan nan 0 0 0 "<<intensity<<" "<<duration<<" "<<intReflDeriv<<" "<<maxDepthDer<<" "<<minDepthLaplacian<<" "<<maxDepthLaplacian<<"\n";
        }else{
            wfile<<none<<" "<<FZdif<<" "<<res(2)<<" "<<2*abs(res(3))<<" "<<2*abs(res(4))<<" "<<res(5)<<" "<<2*(abs(res(3))+abs(res(4)))/2<<" "<<res(0)<<" "<<res(1)<<" "<<sqrt(res(0)*res(0)+res(1)*res(1))<<" "<<intensity<<" "<<duration<<" "<<intReflDeriv<<" "<<maxDepthDer<<" "<<minDepthLaplacian<<" "<<maxDepthLaplacian<<"\n";
            std::cerr<<"("<<n<<"/"<<measFolders.size()<<") "
                 <<none<<" "<<FZdif<<" "<<res(2)<<" "<<2*abs(res(3))<<" "<<2*abs(res(4))<<" "<<res(5)<<" "<<2*(abs(res(3))+abs(res(4)))/2<<" "<<res(0)<<" "<<res(1)<<" "<<sqrt(res(0)*res(0)+res(1)*res(1))<<" "<<intensity<<" "<<duration<<" "<<intReflDeriv<<" "<<maxDepthDer<<" "<<minDepthLaplacian<<" "<<maxDepthLaplacian<<"\n";
        }
    }
    wfile.close();
}

void pgCalib::onChangeDrawWriteAreaOn(bool status){
    drawWriteAreaOn=status;
}
void pgCalib::drawWriteArea(cv::Mat* img){
    if(!drawWriteAreaOn) return;
    double xSize;
    double ySize;
    switch(calibMethod->index){
    case 0: xSize=selWriteCalibFocusRadDil->val*1000/pgMGUI->getNmPPx();
            ySize=selWriteCalibFocusRadDil->val*1000/pgMGUI->getNmPPx();
            break;
    case 1: xSize=selArrayXsize->val*selArraySpacing->val*1000/pgMGUI->getNmPPx();
            ySize=selArrayYsize->val*selArraySpacing->val*1000/pgMGUI->getNmPPx();
            if(transposeMat->val) std::swap(xSize,ySize);
            break;
    case 2: xSize=selAArrayXsize->val*selAArraySpacing->val*1000/pgMGUI->getNmPPx();
            ySize=selAArrayYsize->val*selAArraySpacing->val*1000/pgMGUI->getNmPPx();
            break;
    }
    double clr[2]={0,255}; int thck[2]={3,1};
    for(int i=0;i!=2;i++)
    cv::rectangle(*img,  cv::Rect(img->cols/2-xSize/2+pgBeAn->writeBeamCenterOfsX, img->rows/2-ySize/2+pgBeAn->writeBeamCenterOfsY, xSize, ySize), {clr[i]}, thck[i], cv::LINE_AA);
}
