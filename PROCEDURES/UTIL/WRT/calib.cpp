#include "calib.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include "PROCEDURES/UTIL/USC/focus.h"
#include "PROCEDURES/UTIL/USC/move.h"
#include "opencv2/core/utils/filesystem.hpp"
#include "GUI/tab_monitor.h"    //for debug purposes
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <random>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlinear.h>
#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_statistics.h>
#include <CvPlot/cvplot.h>
#include <opencv2/highgui.hpp>

pgCalib::pgCalib(pgScanGUI* pgSGUI, pgFocusGUI* pgFGUI, pgMoveGUI* pgMGUI, pgBeamAnalysis* pgBeAn, pgWrite* pgWr, overlay& ovl): pgSGUI(pgSGUI), pgFGUI(pgFGUI), pgMGUI(pgMGUI), pgBeAn(pgBeAn), pgWr(pgWr), ovl(ovl){
    btnWriteCalib=new HQPushButton("Run Write Focus Calibration");
    connect(btnWriteCalib, SIGNAL(released()), this, SLOT(onWCF()));
    connect(btnWriteCalib, SIGNAL(changed(bool)), this, SLOT(onChangeDrawWriteAreaOn(bool)));
    btnWriteCalib->setCheckable(true);
    scheduleMultiWrite=new HQPushButton("Schedule");
    connect(scheduleMultiWrite, SIGNAL(released()), this, SLOT(onSchedule()));
    connect(scheduleMultiWrite, SIGNAL(changed(bool)), this, SLOT(onChangeDrawWriteAreaOnSch(bool)));
    scheduleMultiWrite->setVisible(false);

    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);

    selArrayXsize=new val_selector(10, "Array Size X", 1, 1000, 0);
    conf["selArrayXsize"]=selArrayXsize;
    selArrayYsize=new val_selector(10, "Array Size Y", 1, 1000, 0);
    conf["selArrayYsize"]=selArrayYsize;
    slayout->addWidget(new twid{selArrayXsize,selArrayYsize});
    selArraySpacing=new val_selector(5, "Array Spacing", 0.001, 100, 3, 0, {"um"});
    conf["selArraySpacing"]=selArraySpacing;
    slayout->addWidget(selArraySpacing);
    selArrayType=new smp_selector("Variable Parameters (X-Y): ", 0, {"Duration(X,Y), no repeat","Focus(X,Y), no repeat","Duration(X)-Focus(Y)", "Duration(X,Y), repeat","Focus(X,Y), repeat"});
    connect(selArrayType, SIGNAL(changed(int)), this, SLOT(onSelArrayTypeChanged(int)));
    conf["selArrayType"]=selArrayType;
    slayout->addWidget(selArrayType);
    transposeMat=new checkbox_gs(false,"Transpose matrix.");
    conf["transposeMat"]=transposeMat;
    slayout->addWidget(transposeMat);
    slayout->addWidget(new QLabel("The Variable Parameters Will be Within the Specified Range."));
    selArrayDurA=new val_selector(1, "Duration", 0.001, 1000, 3, 0, {"ms"});
    conf["selArrayDurA"]=selArrayDurA;
    slayout->addWidget(selArrayDurA);
    selArrayDurB=new val_selector(1, "Duration upper limit", 0.001, 1000, 3, 0, {"ms"});
    conf["selArrayDurB"]=selArrayDurB;
    slayout->addWidget(selArrayDurB);
    selArrayFocA=new val_selector(-1, "Focus", -1000, 1000, 3, 0, {"um"});
    conf["selArrayFocA"]=selArrayFocA;
    slayout->addWidget(selArrayFocA);
    selArrayFocB=new val_selector(1, "Focus upper limit", -1000, 1000, 3, 0, {"um"});
    conf["selArrayFocB"]=selArrayFocB;
    slayout->addWidget(selArrayFocB);
    selArrayOneScanN=new val_selector(10, "Take and average This Many Scans: ", 1, 1000, 0);
    conf["selArrayOneScanN"]=selArrayOneScanN;
    slayout->addWidget(selArrayOneScanN);
    selArrayRandomize=new checkbox_gs(false,"Randomize Value Order");
    conf["selArrayRandomize"]=selArrayRandomize;
    slayout->addWidget(selArrayRandomize);
    saveMats=new checkbox_gs(true,"Extra Save Mats Containing D,F for Convenience.");
    conf["saveMats"]=saveMats;
    slayout->addWidget(saveMats);
    saveRF=new checkbox_gs(true,"Also save reflectivity.");
    conf["saveRF"]=saveRF;
    slayout->addWidget(saveRF);
    slayout->addWidget(new hline);
    selPrerunType=new smp_selector("Prerun type ", 0, {"Plateau","Peak"});
    connect(selPrerunType, SIGNAL(changed(int)), this, SLOT(onPrerunTypeChanged(int)));
    conf["selPrerunType"]=selPrerunType;
    slayout->addWidget(selPrerunType);
    selPlateauA=new val_selector(0, "Height", 0, 1000, 3, 0, {"nm"});
    conf["selPlateauA"]=selPlateauA;
    slayout->addWidget(selPlateauA);
    selPlateauB=new val_selector(0, "Height upper limit", 0, 1000, 3, 0, {"nm"});
    conf["selPlateauB"]=selPlateauB;
    slayout->addWidget(selPlateauB);
    selPeakXshift=new val_selector(0, "Peak shift X", -100, 100, 3, 0, {"um"});
    conf["selPeakXshift"]=selPeakXshift;
    slayout->addWidget(selPeakXshift);
    selPeakYshift=new val_selector(0, "Peak shift Y", -100, 100, 3, 0, {"um"});
    conf["selPeakYshift"]=selPeakYshift;
    slayout->addWidget(selPeakYshift);
    selPeakYshift->setToolTip("NOTE: baseline calibration needed for plateau/peak.\nNOTE: set height to 0 to disable.");
    slayout->addWidget(new hline);
    selMultiArrayType=new smp_selector("Multiple runs variable parameter: ", 0, {"none","Focus","Duration", "Plateau/Peak"});
    connect(selMultiArrayType, SIGNAL(changed(int)), this, SLOT(onSelMultiArrayTypeChanged(int)));
    conf["selMultiArrayType"]=selMultiArrayType;
    slayout->addWidget(selMultiArrayType);
    multiarrayN=new val_selector(1, "N of runs:", 1, 10000, 0);
    connect(multiarrayN, SIGNAL(changed(double)), this, SLOT(onMultiarrayNChanged(double)));
    conf["multiarrayN"]=multiarrayN;
    slayout->addWidget(multiarrayN);
    slayout->addWidget(new hline);
    report=new QLabel("");
    slayout->addWidget(new QLabel("Calibration run:"));
    slayout->addWidget(new twid(scheduleMultiWrite,btnWriteCalib,report));

    slayout->addWidget(new hline);
    slayout->addWidget(new QLabel("Calibration processing:"));
    btnProcessFocusMes=new QPushButton("Select Folders to Process Focus Measurements");
    connect(btnProcessFocusMes, SIGNAL(released()), this, SLOT(onProcessFocusMes()));
    slayout->addWidget(new twid(btnProcessFocusMes));

    slayout->addWidget(new hline);
    slayout->addWidget(new QLabel("Calibration curve fitting:"));
    fpLoad=new QPushButton("Load");
    connect(fpLoad, SIGNAL(released()), this, SLOT(onfpLoad()));
    fpClear=new QPushButton("Clear");
    fpClear->setEnabled(false);
    connect(fpClear, SIGNAL(released()), this, SLOT(onfpClear()));
    fitAndPlot=new QPushButton("Fit/Plot");
    fitAndPlot->setEnabled(false);
    connect(fitAndPlot, SIGNAL(released()), this, SLOT(onFitAndPlot()));
    nBSplineCoef=new val_selector(6, "NBsplineCoef", 4, 100, 0);
    conf["nBSplineCoef"]=nBSplineCoef;
    connect(nBSplineCoef, SIGNAL(changed()), this, SLOT(onNBSplineCoefChanged()));
    showBP=new checkbox_gs(false,"Show BP.");
    conf["showBreakPoints"]=showBP;
    connect(showBP, SIGNAL(changed()), this, SLOT(onNBSplineCoefChanged()));
    optimizeBP=new checkbox_gs(true,"Optimize BP.");
    conf["optimizeBreakPoints"]=optimizeBP;
    connect(optimizeBP, SIGNAL(changed()), this, SLOT(onNBSplineCoefChanged()));
    slayout->addWidget(new twid(fpLoad,fpClear,fitAndPlot));
    slayout->addWidget(new twid(nBSplineCoef,showBP,optimizeBP));
    fpList=new QLabel("");
    fpList->setVisible(false);
    slayout->addWidget(fpList);
}
pgCalib::~pgCalib(){
    if(cpwin!=nullptr) delete cpwin;
}
void pgCalib::onMultiarrayNChanged(double val){
    selArray(selArrayType->index, selMultiArrayType->index);
    btnWriteCalib->setEnabled(val==1);
    scheduleMultiWrite->setVisible(val!=1);
    if(val==1) while(!scheduledPos.empty()){
        ovl.rm_overlay(scheduledPos.front().ovlptr);
        scheduledPos.pop_front();
    }
    report->setText("");
}
void pgCalib::onSelArrayTypeChanged(int index){
    selArray(index, selMultiArrayType->index);
}
void pgCalib::onSelMultiArrayTypeChanged(int index){
    selArray(selArrayType->index, index);
}
void pgCalib::onPrerunTypeChanged(int type){
    selPeakXshift->setVisible(type==1);
    selPeakYshift->setVisible(type==1);
}
void pgCalib::selArray(int ArrayIndex, int MultiArrayIndex){
    if(multiarrayN->val==1) MultiArrayIndex=0;
    bool foc=ArrayIndex==1 || ArrayIndex==2 || ArrayIndex==4 || MultiArrayIndex==1;
    bool dur=ArrayIndex==0 || ArrayIndex==2 || ArrayIndex==3 || MultiArrayIndex==2;
    bool pla=MultiArrayIndex==3;
    selArrayFocA->setLabel(foc?"Focus lower limit":"Focus");
    selArrayDurA->setLabel(dur?"Duration lower limit":"Duration");
    selPlateauA->setLabel(pla?"Height lower limit":"Height");
    selArrayFocB->setVisible(foc);
    selArrayDurB->setVisible(dur);
    selPlateauB->setVisible(pla);
}

void pgCalib::onWCF(){
    if(!btnWriteCalib->isChecked()){
        if(multiarrayN->val>scheduledPos.size()) btnWriteCalib->setEnabled(false);
        return;
    }
    if(!go.pRPTY->connected) {QMessageBox::critical(this, "Error", "Error: Red Pitaya not Connected"); return;}

    std::time_t time=std::time(nullptr); std::tm ltime=*std::localtime(&time);
    std::stringstream ifolder;
    ifolder<<lastFolder;
    ifolder<<std::put_time(&ltime, "%Y-%m-%d-%H-%M-%S");
    std::string saveFolderName=QFileDialog::getSaveFileName(this, tr("Select Folder for Saving Calibration Data"),QString::fromStdString(ifolder.str()),tr("Folders")).toStdString();
    if(saveFolderName.empty()){
        btnWriteCalib->setChecked(false);
        return;
    }
    lastFolder=saveFolderName.substr(0,saveFolderName.find_last_of("/")+1);
    saveFolderName+="/";
    cv::utils::fs::createDirectory(saveFolderName);

    WCFArray(saveFolderName);
}
void pgCalib::onSchedule(){
    pgMGUI->wait4motionToComplete();
    QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
    double coords[3];
    pgMGUI->getPos(&coords[0], &coords[1], &coords[2]);
    if(scheduledPos.size()==multiarrayN->val){
        ovl.rm_overlay(scheduledPos.front().ovlptr);
        scheduledPos.pop_front();
    }
    cv::Size size(pgMGUI->mm2px(selArrayXsize->val*selArraySpacing->val/1000,0),pgMGUI->mm2px(selArrayYsize->val*selArraySpacing->val/1000,0));
    cv::Mat mat(size,CV_8U,255);
    scheduledPos.push_back({ovl.add_overlay(mat,pgMGUI->mm2px(coords[0]-pgBeAn->writeBeamCenterOfsX,0), pgMGUI->mm2px(coords[1]-pgBeAn->writeBeamCenterOfsY,0)),{coords[0],coords[1],coords[2]}});
    report->setText(QString::fromStdString(util::toString("Scheduled ",scheduledPos.size(),"/",multiarrayN->val)));
    if(scheduledPos.size()==multiarrayN->val) btnWriteCalib->setEnabled(true);
}

void pgCalib::saveMainConf(std::string filename){
    std::ofstream setFile(filename);     //this file contains some settings:
    setFile <<"#Objective_displacement_X(mm) Objective_displacement_Y(mm) Objective_displacement_Z(mm) MirauXYmmppx(mm/px)\n";
    setFile << std::fixed << std::setprecision(6);
    setFile <<pgMGUI->objectiveDisplacementX<<" "<<pgMGUI->objectiveDisplacementY<<" "<<pgMGUI->objectiveDisplacementZ<<" ";
    setFile << std::defaultfloat <<pgMGUI->getNmPPx()/1000000<<"\n";
    setFile.close();
}
void pgCalib::saveConf(std::string filename, double duration, double focus, double plateau, double peak, double peakXshift, double peakYshift){
    std::ofstream setFile(filename);            //this file contains some settings:
    setFile <<"#Duration(ms) Focus(um)";
    if(peak!=0 || plateau!=0) setFile <<" Plateau(nm)";
    if(peak!=0) setFile<<" Peak(nm) PeakXShift(um) PeakYShift(um)";
    setFile <<"\n"<<duration<<" "<<focus;
    if(peak!=0 || plateau!=0) setFile <<" "<<plateau;
    if(peak!=0) setFile<<" "<<peak<<" "<<peakXshift<<" "<<peakYshift;
    setFile <<"\n";
    setFile.close();
}

void pgCalib::WCFArray(std::string folder){
    cv::Mat WArray(selArrayYsize->val,selArrayXsize->val,CV_64FC2,cv::Scalar(0,0));         //Duration(ms), Focus(um)
    int arraySizeDur{1}, arraySizeFoc{1}, arraySizePla{1};
    int index=selArrayType->index;
    int mindex=selMultiArrayType->index;
    bool multiFoc=false;
    bool multiDur=false;
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
    switch(mindex){
    case 1: arraySizeFoc*=multiarrayN->val;                         // Focus
            multiFoc=true;
            break;
    case 2: arraySizeDur*=multiarrayN->val;                         // Duration
            multiDur=true;
            break;
    case 3: arraySizePla=multiarrayN->val;                          // Plateau
            break;
    }

    std::vector<double> arrayDur; arrayDur.reserve(arraySizeDur);
    std::vector<double> arrayFoc; arrayFoc.reserve(arraySizeFoc);
    std::vector<double> arrayPla; arrayFoc.reserve(arraySizePla);
    if(arraySizeDur==1) arrayDur.push_back(selArrayDurA->val);
    else for(int i=0;i!=arraySizeDur; i++) arrayDur.push_back(selArrayDurA->val*(1-(double)i/(arraySizeDur-1))+selArrayDurB->val*((double)i/(arraySizeDur-1)));
    if(arraySizeFoc==1) arrayFoc.push_back(selArrayFocA->val);
    else for(int i=0;i!=arraySizeFoc; i++) arrayFoc.push_back(selArrayFocA->val*(1-(double)i/(arraySizeFoc-1))+selArrayFocB->val*((double)i/(arraySizeFoc-1)));
    if(arraySizePla==1) arrayPla.push_back(selPlateauA->val);
    else for(int i=0;i!=arraySizePla; i++) arrayPla.push_back(selPlateauA->val*(1-(double)i/(arraySizePla-1))+selPlateauB->val*((double)i/(arraySizePla-1)));

    if(selArrayRandomize->val){                         //randomize if chosen
        std::mt19937 rnd(std::random_device{}());
        std::shuffle(arrayDur.begin(), arrayDur.end(), rnd);
        std::shuffle(arrayFoc.begin(), arrayFoc.end(), rnd);
        std::shuffle(arrayPla.begin(), arrayPla.end(), rnd);
    }


    double xSize=WArray.cols*pgMGUI->mm2px(selArraySpacing->val/1000,0);
    double ySize=WArray.rows*pgMGUI->mm2px(selArraySpacing->val/1000,0);

    // set ROI
    bool err=false;
    cv::Rect ROI, sROI;
    sROI.width=pgMGUI->mm2px(selArraySpacing->val/1000,0);
    sROI.height=sROI.width;

    if(go.pGCAM->iuScope->camCols/2-xSize/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX,0)<0 || go.pGCAM->iuScope->camCols/2-xSize/2-pgBeAn->writeBeamCenterOfsX+xSize>=go.pGCAM->iuScope->camCols){
        err=true;
    }else{
        ROI.x=go.pGCAM->iuScope->camCols/2-xSize/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX,0);
        ROI.width=xSize;
    }
    if(go.pGCAM->iuScope->camRows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY,0)<0 || go.pGCAM->iuScope->camRows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY,0)+ySize>=go.pGCAM->iuScope->camRows){
        err=true;
    }else{
        ROI.y=go.pGCAM->iuScope->camRows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY,0);
        ROI.height=ySize;
    }
    if(err){
        QMessageBox::critical(gui_settings, "Error", "The calibration ROI does not fit the viewport, aborting calibration.\n");
        btnWriteCalib->setChecked(false);
        return;
    }

    saveMainConf(util::toString(folder,"/main-settings.txt"));
    bool isPlateau=selPrerunType->index==0;
    if(multiarrayN->val>1){
        std::ofstream wfile(util::toString(folder,isPlateau?"/Plateau.txt":"/Peak.txt"));
        for(int i=0;i!=arrayPla.size(); i++){
            wfile<<arrayPla[i]<<"\n";
        }wfile.close();
    }
    for(int n=0;n!=multiarrayN->val;n++){
        report->setText(QString::fromStdString(util::toString(n,"/",multiarrayN->val)));
        for(int i=0;i!=WArray.cols; i++) for(int j=0;j!=WArray.rows; j++){              //populate 3D x2 array
            if(index==0) WArray.at<cv::Vec2d>(j,i)[0]=arrayDur[i+j*WArray.cols+(multiDur?n:0)*WArray.rows*WArray.cols];
            else if(index==2 || index==3) WArray.at<cv::Vec2d>(j,i)[0]=arrayDur[i+(multiDur?n:0)*WArray.cols];
            else WArray.at<cv::Vec2d>(j,i)[0]=arrayDur[multiDur?n:0];
            if(index==1) WArray.at<cv::Vec2d>(j,i)[1]=arrayFoc[i+j*WArray.cols+(multiFoc?n:0)*WArray.rows*WArray.cols];
            else if(index==2)  WArray.at<cv::Vec2d>(j,i)[1]=arrayFoc[j+(multiFoc?n:0)*WArray.rows];
            else if(index==4)  WArray.at<cv::Vec2d>(j,i)[1]=arrayFoc[i+(multiFoc?n:0)*WArray.cols];
            else WArray.at<cv::Vec2d>(j,i)[1]=arrayFoc[multiFoc?n:0];
        }
        if(transposeMat->val)
            cv::transpose(WArray,WArray);

        if(!scheduledPos.empty()){
            ovl.rm_overlay(scheduledPos.front().ovlptr);
            pgMGUI->move(scheduledPos.front().pos[0],scheduledPos.front().pos[1],scheduledPos.front().pos[2],true);
            pgMGUI->wait4motionToComplete();
            scheduledPos.pop_front();
        }

        if(saveMats->val){      //export values as matrices, for convenience
            std::string names[2]={"Duration","Focus"};
            for(int k=0;k!=2;k++){
                std::ofstream wfile(util::toString(folder,"/", names[k],".txt"), std::ofstream::out | std::ofstream::app);
                for(int i=0;i!=WArray.cols; i++){
                    for(int j=0;j!=WArray.rows; j++)
                        wfile<<WArray.at<cv::Vec2d>(j,i)[k]<<" ";
                    wfile<<"\n";
                }
                wfile<<"\n";
                wfile.close();
            }
        }
        if(WCFArrayOne(WArray, arrayPla.size()==1?arrayPla[0]:arrayPla[n], ROI, sROI, folder, isPlateau, selPeakXshift->val, selPeakYshift->val,n)){
            QMessageBox::critical(gui_settings, "Error", "Calibration failed/aborted. Measurements up to this point were saved.");
            btnWriteCalib->setChecked(false);
            if(multiarrayN->val>1) btnWriteCalib->setEnabled(false);
            report->setText(QString::fromStdString(util::toString("Aborted at ",n,"/",multiarrayN->val)));
            return;
        }
        if(transposeMat->val)
            cv::transpose(WArray,WArray);

    }
    report->setText(QString::fromStdString(util::toString("Done ",multiarrayN->val,"/",multiarrayN->val)));
    btnWriteCalib->setChecked(false);
    if(multiarrayN->val>1) btnWriteCalib->setEnabled(false);
}
bool pgCalib::WCFArrayOne(cv::Mat WArray, double plateau, cv::Rect ROI, cv::Rect sROI, std::string folder, bool isPlateau, double peakXshift, double peakYshift, unsigned n){
    if(pgFGUI->doRefocus(true, ROI, maxRedoRefocusTries)) return true;

    varShareClient<pgScanGUI::scanRes>* scanRes=pgSGUI->result.getClient();
    double xOfs=((WArray.cols-1)*selArraySpacing->val)/2000;         //in mm
    double yOfs=((WArray.rows-1)*selArraySpacing->val)/2000;

    if(plateau!=0){
        if(isPlateau){
            cv::Mat mplateau(WArray.rows+1, WArray.cols+1, CV_32F, cv::Scalar(plateau/1000000));
            pgWr->writeMat(&mplateau, 0, selArraySpacing->val/1000);
        }else{
            cv::Mat mplateau(WArray.rows, WArray.cols, CV_32F, cv::Scalar(plateau/1000000));
            pgMGUI->move(peakXshift/1000,peakYshift/1000,0);
            pgWr->writeMat(&mplateau, 0, selArraySpacing->val/1000, selArraySpacing->val/1000);
            pgMGUI->move(-peakXshift/1000,-peakYshift/1000,0);
        }
    }

    const pgScanGUI::scanRes* res;
    if(!btnWriteCalib->isChecked()){delete scanRes;return true;}        //abort
    if(pgSGUI->doNRounds((int)selArrayOneScanN->val, ROI, maxRedoScanTries,0,saveRF->val?1:0)) return true;
    if(!btnWriteCalib->isChecked()){delete scanRes;return true;}        //abort

    res=scanRes->get();
    pgScanGUI::saveScan(res, util::toString(folder,"/",n,"-before"), false, true, saveRF->val?1:0);

    for(int j=0;j!=WArray.rows; j++) for(int i=0;i!=WArray.cols; i++){   // separate them into individual scans
        cv::utils::fs::createDirectory(util::toString(folder,"/",n*WArray.cols*WArray.rows+i+j*WArray.cols));
        sROI.x=pgMGUI->mm2px(i*selArraySpacing->val/1000,0);
        sROI.y=pgMGUI->mm2px(j*selArraySpacing->val/1000,0);
        pgScanGUI::saveScan(res, sROI, util::toString(folder,"/",n*WArray.cols*WArray.rows+i+j*WArray.cols,"/before"), true, saveRF->val?1:0);
    }

    {std::lock_guard<std::mutex>lock(pgSGUI->MLP._lock_proc);
        pgMGUI->chooseObj(false);
        pgMGUI->move(-xOfs,yOfs,0);
        CTRL::CO CO(go.pRPTY);
        CO.clear(true);
        for(int j=0;j!=WArray.rows; j++){
            for(int i=0;i!=WArray.cols; i++){
                if(!btnWriteCalib->isChecked()){delete scanRes;return true;}        //abort

                pgMGUI->corCOMove(CO,0,0,WArray.at<cv::Vec2d>(j,i)[1]/1000);
                CO.addHold("X",CTRL::he_motion_ontarget);
                CO.addHold("Y",CTRL::he_motion_ontarget);
                CO.addHold("Z",CTRL::he_motion_ontarget);
                CO.pulseGPIO("wrLaser",WArray.at<cv::Vec2d>(j,i)[0]/1000);
                pgMGUI->corCOMove(CO,0,0,-WArray.at<cv::Vec2d>(j,i)[1]/1000);
                saveConf(util::toString(folder,"/",n*WArray.cols*WArray.rows+i+j*WArray.cols,"/settings.txt"), WArray.at<cv::Vec2d>(j,i)[0], WArray.at<cv::Vec2d>(j,i)[1], isPlateau?plateau:0, isPlateau?0:plateau, isPlateau?0:static_cast<double>(selArraySpacing->val), isPlateau?0:static_cast<double>(selArraySpacing->val));

                CO.execute();
                CO.clear(true);

                while(CO.getProgress()<0.5) QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
                if(i!=WArray.cols-1) pgMGUI->corCOMove(CO,selArraySpacing->val/1000,0,0);
            }
            if(j!=WArray.rows-1) pgMGUI->corCOMove(CO,-2*xOfs,-selArraySpacing->val/1000,0);
        }
        pgMGUI->move(-xOfs,yOfs,0);
        pgMGUI->chooseObj(true);
    }

    if(!btnWriteCalib->isChecked()){delete scanRes;return true;}        //abort
    if(pgSGUI->doNRounds((int)selArrayOneScanN->val, ROI, maxRedoScanTries,0,saveRF->val?1:0)) return true;
    if(!btnWriteCalib->isChecked()){delete scanRes;return true;}        //abort
    res=scanRes->get();
    pgScanGUI::saveScan(res, util::toString(folder,"/",n,"-after"), false, true, saveRF->val?1:0);

    for(int j=0;j!=WArray.rows; j++) for(int i=0;i!=WArray.cols; i++){   // separate them into individual scans
        sROI.x=pgMGUI->mm2px(i*selArraySpacing->val/1000,0);
        sROI.y=pgMGUI->mm2px(j*selArraySpacing->val/1000,0);
        pgScanGUI::saveScan(res, sROI, util::toString(folder,"/",n*WArray.cols*WArray.rows+i+j*WArray.cols,"/after"), true, saveRF->val?1:0);
    }

    delete scanRes;
    return false;
}

bool folderSort(std::string i,std::string j){
    size_t posi=i.find_last_of("/");
    size_t posj=j.find_last_of("/");
    return (std::stoi(i.substr(posi+1,9))<std::stoi(j.substr(posj+1,9)));
}
void pgCalib::onProcessFocusMes(){
    std::vector<std::string> readFolders;   //folders still yet to be checked
    std::vector<std::string> measFolders;   //folders that contain the expected measurement files

    readFolders.emplace_back(QFileDialog::getExistingDirectory(this, "Select Folder Containing Measurements. It will be Searched Recursively.", QString::fromStdString(lastFolder)).toStdString());
    if(readFolders.back().empty()) return;
    lastFolder=readFolders.back().substr(0,readFolders.back().find_last_of("/")+1);
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
                    else if(strcmp(entry->d_name,"after-RF.png")==0) dirHasMes[3]=true;
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
    wfile<<"# 11: plateau(nm)\n";
    wfile<<"# 12: duration(ms)\n";
    wfile<<"# 13: Peak Center Reflectivity change (averaged within FWHM)(a.u.)\n";
    wfile<<"# 14: Max absolute 1st der. (nm/um)\n";
    wfile<<"# 15: Min Laplacian (nm/um^2)\n";
    wfile<<"# 16: Max Laplacian (nm/um^2)\n";
    wfile<<"# 17: peak height(nm)(max)\n";
    wfile<<"# 18: peak height(nm)(max-min)\n";
    wfile<<"# 19: X width (FWHM)(um)\n";
    wfile<<"# 20: Y width (FWHM)(um)\n";
    wfile<<"# 21: XY width (FWHM)(um)\n";
    wfile<<"# 22: prepeak(nm)\n";
    wfile<<"# 23: prepeakXoffs(um)\n";
    wfile<<"# 24: prepeakYoffs(um)\n";
    wfile<<"# 25: Mirror tilt X(nm/nm)\n";
    wfile<<"# 26: Mirror tilt Y(nm/nm)\n";
    wfile<<"# 27: asymptotic standard error for 3: peak height(nm)\n";
    wfile<<"# 28: asymptotic standard error for 4: X width (1/e^2)(um)\n";
    wfile<<"# 29: asymptotic standard error for 5: Y width (1/e^2)(um)\n";
    wfile<<"# 30: asymptotic standard error for 6: ellipse angle(rad)\n";
    wfile<<"# 31: asymptotic standard error for 7: XY width (1/e^2)(um)\n";
    wfile<<"# 32: asymptotic standard error for 8: X offset(um)\n";
    wfile<<"# 33: asymptotic standard error for 9: Y offset(um)\n";
    wfile<<"# 34: asymptotic standard error for 10: XY offset(um)\n";
    wfile<<"# 35: asymptotic standard error for 19: X width (FWHM)(um)\n";
    wfile<<"# 36: asymptotic standard error for 20: Y width (FWHM)(um)\n";
    wfile<<"# 37: asymptotic standard error for 21: XY width (FWHM)(um)\n";

    std::vector<std::string> results;
    std::atomic<unsigned> completed=0;
    int n=0;
    for(auto& fldr:measFolders){
        results.emplace_back();
        results.back().reserve(26*100);
    }
    for(auto& fldr:measFolders){
        go.OCL_threadpool.doJob(std::bind(&pgCalib::calcParameters, this, fldr, &results[n], &completed));
        n++;
    }
    while(completed!=n){
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        pgSGUI->MLP.progress_comp=100./n*completed;
    }
    for(int i=0;i!=n;i++) wfile<<results[i];
    pgSGUI->MLP.progress_comp=100;
    wfile.close();
}



int pgCalib::gauss2De_f (const gsl_vector* pars, void* data, gsl_vector* f){
    size_t n=((struct fit_data*)data)->n;
    double* x=((struct fit_data*)data)->x;
    double* y=((struct fit_data*)data)->y;
    double* h=((struct fit_data*)data)->h;

    double x0=gsl_vector_get(pars, 0);
    double y0=gsl_vector_get(pars, 1);
    double a =gsl_vector_get(pars, 2);
    double wx=gsl_vector_get(pars, 3);
    double wy=gsl_vector_get(pars, 4);
    double an=gsl_vector_get(pars, 5);
    double i0=gsl_vector_get(pars, 6);
    double A=pow(cos(an),2)/2/pow(wx,2)+pow(sin(an),2)/2/pow(wy,2);
    double B=sin(2*an)/2/pow(wx,2)-sin(2*an)/2/pow(wy,2);
    double C=pow(sin(an),2)/2/pow(wx,2)+pow(cos(an),2)/2/pow(wy,2);
    for (size_t i=0; i!=n; i++){
        double model=i0+a*exp(-A*pow(x[i]-x0,2)-B*(x[i]-x0)*(y[i]-y0)-C*pow(y[i]-y0,2));
        gsl_vector_set(f, i, model-h[i]);
    }
    return GSL_SUCCESS;
}

int pgCalib::gauss2De_df (const gsl_vector* pars, void* data, gsl_matrix* J){
    size_t n=((struct fit_data*)data)->n;
    double* x=((struct fit_data*)data)->x;
    double* y=((struct fit_data*)data)->y;

    double x0=gsl_vector_get(pars, 0);
    double y0=gsl_vector_get(pars, 1);
    double a =gsl_vector_get(pars, 2);
    double wx=gsl_vector_get(pars, 3);
    double wy=gsl_vector_get(pars, 4);
    double an=gsl_vector_get(pars, 5);
    double A=pow(cos(an),2)/2/pow(wx,2)+pow(sin(an),2)/2/pow(wy,2);
    double B=sin(2*an)/2/pow(wx,2)-sin(2*an)/2/pow(wy,2);
    double C=pow(sin(an),2)/2/pow(wx,2)+pow(cos(an),2)/2/pow(wy,2);
    double dAdwx=-pow(cos(an),2)/pow(wx,3);
    double dAdwy=-pow(sin(an),2)/pow(wy,3);
    double dBdwx=-sin(2*an)/pow(wx,3);
    double dBdwy=+sin(2*an)/pow(wy,3);
    double dCdwx=-pow(sin(an),2)/pow(wx,3);
    double dCdwy=-pow(cos(an),2)/pow(wy,3);
    double dAdan=sin(an)*cos(an)*(1/pow(wy,2)-1/pow(wx,2));
    double dBdan=cos(2*an)*(1/pow(wx,2)-1/pow(wy,2));
    double dCdan=-dAdan;
    for (size_t i=0; i!=n; i++){
        double D=exp(-A*pow(x[i]-x0,2)-B*(x[i]-x0)*(y[i]-y0)-C*pow(y[i]-y0,2));
        gsl_matrix_set (J, i, 0, a*D*(2*A*(x[i]-x0)+B*(y[i]-y0)));
        gsl_matrix_set (J, i, 1, a*D*(2*C*(y[i]-y0)+B*(x[i]-x0)));
        gsl_matrix_set (J, i, 2, D);
        gsl_matrix_set (J, i, 3, a*D*(-pow((x[i]-x0),2)*dAdwx-(x[i]-x0)*(y[i]-y0)*dBdwx-pow((y[i]-y0),2)*dCdwx));
        gsl_matrix_set (J, i, 4, a*D*(-pow((x[i]-x0),2)*dAdwy-(x[i]-x0)*(y[i]-y0)*dBdwy-pow((y[i]-y0),2)*dCdwy));
        gsl_matrix_set (J, i, 5, a*D*(-pow((x[i]-x0),2)*dAdan-(x[i]-x0)*(y[i]-y0)*dBdan-pow((y[i]-y0),2)*dCdan));
        gsl_matrix_set (J, i, 6, 1.);
    }
    return GSL_SUCCESS;
}

void pgCalib::calcParameters(std::string fldr, std::string* output, std::atomic<unsigned>* completed){
    double focus;
    double duration;
    double plateau{0};
    double prepeak{0}, prepeakXofs{0}, prepeakYofs{0};
    std::ifstream ifs(util::toString(fldr,"/settings.txt"));
    ifs.ignore(std::numeric_limits<std::streamsize>::max(),'\n');       // ignore header
    ifs>>duration;
    ifs>>focus;
    ifs>>plateau;
    ifs>>prepeak;
    ifs>>prepeakXofs;
    ifs>>prepeakYofs;
    ifs.close();

    pgScanGUI::scanRes scanBefore, scanAfter;
    if(!pgScanGUI::loadScan(&scanBefore, util::toString(fldr,"/before.pfm"))) return;
    if(!pgScanGUI::loadScan(&scanAfter, util::toString(fldr,"/after.pfm"))) return;
    pgScanGUI::scanRes scanDif=pgSGUI->difScans(&scanBefore, &scanAfter);
    pgScanGUI::saveScan(&scanDif, util::toString(fldr,"/scandif.pfm"));

    const size_t parN=7;
    double initval[parN] = { scanDif.depth.cols/2., scanDif.depth.rows/2., 0, 4000/scanDif.XYnmppx, 4000/scanDif.XYnmppx, 0.01, 0};
    cv::minMaxIdx(scanDif.depth,&initval[6],&initval[2]);
    size_t ptN=scanDif.depth.cols*scanDif.depth.rows;
    double x[ptN], y[ptN], h[ptN], weights[ptN];
    size_t n{0};
    for(int i=0;i!=scanDif.depth.cols;i++) for(int j=0;j!=scanDif.depth.rows;j++)
        if(scanDif.mask.at<uchar>(j,i)==0){
            x[n]=i;
            y[n]=j;
            h[n]=scanDif.depth.at<float>(j,i);
            weights[n]=1;
            n++;
        }
    ptN=n;
    if(ptN<parN){(*completed)++;return;}

    struct fit_data data{ptN,x,y,h};
    gsl_multifit_nlinear_fdf fdf{gauss2De_f,gauss2De_df,NULL,ptN,parN,&data,0,0,0};
    gsl_multifit_nlinear_parameters fdf_params=gsl_multifit_nlinear_default_parameters();
    gsl_multifit_nlinear_workspace* workspace=gsl_multifit_nlinear_alloc(gsl_multifit_nlinear_trust, &fdf_params, ptN, parN);

    gsl_vector_view v_startp=gsl_vector_view_array(initval, parN);
    gsl_vector_view v_weights = gsl_vector_view_array(weights, ptN);
    gsl_multifit_nlinear_winit (&v_startp.vector, &v_weights.vector, &fdf, workspace);

    gsl_vector* fcost;
    fcost=gsl_multifit_nlinear_residual(workspace);

    int info;
    const double xtol=1e-8;
    const double gtol=1e-8;
    const double ftol=0.0;
    const unsigned maxIter=1000;
    int status=gsl_multifit_nlinear_driver(maxIter, xtol, gtol, ftol, NULL, NULL, &info, workspace);

    gsl_matrix* Jac;
    Jac=gsl_multifit_nlinear_jac(workspace);
    gsl_matrix* covar = gsl_matrix_alloc(parN, parN);
    gsl_multifit_nlinear_covar (Jac, 0.0, covar);

    double chisq;
    gsl_blas_ddot(fcost, fcost, &chisq);
    double res[parN], err[parN];

    for(int i=0;i!=parN;i++){
        res[i]=gsl_vector_get(workspace->x, i);
        err[i]=GSL_MAX_DBL(1,sqrt(chisq/(ptN-parN)))*sqrt(gsl_matrix_get(covar,i,i));
    }
    gsl_multifit_nlinear_free (workspace);
    gsl_matrix_free (covar);

    while(res[5]>M_PI) res[5]-=M_PI;
    while(res[5]<0) res[5]+=M_PI;
    if(res[5]>=M_PI/2){
       res[5]-= M_PI/2;
       double tmp=res[3];
       res[3]=res[4];
       res[4]=tmp;
    }

    const double toFWHM=sqrt(2*log(2));

    double peakRefl{0};
    int nref=0;
    cv::Mat reflb=imread(util::toString(fldr,"/before-RF.png"), cv::IMREAD_GRAYSCALE);
    cv::Mat refla=imread(util::toString(fldr,"/after-RF.png"), cv::IMREAD_GRAYSCALE);
    if(!reflb.empty() && !refla.empty())
        for(int i=0;i!=reflb.cols;i++) for(int j=0;j!=reflb.rows;j++)
            if(pow((i-res[0]),2)+pow((j-res[1]),2)<=pow((res[3]+res[4])/2*toFWHM,2)){
                peakRefl+=static_cast<double>(refla.at<uint8_t>(j,i))-static_cast<double>(reflb.at<uint8_t>(j,i));
                nref++;
            }
    peakRefl/=nref;

    //find max abs first derivative of depth (nm/um):
    const int ksize=5;
    cv::Mat du, dx,dy;
    cv::Sobel(scanDif.depth, dx, CV_32F,1,0,ksize);  //first derivative with smoothing
    cv::Sobel(scanDif.depth, dy, CV_32F,0,1,ksize);
    cv::magnitude(dx,dy,du);
    double maxDepthDer;
    cv::minMaxIdx(du, nullptr, &maxDepthDer);
    //find min, max laplacian of depth (nm/um^2):
    cv::Laplacian(scanDif.depth, du, CV_32F, ksize);
    double minDepthLaplacian, maxDepthLaplacian;
    cv::minMaxIdx(du, &minDepthLaplacian, &maxDepthLaplacian);

    double XYumppx=scanDif.XYnmppx/1000;
    maxDepthDer/=XYumppx;
    minDepthLaplacian/=XYumppx;
    maxDepthLaplacian/=XYumppx;

    double Xwidth=2*abs(res[3])*XYumppx;                        double eXwidth=2*abs(err[3])*XYumppx;
    double Ywidth=2*abs(res[4])*XYumppx;                        double eYwidth=2*abs(err[4])*XYumppx;
    double XYwidth=(Xwidth+Ywidth)/2;                           double eXYwidth=(eXwidth+eYwidth)/2;
    double Xofs=(res[0]-scanDif.depth.cols/2.)*XYumppx;         double eXofs=err[0]*XYumppx;
    double Yofs=(res[1]-scanDif.depth.rows/2.)*XYumppx;         double eYofs=err[1]*XYumppx;
    double XYofs=sqrt(pow(Xofs,2)+pow(Yofs,2));                 double eXYofs=eXofs*Xofs/XYofs+eYofs*Yofs/XYofs;
    double tiltX=scanBefore.tiltCor[0]/scanBefore.XYnmppx;
    double tiltY=scanBefore.tiltCor[1]/scanBefore.XYnmppx;

    int valid=1;
    if(res[0]<0 || res[0]>scanDif.depth.cols || res[1]<0 || res[1]>scanDif.depth.rows || res[2]<=0 || status!=0) valid=0;    //fit failure
    for(int i=0;i!=7;i++) if(err[i]>1e3) valid=0;    // fit failed if errors are arbitrarily high
    *output=util::toString(
        valid," ",                      // 1: valid measurement(=1)
        focus," ",                      // 2: focus distance(um)
        res[2]," ",                     // 3: peak height(nm)
        Xwidth," ",                     // 4: X width (1/e^2)(um)
        Ywidth," ",                     // 5: Y width (1/e^2)(um)
        res[5]," ",                     // 6: ellipse angle(rad)
        XYwidth," ",                    // 7: XY width (1/e^2)(um)
        Xofs," ",                       // 8: X offset(um)
        Yofs," ",                       // 9: Y offset(um)
        XYofs," ",                      // 10: XY offset(um)
        plateau," ",                    // 11: plateau(nm)
        duration," ",                   // 12: duration(ms)
        peakRefl," ",                   // 13: Peak Center Reflectivity change (averaged within FWHM)(a.u.)\n";
        maxDepthDer," ",                // 14: Max absolute 1st der. (nm/um)
        minDepthLaplacian," ",          // 15: Min Laplacian (nm/um^2)
        maxDepthLaplacian," ",          // 16: Max Laplacian (nm/um^2)
        scanDif.max," ",                // 17: peak height(nm)(max)
        scanDif.max-scanDif.min," ",    // 18: peak height(nm)(max-min)
        Xwidth*toFWHM," ",              // 19: X width (FWHM)(um)
        Ywidth*toFWHM," ",              // 20: Y width (FWHM)(um)
        XYwidth*toFWHM," ",             // 21: XY width (FWHM)(um)
        prepeak," ",                    // 22: prepeak(nm)
        prepeakXofs," ",                // 23: prepeakXoffs(um)
        prepeakYofs," ",                // 24: prepeakYoffs(um)
        tiltX," ",                      // 25: Mirror tilt X(nm/nm)
        tiltY," ",                      // 26: Mirror tilt Y(nm/nm)
        err[2]," ",                     // 27: asymptotic standard error for 3: peak height(nm)
        eXwidth," ",                    // 28: asymptotic standard error for 4: X width (1/e^2)(um)
        eYwidth," ",                    // 29: asymptotic standard error for 5: Y width (1/e^2)(um)
        err[2]," ",                     // 30: asymptotic standard error for 6: ellipse angle(rad)
        eXYwidth," ",                   // 31: asymptotic standard error for 7: XY width (1/e^2)(um)
        eXofs," ",                      // 32: asymptotic standard error for 8: X offset(um)
        eYofs," ",                      // 33: asymptotic standard error for 9: Y offset(um)
        eXYofs," ",                     // 34: asymptotic standard error for 10: XY offset(um)
        eXwidth*toFWHM," ",             // 35: asymptotic standard error for 19: X width (FWHM)(um)
        eYwidth*toFWHM," ",             // 36: asymptotic standard error for 20: Y width (FWHM)(um)
        eXYwidth*toFWHM,"\n"            // 37: asymptotic standard error for 21: XY width (FWHM)(um)
                );

    (*completed)++;
    return;
}


void pgCalib::onChangeDrawWriteAreaOn(bool status){
    drawWriteAreaOn=status&(multiarrayN->val==1);
}
void pgCalib::onChangeDrawWriteAreaOnSch(bool status){
    drawWriteAreaOn=status;
}
void pgCalib::drawWriteArea(cv::Mat* img){
    if(!drawWriteAreaOn) return;
    double xSize;
    double ySize;
    xSize=pgMGUI->mm2px(selArrayXsize->val*selArraySpacing->val/1000);
    ySize=pgMGUI->mm2px(selArrayYsize->val*selArraySpacing->val/1000);
    if(transposeMat->val) std::swap(xSize,ySize);
    double clr[2]={0,255}; int thck[2]={3,1};
    for(int i=0;i!=2;i++)
        cv::rectangle(*img,  cv::Rect(img->cols/2-xSize/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX), img->rows/2-ySize/2+pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY), xSize, ySize), {clr[i]}, thck[i], cv::LINE_AA);
}

void pgCalib::onfpLoad(){
    std::vector<std::string> readFolders;
    std::vector<std::string> measFiles;

    readFolders.emplace_back(QFileDialog::getExistingDirectory(this, "Select Folder Containing Processed Measurements. It will be Searched Recursively for proc.txt files.", QString::fromStdString(lastFolder)).toStdString());
    if(readFolders.back().empty()) return;
    lastFolder=readFolders.back().substr(0,readFolders.back().find_last_of("/")+1);
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
        if(wp!=nullptr) while(entry=readdir(wp)){
            curFile=entry->d_name;
            if (curFile!="." && curFile!=".."){
                curFile=curFolder+'/'+entry->d_name;

                stat(curFile.data(),&filetype);
                if(filetype.st_mode&S_IFDIR) readFolders.push_back(curFile);
                else if(strcmp(entry->d_name,"proc.txt")==0) measFiles.push_back(curFile);
            }
        }
        closedir(wp);
    }

    const size_t pos[4]={1,12,3,27};    // indices in data(starting from 1): valid, duration, height, height_err
    const size_t maxpos=*std::max_element(pos,pos+4);
    for(auto& item:measFiles){
        std::ifstream ifs(item);
        for(std::string line; std::getline(ifs, line);){
            if(line[0]=='#') continue;
            size_t sx=0, sxi;
            bool valid;
            double _duration, _height, _height_err;
            for(size_t i=1;i<=maxpos;i++){
                double tmp=std::stod(line.substr(sx),&sxi);
                sx+=sxi;
                if(i==pos[0]) valid=(tmp==1);
                else if(i==pos[1]) _duration=tmp;
                else if(i==pos[2]) _height=tmp;
                else if(i==pos[3]) _height_err=tmp;
            }
            if(valid){
                fpLoadedData.push_back({_duration,_height,_height_err});
            }
        }
        ifs.close();
    }

    if(fpLoadedData.empty()) return;

    // sort by pulse duration
    std::sort(fpLoadedData.begin(), fpLoadedData.end(), [](durhe_data i,durhe_data j){return (i.duration<j.duration);});
    fitAndPlot->setEnabled(true);
    fpClear->setEnabled(true);
    QString list=fpList->text();
    for(auto& item:measFiles) {list+=item.substr(item.find_last_of('/',item.find_last_of('/')-1)+1).c_str(); list+="\n";}
    fpList->setText(list);
    fpList->setVisible(true);
}
void pgCalib::onfpClear(){
    fpLoadedData.clear();
    fitAndPlot->setEnabled(false);
    fpClear->setEnabled(false);
    fpList->setText("");
    fpList->setVisible(false);
}


int pgCalib::bspline_f(const gsl_vector* pars, void* data, gsl_vector* f){
    size_t mesN=((struct fit_data_och*)data)->n;
    double* x=((struct fit_data_och*)data)->x;
    double* y=((struct fit_data_och*)data)->y;
    double* w=((struct fit_data_och*)data)->w;
    double* _breakpts=((struct fit_data_och*)data)->breakpts;
    double* _fitpars=((struct fit_data_och*)data)->fitpars;
    double* _bsplcoef=((struct fit_data_och*)data)->bsplcoef;
    double* _covmat=((struct fit_data_och*)data)->covmat;
    size_t ncoeffs=((struct fit_data_och*)data)->ncoeffs;
    size_t nbreak=ncoeffs-2;

    double breakDis[nbreak-2];
    for(size_t i=0;i!=nbreak-2;i++) breakDis[i]=gsl_vector_get(pars, i);

    gsl_bspline_workspace *bsplws=gsl_bspline_alloc(4, nbreak); // cubis bspline
    gsl_vector bsplcoef=gsl_vector_view_array(_bsplcoef,ncoeffs).vector;
    gsl_vector fitpars=gsl_vector_view_array(_fitpars,ncoeffs).vector;
    gsl_vector weights=gsl_vector_view_array(w,mesN).vector;
    gsl_vector yvec=gsl_vector_view_array(y,mesN).vector;
    gsl_matrix *predVarMat=gsl_matrix_alloc(mesN, ncoeffs);
    gsl_matrix covmat=gsl_matrix_view_array(_covmat,ncoeffs,ncoeffs).matrix;
    gsl_multifit_linear_workspace *mfitws=gsl_multifit_linear_alloc(mesN, ncoeffs);
    gsl_vector breakpts=gsl_vector_view_array(_breakpts,nbreak).vector;
    gsl_vector_set(&breakpts, 0, x[0]);
    for(size_t i=0;i!=nbreak-2;i++) gsl_vector_set(&breakpts, i+1, breakpts.data[i]+abs(breakDis[i]));
    gsl_vector_set(&breakpts, nbreak-1, x[mesN-1]);
    gsl_bspline_knots(&breakpts,bsplws);
    for (size_t i=0;i!=mesN;i++){
        gsl_bspline_eval(x[i], &bsplcoef, bsplws);   // compute bspline coefs
        for (size_t j=0;j!=ncoeffs;j++)
            gsl_matrix_set(predVarMat, i, j, gsl_vector_get(&bsplcoef, j));  //  construct the fit matrix
    }

    double ybs, yerr, ign;
    gsl_multifit_wlinear(predVarMat, &weights, &yvec, &fitpars, &covmat, &ign, mfitws); // fit

    for (size_t i=0; i!=mesN; i++){
        gsl_bspline_eval(x[i], &bsplcoef, bsplws);
        gsl_multifit_linear_est(&bsplcoef, &fitpars, &covmat, &ybs, &yerr);
        if(f!=nullptr) gsl_vector_set(f, i, ybs-y[i]);
    }

    gsl_bspline_free(bsplws);
    gsl_matrix_free(predVarMat);
    gsl_multifit_linear_free(mfitws);

    return GSL_SUCCESS;
}

void pgCalib::onFitAndPlot(){
    std::vector<double> duration, height, height_err, weight;
    for(auto& el:fpLoadedData){
        duration.push_back(el.duration);
        height.push_back(el.height);
        height_err.push_back(el.height_err);
    }

    if(cpwin==nullptr) cpwin=new CvPlotQWindow;
    cpwin->axes=CvPlot::makePlotAxes();
    cpwin->axes.xLabel("Pulse Duration (ms)");
    cpwin->axes.yLabel("Peak Height (nm)");
    auto& sdata=cpwin->axes.create<CvPlot::Series>(duration, height, "o");
    sdata.setName("Data");
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    duration.insert(duration.begin(),0);      // add a zero point for fitting
    height.insert(height.begin(),0);
    height_err.insert(height_err.begin(),1e-5);

    const double min=0;
    const double max=*std::max_element(duration.begin(),duration.end());

    const size_t ncoeffs=static_cast<size_t>(nBSplineCoef->val);
    const size_t nMes=duration.size();
    gsl_vector *bsplcoef=gsl_vector_alloc(ncoeffs);
    gsl_vector *fitpars=gsl_vector_alloc(ncoeffs);
    gsl_matrix *covmat=gsl_matrix_alloc(ncoeffs, ncoeffs);
    const size_t nbreak = ncoeffs-2;    // must be at least 2
    std::vector<double>breakpts(nbreak,0);
    gsl_vector gbreakpts=gsl_vector_view_array(breakpts.data(),nbreak).vector;
    cv::divide(1.,height_err,weight,CV_64F);
    cv::divide(weight,height_err,weight,CV_64F);
    fit_data_och data{
        nMes,
        ncoeffs,
        duration.data(),
        height.data(),
        weight.data(),
        breakpts.data(),
        fitpars->data,
        bsplcoef->data,
        covmat->data
    };
    gsl_vector *pars=gsl_vector_alloc(nbreak-2);
    gsl_vector_set_all(pars,(max-min)/(nbreak-1));
    if(nbreak==2 || !optimizeBP->val){
        bspline_f(pars, &data);
    }else{
        gsl_multifit_nlinear_fdf fdf{bspline_f,NULL,NULL,nMes,nbreak-2,&data,0,0,0};
        gsl_multifit_nlinear_parameters fdf_params=gsl_multifit_nlinear_default_parameters();
        gsl_multifit_nlinear_workspace* workspace=gsl_multifit_nlinear_alloc(gsl_multifit_nlinear_trust, &fdf_params, nMes, nbreak-2);

        gsl_vector weights=gsl_vector_view_array(weight.data(),nMes).vector;
        gsl_multifit_nlinear_winit (pars, &weights, &fdf, workspace);

        gsl_vector* fcost;
        fcost=gsl_multifit_nlinear_residual(workspace);

        int info;
        const double xtol=1e-8;
        const double gtol=1e-8;
        const double ftol=0.0;
        const unsigned maxIter=1000;
        gsl_multifit_nlinear_driver(maxIter, xtol, gtol, ftol, NULL, NULL, &info, workspace);
        gsl_multifit_nlinear_free (workspace);
    }

    gsl_bspline_workspace *bsplws=gsl_bspline_alloc(4, nbreak); // cubis bspline
    gsl_bspline_knots(&gbreakpts,bsplws);
    std::vector<double> fduration, fheight;
    double nplot=1000;
    double step=(max-min)/(nplot-1);
    double yerr;
    for(int i=0;i!=nplot;i++){
        fduration.push_back(min+i*step);
        fheight.emplace_back();
        gsl_bspline_eval(fduration.back(), bsplcoef, bsplws);
        gsl_multifit_linear_est(bsplcoef, fitpars, covmat, &fheight.back(), &yerr);
    }

    gsl_vector_free(bsplcoef);
    gsl_vector_free(fitpars);
    gsl_matrix_free(covmat);

    if(showBP->val){
        auto& sbp=cpwin->axes.create<CvPlot::Series>(breakpts, std::vector(nbreak,0), "og");
        sbp.setName("Breakpoints");
    }
    auto& sfit=cpwin->axes.create<CvPlot::Series>(fduration, fheight, "-r");
    sfit.setName("Spline fit");
    cpwin->redraw();
    cpwin->show();
    cpwin->activateWindow();    // takes focus
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}
void pgCalib::onNBSplineCoefChanged(){
    if(fpLoadedData.empty()) return;
    if(cpwin!=nullptr) if(cpwin->isVisible()){
        onFitAndPlot();
    }
}
