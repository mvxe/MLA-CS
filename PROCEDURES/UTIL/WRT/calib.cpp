#include "calib.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include "PROCEDURES/UTIL/USC/focus.h"
#include "PROCEDURES/UTIL/USC/move.h"
#include "opencv2/core/utils/filesystem.hpp"
#include "GUI/tab_monitor.h"    //for debug purposes
#include <algorithm>
#include <random>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_sort_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlinear.h>
#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_statistics.h>
#include <CvPlot/cvplot.h>
#include <opencv2/highgui.hpp>
#include <filesystem>

pgCalib::pgCalib(pgScanGUI* pgSGUI, pgFocusGUI* pgFGUI, pgMoveGUI* pgMGUI, pgBeamAnalysis* pgBeAn, pgWrite* pgWr, overlay& ovl): pgSGUI(pgSGUI), pgFGUI(pgFGUI), pgMGUI(pgMGUI), pgBeAn(pgBeAn), pgWr(pgWr), ovl(ovl){
    btnWriteCalib=new HQPushButton("Run calibration");
    connect(btnWriteCalib, SIGNAL(released()), this, SLOT(onWCF()));
    connect(btnWriteCalib, SIGNAL(changed(bool)), this, SLOT(onChangeDrawWriteAreaOn(bool)));
    scheduleMultiWrite=new HQPushButton("Schedule");
    connect(scheduleMultiWrite, SIGNAL(released()), this, SLOT(onSchedule()));
    connect(scheduleMultiWrite, SIGNAL(changed(bool)), this, SLOT(onChangeDrawWriteAreaOnSch(bool)));
    scheduleMultiWrite->setVisible(false);
    overlappingCalib=new QPushButton("Overlapping run");
    overlappingCalib->setToolTip("After a successful calibration run, you may do a rerun with overlapping points. Note that if you have randomize enabled and change focus, the points may not be aligned.\n"
                                 "The following parameters must stay the same: 'Array Size X&Y', 'Array Spacing', 'N of runs' and 'Transpose matrix'. Plateau will be ignored for subsequent runs.\n"
                                 "The data will be saved in the same folder as the previous run. Multiple reruns may be done.");
    connect(overlappingCalib, SIGNAL(released()), this, SLOT(onOverlappingCalib()));
    overlappingCalib->setEnabled(false);
    ovl_xofs=new val_selector(0, "offsets X:", -1, 1, 3, 0, {"um"});
    ovl_yofs=new val_selector(0, "Y:", -1, 1, 3, 0, {"um"});
    ovl_xofs->setEnabled(false);
    ovl_yofs->setEnabled(false);
    ovl_xofs->setToolTip("NOTE: the offset is relative in relation to the original calibration!");
    ovl_yofs->setToolTip(ovl_xofs->toolTip());

    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);

    selArrayXsize=new val_selector(10, "Array Size X", 1, 1000, 0);
    conf["selArrayXsize"]=selArrayXsize;
    connect(selArrayXsize, SIGNAL(changed()), this, SLOT(updateOverlappingCalibEnabled()));
    selArrayYsize=new val_selector(10, "Array Size Y", 1, 1000, 0);
    conf["selArrayYsize"]=selArrayYsize;
    connect(selArrayYsize, SIGNAL(changed()), this, SLOT(updateOverlappingCalibEnabled()));
    slayout->addWidget(new twid{selArrayXsize,selArrayYsize});
    selArraySpacing=new val_selector(5, "Array Spacing", 0.001, 100, 3, 0, {"um"});
    conf["selArraySpacing"]=selArraySpacing;
    connect(selArraySpacing, SIGNAL(changed()), this, SLOT(updateOverlappingCalibEnabled()));
    slayout->addWidget(selArraySpacing);
    selArrayType=new smp_selector("Variable Parameters (X-Y): ", 0, {"Duration(X,Y), no repeat","Focus(X,Y), no repeat","Duration(X)-Focus(Y)", "Duration(X,Y), repeat","Focus(X,Y), repeat", "none (for prepeak)"});
    connect(selArrayType, SIGNAL(changed(int)), this, SLOT(onSelArrayTypeChanged(int)));
    conf["selArrayType"]=selArrayType;
    slayout->addWidget(selArrayType);
    transposeMat=new checkbox_gs(false,"Transpose matrix.");
    conf["transposeMat"]=transposeMat;
    connect(transposeMat, SIGNAL(changed()), this, SLOT(updateOverlappingCalibEnabled()));
    slayout->addWidget(transposeMat);
    slayout->addWidget(new QLabel("The Variable Parameters Will be Within the Specified Range."));
    selArrayDurA=new val_selector(1, "Duration", 0.0001, 1000, 4, 0, {"ms"});
    conf["selArrayDurA"]=selArrayDurA;
    slayout->addWidget(selArrayDurA);
    selArrayDurB=new val_selector(1, "Duration upper limit", 0.0001, 1000, 4, 0, {"ms"});
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
    selPlateauA=new val_selector(0, "Plateau Height", 0, 1000, 3, 0, {"nm"});
    conf["selPlateauA"]=selPlateauA;
    slayout->addWidget(selPlateauA);
    selPlateauB=new val_selector(0, "Plateau upper limit", 0, 1000, 3, 0, {"nm"});
    conf["selPlateauB"]=selPlateauB;
    slayout->addWidget(selPlateauB);
    slayout->addWidget(new hline);
    selMultiArrayType=new smp_selector("Multiple runs variable parameter: ", 0, {"none","Focus","Duration", "Plateau"});
    connect(selMultiArrayType, SIGNAL(changed(int)), this, SLOT(onSelMultiArrayTypeChanged(int)));
    conf["selMultiArrayType"]=selMultiArrayType;
    slayout->addWidget(selMultiArrayType);
    multiarrayN=new val_selector(1, "N of runs:", 1, 10000, 0);
    connect(multiarrayN, SIGNAL(changed(double)), this, SLOT(onMultiarrayNChanged(double)));
    conf["multiarrayN"]=multiarrayN;
    connect(multiarrayN, SIGNAL(changed()), this, SLOT(updateOverlappingCalibEnabled()));
    slayout->addWidget(multiarrayN);
    slayout->addWidget(new hline);
    report=new QLabel("");
    slayout->addWidget(new QLabel("Calibration run:"));
    slayout->addWidget(new twid(scheduleMultiWrite,btnWriteCalib,report));
    slayout->addWidget(new twid(overlappingCalib,ovl_xofs,ovl_yofs));

    gui_settings_proc=new QWidget;
    splayout=new QVBoxLayout;
    gui_settings_proc->setLayout(splayout);

    btnProcessFocusMes=new QPushButton("Select Folders to Process Focus Measurements");
    connect(btnProcessFocusMes, SIGNAL(released()), this, SLOT(onProcessFocusMes()));
    splayout->addWidget(new twid(btnProcessFocusMes));
    nPeakFit=new val_selector(1, "Overlapping fit peak number", 1, 100, 0);
    conf["nPeakFit"]=nPeakFit;
    splayout->addWidget(nPeakFit);

    splayout->addWidget(new hline);
    splayout->addWidget(new QLabel("Calibration curve fitting (duration-height):"));
    fpLoad=new QPushButton("Load");
    connect(fpLoad, SIGNAL(released()), this, SLOT(onfpLoad()));
    fpClear=new QPushButton("Clear");
    fpClear->setEnabled(false);
    connect(fpClear, SIGNAL(released()), this, SLOT(onfpClear()));
    fitAndPlot=new QPushButton("Fit/Plot");
    fitAndPlot->setEnabled(false);
    connect(fitAndPlot, SIGNAL(released()), this, SLOT(onFitAndPlot()));
    fpApply=new QPushButton("Apply");
    fpApply->setEnabled(false);
    connect(fpApply, SIGNAL(released()), this, SLOT(onApply()));
    applyMenu=new QMenu;
    connect(applyMenu, SIGNAL(triggered(QAction*)), this, SLOT(onApplyMenuTriggered(QAction*)));
    nBSplineCoef=new val_selector(6, "NBsplineCoef", 4, 100, 0);
    conf["nBSplineCoef"]=nBSplineCoef;
    connect(nBSplineCoef, SIGNAL(changed()), this, SLOT(onNBSplineCoefChanged()));
    showBP=new checkbox_gs(false,"Show BP.");
    conf["showBreakPoints"]=showBP;
    connect(showBP, SIGNAL(changed()), this, SLOT(onNBSplineCoefChanged()));
    optimizeBP=new checkbox_gs(true,"Optimize BP.");
    conf["optimizeBreakPoints"]=optimizeBP;
    connect(optimizeBP, SIGNAL(changed()), this, SLOT(onNBSplineCoefChanged()));
    upperLim=new QSlider(Qt::Horizontal);
    upperLim->setMaximum(1000);
    upperLim->setValue(1000);
    upperLim->setTracking(false);
    connect(upperLim, SIGNAL(valueChanged(int)), this, SLOT(onNBSplineCoefChanged()));
    splayout->addWidget(new twid(fpLoad,fpClear,fitAndPlot,fpApply));
    splayout->addWidget(new twid(nBSplineCoef,showBP,optimizeBP));
    splayout->addWidget(new twid(new QLabel("Fit upper limit:"),upperLim));
    fpList=new QLabel("");
    fpList->setVisible(false);
    splayout->addWidget(fpList);
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
void pgCalib::selArray(int ArrayIndex, int MultiArrayIndex){
    if(multiarrayN->val==1) MultiArrayIndex=0;
    bool foc=ArrayIndex==1 || ArrayIndex==2 || ArrayIndex==4 || MultiArrayIndex==1;
    bool dur=ArrayIndex==0 || ArrayIndex==2 || ArrayIndex==3 || MultiArrayIndex==2;
    bool pla=MultiArrayIndex==3;
    selArrayFocA->setLabel(foc?"Focus lower limit":"Focus");
    selArrayDurA->setLabel(dur?"Duration lower limit":"Duration");
    selPlateauA->setLabel(pla?"Plateau lower limit":"Plateau Height");
    selArrayFocB->setVisible(foc);
    selArrayDurB->setVisible(dur);
    selPlateauB->setVisible(pla);
}

void pgCalib::onWCF(){
    if(btnWriteCalib->text()=="Abort calibration"){
        btnWriteCalib->setEnabled(false);
        wcabort=true;
        pgWr->wabort=true;
        return;
    }
    if(!go.pRPTY->connected) {QMessageBox::critical(this, "Error", "Error: Red Pitaya not Connected"); return;}

    btnWriteCalib->setText("Abort calibration");
    wcabort=false;
    drawWriteAreaOn=false;

    std::time_t time=std::time(nullptr); std::tm ltime=*std::localtime(&time);
    std::stringstream ifolder;
    ifolder<<lastFolder;
    ifolder<<std::put_time(&ltime, "%Y-%m-%d-%H-%M-%S");
    std::string saveFolderName=QFileDialog::getSaveFileName(this, tr("Select Folder for Saving Calibration Data"),QString::fromStdString(ifolder.str()),tr("Folders")).toStdString();
    if(!saveFolderName.empty()){
        lastFolder=saveFolderName.substr(0,saveFolderName.find_last_of("/")+1);
        saveFolderName+="/";
        cv::utils::fs::createDirectory(saveFolderName);
        cv::utils::fs::createDirectory(util::toString(saveFolderName,"fullScans/"));
        WCFArray(saveFolderName);
    }

    btnWriteCalib->setText("Run calibration");
    btnWriteCalib->setEnabled((multiarrayN->val<=scheduledPos.size() || multiarrayN->val==1));
}
void pgCalib::onOverlappingCalib(){
    if(overlappingCalib->text()=="Abort calibration"){
        overlappingCalib->setEnabled(false);
        wcabort=true;
        return;
    }
    if(!go.pRPTY->connected) {QMessageBox::critical(this, "Error", "Error: Red Pitaya not Connected"); return;}

    overlappingCalib->setText("Abort calibration");
    wcabort=false;

    WCFArray(ovrData.folder, true);

    overlappingCalib->setText("Overlapping run");
    updateOverlappingCalibEnabled();
}
void pgCalib::onSchedule(){
    while(go.pRPTY->getMotionSetting("",CTRL::mst_position_update_pending)!=0) QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
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
    ovrData.success=false;
    updateOverlappingCalibEnabled();
}
void pgCalib::updateOverlappingCalibEnabled(){
    overlappingCalib->setEnabled(ovrData.success && ovrData.multiarrayN==multiarrayN->val && ovrData.selArraySpacing==selArraySpacing->val && ovrData.selArrayXsize==selArrayXsize->val
            && ovrData.selArrayYsize==selArrayYsize->val && ovrData.transpose==transposeMat->val && ovrData.pos.size()==ovrData.multiarrayN);
    ovl_xofs->setEnabled(overlappingCalib->isEnabled());
    ovl_yofs->setEnabled(overlappingCalib->isEnabled());
}
void pgCalib::WCFArray(std::string folder, bool isOverlap){
    ovrData.success=false;
    if(!isOverlap){
        updateOverlappingCalibEnabled();
        ovrData.folder=folder;
        ovrData.multiarrayN=multiarrayN->val;
        ovrData.selArraySpacing=selArraySpacing->val;
        ovrData.selArrayXsize=selArrayXsize->val;
        ovrData.selArrayYsize=selArrayYsize->val;
        ovrData.transpose=transposeMat->val;
        ovrData.ovrIter=0;
        ovrData.pos.clear();
    }
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
    std::vector<double> arrayPla; arrayPla.reserve(arraySizePla);
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
        return;
    }

    if(!isOverlap){
        std::ofstream setFile(util::toString(folder,"/main-settings.txt"));     //this file contains some settings:
        setFile <<"#Objective_displacement_X(mm) Objective_displacement_Y(mm) Objective_displacement_Z(mm) MirauXYmmppx(mm/px)\n";
        setFile << std::fixed << std::setprecision(6);
        setFile <<pgMGUI->objectiveDisplacementX<<" "<<pgMGUI->objectiveDisplacementY<<" "<<pgMGUI->objectiveDisplacementZ<<" ";
        setFile << std::defaultfloat <<pgMGUI->getNmPPx()/1000000<<"\n";
        setFile.close();
    }else{
        std::ofstream setFile(util::toString(folder,"/offset-ovr",ovrData.ovrIter,".txt"));
        setFile <<"#X_offset(um) Y_offset(um) #Offsets are relative to the first calib run!#\n";
        setFile <<ovl_xofs->val<<" "<<ovl_yofs->val<<"\n";
        setFile.close();
    }
    if(multiarrayN->val>1 && arrayPla.size()>1){
        std::ofstream wfile(util::toString(folder,"/Plateau.txt"));
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
                std::ofstream wfile(isOverlap?util::toString(folder,"/", names[k],"-ovr",ovrData.ovrIter,".txt"):util::toString(folder,"/", names[k],".txt"), std::ofstream::out | std::ofstream::app);
                for(int i=0;i!=WArray.cols; i++){
                    for(int j=0;j!=WArray.rows; j++)
                        wfile<<WArray.at<cv::Vec2d>(j,i)[k]<<" ";
                    wfile<<"\n";
                }
                wfile<<"\n";
                wfile.close();
            }
        }
        if(WCFArrayOne(WArray, arrayPla.size()==1?arrayPla[0]:arrayPla[n], ROI, sROI, folder,n,isOverlap)){
            QMessageBox::critical(gui_settings, "Error", "Calibration failed/aborted. Measurements up to this point were saved.");
            report->setText(QString::fromStdString(util::toString("Aborted at ",n,"/",multiarrayN->val)));
            return;
        }
        if(transposeMat->val)
            cv::transpose(WArray,WArray);

    }
    report->setText(QString::fromStdString(util::toString("Done ",multiarrayN->val,"/",multiarrayN->val)));
    ovrData.success=true;
    if(isOverlap)ovrData.ovrIter++;
    updateOverlappingCalibEnabled();
}
bool pgCalib::WCFArrayOne(cv::Mat WArray, double plateau, cv::Rect ROI, cv::Rect sROI, std::string folder, unsigned n, bool isOverlap){
    if(!isOverlap){
        if(pgFGUI->doRefocus(true, ROI, maxRedoRefocusTries)) return true;

        // take pos for reruns
        while(go.pRPTY->getMotionSetting("",CTRL::mst_position_update_pending)!=0) QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
        ovrData.pos.emplace_back();
        pgMGUI->getPos(&ovrData.pos.back().x, &ovrData.pos.back().y, &ovrData.pos.back().z);
    }else{
        pgMGUI->move(ovrData.pos[n].x,ovrData.pos[n].y,ovrData.pos[n].z,true);
        pgMGUI->move(ovl_xofs->val/1000,ovl_yofs->val/1000,0);  // separate relative move so that it corrects Z, if calibrated to do so
    }

    varShareClient<pgScanGUI::scanRes>* scanRes=pgSGUI->result.getClient();
    double xOfs=((WArray.cols-1)*selArraySpacing->val)/2000;         //in mm
    double yOfs=((WArray.rows-1)*selArraySpacing->val)/2000;

    const pgScanGUI::scanRes* res;
    if(!isOverlap){ // no need to scan before for overlaps, as the before is the previous run's after; also no plateau
        if(plateau!=0){
            cv::Mat mplateau(WArray.rows+1, WArray.cols+1, CV_32F, cv::Scalar(plateau));
            if(wcabort){delete scanRes;return true;}        //abort
            if(pgWr->writeMat(&mplateau, 0, selArraySpacing->val/1000)){delete scanRes;return true;}    //abort/fail
        }

        if(wcabort){delete scanRes;return true;}        //abort
        if(pgSGUI->doNRounds((int)selArrayOneScanN->val, ROI, maxRedoScanTries,0,saveRF->val?1:0)) return true;
        if(wcabort){delete scanRes;return true;}        //abort

        res=scanRes->get();
        pgScanGUI::saveScan(res, util::toString(folder,"/fullScans/",n,"-before"), false, true, saveRF->val?1:0);

        for(int j=0;j!=WArray.rows; j++) for(int i=0;i!=WArray.cols; i++){   // separate them into individual scans
            cv::utils::fs::createDirectory(util::toString(folder,"/",n*WArray.cols*WArray.rows+i+j*WArray.cols));
            sROI.x=pgMGUI->mm2px(i*selArraySpacing->val/1000,0);
            sROI.y=pgMGUI->mm2px(j*selArraySpacing->val/1000,0);
            pgScanGUI::saveScan(res, sROI, util::toString(folder,"/",n*WArray.cols*WArray.rows+i+j*WArray.cols,"/before"), true, saveRF->val?1:0);
        }
    }

    {std::lock_guard<std::mutex>lock(pgSGUI->MLP._lock_proc);
        pgMGUI->chooseObj(false);
        pgMGUI->move(-xOfs,yOfs,0);
        CTRL::CO CO(go.pRPTY);
        CO.clear(true);
        for(int j=0;j!=WArray.rows; j++){
            for(int i=0;i!=WArray.cols; i++){
                if(wcabort){delete scanRes;return true;}        //abort

                pgMGUI->corCOMove(CO,0,0,WArray.at<cv::Vec2d>(j,i)[1]/1000);
                CO.addHold("X",CTRL::he_motion_ontarget);
                CO.addHold("Y",CTRL::he_motion_ontarget);
                CO.addHold("Z",CTRL::he_motion_ontarget);
                CO.pulseGPIO("wrLaser",WArray.at<cv::Vec2d>(j,i)[0]/1000);
                pgMGUI->corCOMove(CO,0,0,-WArray.at<cv::Vec2d>(j,i)[1]/1000);
                std::ofstream setFile(util::toString(folder,"/",n*WArray.cols*WArray.rows+i+j*WArray.cols,isOverlap?util::toString("/settings-ovr",ovrData.ovrIter,".txt"):"/settings.txt"));            //this file contains some settings:
                setFile <<"#Duration(ms) Focus(um)";
                setFile <<" Plateau(nm)";
                setFile <<"\n"<<WArray.at<cv::Vec2d>(j,i)[0]<<" "<<WArray.at<cv::Vec2d>(j,i)[1];
                setFile <<" "<<plateau;
                setFile <<"\n";
                setFile.close();

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

    if(wcabort){delete scanRes;return true;}        //abort
    if(pgSGUI->doNRounds((int)selArrayOneScanN->val, ROI, maxRedoScanTries,0,saveRF->val?1:0)) return true;
    if(wcabort){delete scanRes;return true;}        //abort
    res=scanRes->get();
    pgScanGUI::saveScan(res, util::toString(folder,"/fullScans/",n,isOverlap?util::toString("-after-ovr",ovrData.ovrIter):"-after"), false, true, saveRF->val?1:0);

    for(int j=0;j!=WArray.rows; j++) for(int i=0;i!=WArray.cols; i++){   // separate them into individual scans
        sROI.x=pgMGUI->mm2px(i*selArraySpacing->val/1000,0);
        sROI.y=pgMGUI->mm2px(j*selArraySpacing->val/1000,0);
        pgScanGUI::saveScan(res, sROI, util::toString(folder,"/",n*WArray.cols*WArray.rows+i+j*WArray.cols,isOverlap?util::toString("/after-ovr",ovrData.ovrIter):"/after"), true, saveRF->val?1:0);
    }

    delete scanRes;
    return false;
}

bool pgCalib::folderSort(measFolder i,measFolder j){
    size_t posi=i.folder.find_last_of("/");
    size_t posj=j.folder.find_last_of("/");
    return (std::stoi(i.folder.substr(posi+1,9))<std::stoi(j.folder.substr(posj+1,9)));
}
void pgCalib::writeProcessHeader(std::ofstream& wfile){
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
}

void pgCalib::onProcessFocusMes(){
    std::vector<std::filesystem::path> readFolders; //folders still yet to be checked
    std::vector<measFolder> measFolders;            //folders that contain the expected measurement files

    std::string root=QFileDialog::getExistingDirectory(this, "Select Folder Containing Measurements. It will be Searched Recursively.", QString::fromStdString(lastFolder)).toStdString();
    if(root.empty()) return;
    readFolders.push_back(std::filesystem::path(root));
    lastFolder=readFolders.back().parent_path().string()+"/";
    while(!readFolders.empty()){
        std::vector<std::filesystem::directory_entry> files;
        auto current=readFolders.back();
        for(auto const& dir_entry: std::filesystem::directory_iterator{current}) files.push_back(dir_entry);
        readFolders.pop_back();

        bool dirHasMes[3]{false,false,false};
        unsigned overlapping=0;
        for(auto& path: files){
            if(path.is_directory()) readFolders.push_back(path);
            else if(path.path().filename().string()=="settings.txt") dirHasMes[0]=true;
            else if(path.path().filename().string()=="before.pfm") dirHasMes[1]=true;
            else if(path.path().filename().string()=="after.pfm") dirHasMes[2]=true;
            else if(path.path().filename().string()==util::toString("after-ovr",overlapping,".pfm")) overlapping++;
            auto it=path.path().filename().string().find("after-ovr");
            unsigned tmp{0};
            if(it!=std::string::npos){
                tmp=std::stoi(path.path().filename().string().substr(it+9));
                if(tmp>overlapping)overlapping=tmp;
            }
        }
        if(dirHasMes[0]&dirHasMes[1]&dirHasMes[2]) measFolders.push_back({current.string(),overlapping});
    }
    std::lock_guard<std::mutex>lock(pgSGUI->MLP._lock_comp);
    pgSGUI->MLP.progress_comp=0;

    //sort folders by number
    std::sort(measFolders.begin(), measFolders.end(), pgCalib::folderSort);
    unsigned maxovl=0;
    for(auto& fldr: measFolders) if(fldr.overlaps>maxovl) maxovl=fldr.overlaps;

    std::vector<double>prepeaks(measFolders.size(),0);
    for(unsigned i=0;i!=maxovl+1;i++){
        std::string saveName=root+((i==0)?"/proc.txt":util::toString("/proc-ovr",i-1,".txt"));
        std::ofstream wfile(saveName);

        writeProcessHeader(wfile);

        std::vector<std::string> results;
        std::atomic<unsigned> completed=0;
        int n=0;
        for(auto& fldr:measFolders){
            results.emplace_back();
            results.back().reserve(26*100);
        }

        double prepeakxy[2]{0,0};
        if(i>0){
            std::ifstream ifs(util::toString(root,"/offset-ovr",i-1,".txt"));
            ifs.ignore(std::numeric_limits<std::streamsize>::max(),'\n');       // ignore header
            ifs>>prepeakxy[0];
            ifs>>prepeakxy[1];
            ifs.close();
        }

        for(auto& fldr:measFolders) if(fldr.overlaps>=i){
            go.OCL_threadpool.doJob(std::bind(&pgCalib::prepCalcParameters, this, fldr, &results[n], &completed, prepeakxy[0], prepeakxy[1], i, &prepeaks[n]));
            n++;
        }
        while(completed!=n){
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            pgSGUI->MLP.progress_comp=100./n*completed/(maxovl+1)+100/(maxovl+1)*i;
        }
        for(int i=0;i!=n;i++) wfile<<results[i];
        pgSGUI->MLP.progress_comp=100./(maxovl+1)*(i+1);
        wfile.close();
    }
}



int pgCalib::gauss2De_f (const gsl_vector* pars, void* data, gsl_vector* f){
    size_t n=((struct fit_data*)data)->n;
    size_t nPeaks=((struct fit_data*)data)->nPeaks;
    double* x=((struct fit_data*)data)->x;
    double* y=((struct fit_data*)data)->y;
    double* h=((struct fit_data*)data)->h;

    size_t v=0;
    double x0=gsl_vector_get(pars, v++);
    double y0=gsl_vector_get(pars, v++);
    double a0=gsl_vector_get(pars, v++);    // background height
    double ar=gsl_vector_get(pars, v++);    // aspect ratio wx/wy
    double an=gsl_vector_get(pars, v++);    // angle x/y

    double a[nPeaks];
    double w[nPeaks];

    double A[nPeaks];
    double B[nPeaks];
    double C[nPeaks];
    for(size_t i=0;i!=nPeaks;i++){
        w[i]=gsl_vector_get(pars, v++);     // width 1/e = wx
        a[i]=gsl_vector_get(pars, v++);     // height

        A[i]=pow(cos(an),2)/2/pow(w[i],2)+pow(sin(an),2)/2/pow(w[i]/ar,2);
        B[i]=sin(2*an)/2/pow(w[i],2)-sin(2*an)/2/pow(w[i]/ar,2);
        C[i]=pow(sin(an),2)/2/pow(w[i],2)+pow(cos(an),2)/2/pow(w[i]/ar,2);
    }

    for (size_t i=0; i!=n; i++){
        double model=a0;
        for(size_t j=0;j!=nPeaks;j++)
            model+=abs(a[j])*exp(-A[j]*pow(x[i]-x0,2)-B[j]*(x[i]-x0)*(y[i]-y0)-C[j]*pow(y[i]-y0,2));
        gsl_vector_set(f, i, model-h[i]);
    }
    return GSL_SUCCESS;
}

void pgCalib::prepCalcParameters(measFolder fldr, std::string* output, std::atomic<unsigned>* completed, double prepeakXofs, double prepeakYofs, unsigned overlap, double* prepeak){
    double focus{0};
    double duration{0};
    double plateau{0};

    std::ifstream ifs(util::toString((overlap==0)?util::toString(fldr.folder,"/settings.txt"):util::toString(fldr.folder,"/settings-ovr",overlap-1,".txt")));
    ifs.ignore(std::numeric_limits<std::streamsize>::max(),'\n');       // ignore header
    ifs>>duration;
    ifs>>focus;
    ifs>>plateau;
    ifs.close();

    pgScanGUI::scanRes scanBefore, scanAfter;
    if(!pgScanGUI::loadScan(&scanBefore, util::toString(fldr.folder,"/before.pfm"))) {(*completed)++;return;}
    if(!pgScanGUI::loadScan(&scanAfter, (overlap==0)?util::toString(fldr.folder,"/after.pfm"):util::toString(fldr.folder,"/after-ovr",overlap-1,".pfm")))
        return;

    pgScanGUI::scanRes scanDif=pgSGUI->difScans(&scanBefore, &scanAfter);
    pgScanGUI::saveScan(&scanDif, util::toString(fldr.folder,(overlap==0)?"/scandif.pfm":util::toString(fldr.folder,"/scandif-ovr",overlap-1,".pfm")));

    if(!scanAfter.refl.empty() && !scanBefore.refl.empty())
        cv::subtract(scanAfter.refl,scanBefore.refl,scanDif.refl,cv::noArray(),CV_32F);     // we put this refl in float format only temporarily in this case, for conveninece
    scanDif.tiltCor[0]=scanBefore.tiltCor[0];   // for convenience
    scanDif.tiltCor[1]=scanBefore.tiltCor[1];

    calcParameters(scanDif, output, prepeakXofs, prepeakYofs, prepeak, focus, duration, plateau);

    (*completed)++;
    return;
}

void pgCalib::calcParameters(pgScanGUI::scanRes& scanDif, std::string* output, double prepeakXofs, double prepeakYofs , double* prepeak, double focus, double duration, double plateau, cv::Mat* output_mat){
    double _prepeak{0};
    if(prepeak!=nullptr){
        _prepeak=*prepeak;
        *prepeak=0;
    }
    // shared pars: x0, y0, a0 , ar (aspect ratio wx/wy), an (angle x/y)
    // pars per each peak: w (width 1/e), a (height)
    const size_t peakNum=nPeakFit->val;
    const size_t parN=5+2*peakNum;
    double initval[parN];// = { 0, 4000/scanDif.XYnmppx, 4000/scanDif.XYnmppx, 0.01, 0};
    size_t v=0;
    initval[v++]=scanDif.depth.cols/2.; // x0
    initval[v++]=scanDif.depth.rows/2.; // y0
    initval[v++]=scanDif.min;           // a0 (background height)
    initval[v++]=1;                     // ar (aspect ratio wx/wy)
    initval[v++]=0.01;                  // an (angle x/y)
    for(size_t i=0;i!=peakNum;i++){
        initval[v++]=4000/scanDif.XYnmppx;              // w=wx (width 1/e)
        initval[v++]=(scanDif.max-scanDif.min)/peakNum; // a (height)
    }
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
    if(ptN<parN) return;

    struct fit_data data{ptN,peakNum,x,y,h};
    gsl_multifit_nlinear_fdf fdf{gauss2De_f,NULL,NULL,ptN,parN,&data,0,0,0};
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

    if(output_mat!=nullptr){
        *output_mat=cv::Mat::zeros(scanDif.depth.rows, scanDif.depth.cols, scanDif.depth.type());
        int n=0;
        for(int i=0;i!=scanDif.depth.cols;i++) for(int j=0;j!=scanDif.depth.rows;j++){
            if(scanDif.mask.at<uchar>(j,i)==0){
                output_mat->at<float>(j,i)=gsl_vector_get(workspace->f, n);
                n++;
                if(n>ptN) throw std::out_of_range("FIXME: in pgCalib::calcParameters when calculation output_mat: too many points.\n");
            }else{
                output_mat->at<float>(j,i)=0;
            }
        }
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
    if(!scanDif.refl.empty()) peakRefl=cv::mean(scanDif.refl)[0];

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
    double tiltX=scanDif.tiltCor[0]/scanDif.XYnmppx;    // these tilts are actually from scanBefore
    double tiltY=scanDif.tiltCor[1]/scanDif.XYnmppx;

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
        _prepeak," ",                   // 22: prepeak(nm)
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
    if(prepeak!=nullptr)*prepeak=res[2];

    return;
}


void pgCalib::onChangeDrawWriteAreaOn(bool status){
    drawWriteAreaOn=status&(multiarrayN->val==1)&(btnWriteCalib->text()!="Abort calibration");
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
    std::vector<std::filesystem::path> readFolders;
    std::vector<std::filesystem::path> measFiles;

    std::string path=QFileDialog::getExistingDirectory(this, "Select Folder Containing Processed Measurements. It will be Searched Recursively for proc.txt files.", QString::fromStdString(lastFolder)).toStdString();
    if(path.empty()) return;
    readFolders.push_back(std::filesystem::path(path));
    lastFolder=readFolders.back().parent_path().string()+"/";
    while(!readFolders.empty()){
        std::vector<std::filesystem::directory_entry> files;
        for(auto const& dir_entry: std::filesystem::directory_iterator{readFolders.back()}) files.push_back(dir_entry);
        readFolders.pop_back();
        for(auto& path: files){
            if(path.is_directory()) readFolders.push_back(path);
            else if(path.path().stem().string().find("proc")!=std::string::npos && path.path().extension().string()==".txt") measFiles.push_back(path);
        }
    }

    const size_t pos[]={1,12,3,27,2,8,9};     // indices in data(starting from 1): valid, duration, height, height_err, focus, xofs, yofs
    const size_t maxpos=*std::max_element(pos,pos+sizeof(pos)/sizeof(size_t));
    for(size_t i:{0,1,2}) focus[i]=0;
    for(auto& item:measFiles){
        std::ifstream ifs(item);
        for(std::string line; std::getline(ifs, line);){
            if(line[0]=='#') continue;
            size_t sx=0, sxi;
            bool valid;
            double _duration, _height, _height_err, _focus[3];
            for(size_t i=1;i<=maxpos;i++){
                double tmp=std::stod(line.substr(sx),&sxi);
                sx+=sxi;
                if(i==pos[0]) valid=(tmp==1);
                else if(i==pos[1]) _duration=tmp;
                else if(i==pos[2]) _height=tmp;
                else if(i==pos[3]) _height_err=tmp;
                else if(i==pos[4]) _focus[0]=tmp;
                else if(i==pos[5]) _focus[1]=tmp;
                else if(i==pos[6]) _focus[2]=tmp;
            }
            if(valid){
                fpLoadedData.push_back({_duration,_height,_height_err});
                for(size_t j:{0,1,2}) focus[j]+=_focus[j];
            }
        }
        ifs.close();
    }

    if(fpLoadedData.empty()) return;
    for(size_t i:{0,1,2}) focus[i]/=fpLoadedData.size();

    fpLoadedDataSorted=false;
    fitAndPlot->setEnabled(true);
    fpClear->setEnabled(true);
    QString list=fpList->text();
    for(auto& item:measFiles) {list+=item.parent_path().filename().c_str(); list+="/"; list+=item.filename().c_str(); list+="\n";}
    fpList->setText(list);
    fpList->setVisible(true);

    if(cpwin!=nullptr) if(cpwin->isVisible()){
        onFitAndPlot();
    }
}
void pgCalib::onfpClear(){
    fpLoadedData.clear();
    fitAndPlot->setEnabled(false);
    fpClear->setEnabled(false);
    fpList->setText("");
    fpList->setVisible(false);
    fpApply->setEnabled(false);
}


int pgCalib::bspline_f(const gsl_vector* pars, void* data, gsl_vector* f){
    size_t mesN=((struct fit_data_och*)data)->n;
    double* x=((struct fit_data_och*)data)->x;
    double* y=((struct fit_data_och*)data)->y;
    double* w=((struct fit_data_och*)data)->w;
    double* _breakpts=((struct fit_data_och*)data)->breakpts;
    double* _coefs=((struct fit_data_och*)data)->coefs;
    double* _covmat=((struct fit_data_och*)data)->covmat;
    size_t ncoeffs=((struct fit_data_och*)data)->ncoeffs;
    size_t nbreak=ncoeffs-2;

    gsl_bspline_workspace *bsplws=gsl_bspline_alloc(4, nbreak); // cubis bspline
    gsl_vector *basisfun=gsl_vector_alloc(ncoeffs);
    gsl_vector coefs=gsl_vector_view_array(_coefs,ncoeffs).vector;
    gsl_vector weights=gsl_vector_view_array(w,mesN).vector;
    gsl_vector yvec=gsl_vector_view_array(y,mesN).vector;
    gsl_matrix *predVarMat=gsl_matrix_alloc(mesN, ncoeffs);
    gsl_matrix covmat=gsl_matrix_view_array(_covmat,ncoeffs,ncoeffs).matrix;
    gsl_multifit_linear_workspace *mfitws=gsl_multifit_linear_alloc(mesN, ncoeffs);
    gsl_vector breakpts=gsl_vector_view_array(_breakpts,nbreak).vector;
    double min=gsl_vector_min(pars);
    double max=gsl_vector_max(pars);
    double minc=(min<x[0])?(min-x[0]):0;
    double maxc=((max+minc)/x[mesN-1]>1)?(x[mesN-1]/(max+minc)):1;
    for(size_t i=0;i!=nbreak;i++){
        gsl_vector_set(&breakpts, i, maxc*(gsl_vector_get(pars, i)-minc));
    }
    gsl_sort_vector(&breakpts);
    if(gsl_vector_get(&breakpts, 0)>x[0]) gsl_vector_set(&breakpts,0,x[0]);
    if(gsl_vector_get(&breakpts, nbreak-1)<x[mesN-1]) gsl_vector_set(&breakpts,nbreak-1,x[mesN-1]);
    gsl_bspline_knots(&breakpts,bsplws);
    for (size_t i=0;i!=mesN;i++){
        gsl_bspline_eval(x[i], basisfun, bsplws);   // compute bspline coefs
        for (size_t j=0;j!=ncoeffs;j++)
            gsl_matrix_set(predVarMat, i, j, gsl_vector_get(basisfun, j));  //  construct the fit matrix
    }

    double ybs, yerr, ign;
    gsl_multifit_wlinear(predVarMat, &weights, &yvec, &coefs, &covmat, &ign, mfitws); // fit
    for (size_t i=0; i!=coefs.size; i++){
        if(gsl_vector_get(&coefs, i)<0) gsl_vector_set(&coefs,i,0); // coefs need to be positive
        if(i!=0) if(gsl_vector_get(&coefs, i)<=gsl_vector_get(&coefs, i-1))
            gsl_vector_set(&coefs,i,gsl_vector_get(&coefs, i-1)+1e-9); // positive slope, otherwise solutions not unique
    }

    for (size_t i=0; i!=mesN; i++){
        gsl_bspline_eval(x[i], basisfun, bsplws);
        gsl_multifit_linear_est(basisfun, &coefs, &covmat, &ybs, &yerr);
        if(f!=nullptr) gsl_vector_set(f, i, ybs-y[i]);
    }

    gsl_bspline_free(bsplws);
    gsl_vector_free(basisfun);
    gsl_matrix_free(predVarMat);
    gsl_multifit_linear_free(mfitws);

    return GSL_SUCCESS;
}

void pgCalib::onFitAndPlot(){
    fpApply->setEnabled(false);
    bsplbreakpts.clear();
    bsplcoefs.clear();
    const size_t ncoeffs=static_cast<size_t>(nBSplineCoef->val);
    const size_t nbreak = ncoeffs-2;    // must be at least 2
    if(fpLoadedData.size()<ncoeffs) return;
    std::vector<double> duration, height, height_err, weight;
    if(!fpLoadedDataSorted){    // sort by height
        fpLoadedDataSorted=true;
        std::sort(fpLoadedData.begin(), fpLoadedData.end(), [](durhe_data i,durhe_data j){return (i.height<j.height);});
    }
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
    sdata.setYErr(height_err);
    sdata.setColor(cv::Scalar(255, 0, 0));//red
    sdata.setName("Data");
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    duration.insert(duration.begin(),0);      // add a zero point for fitting
    height.insert(height.begin(),0);
    height_err.insert(height_err.begin(),1e-5);

    // NOTE we for bsplines we use height as X and duration as Y, since that will later allow us to determine the required pulse length for desired height

    const double min=0;
    const double max=(1./upperLim->maximum()*upperLim->value())*(*std::max_element(height.begin(),height.end()));

    while(height.back()>max && height.size()>ncoeffs){
        duration.pop_back();
        height.pop_back();
        height_err.pop_back();
    }

    const size_t nMes=duration.size();
    gsl_vector *basisfun=gsl_vector_alloc(ncoeffs);
    bsplcoefs=std::vector<double>(ncoeffs,0);
    gsl_vector coefs=gsl_vector_view_array(bsplcoefs.data(),ncoeffs).vector;
    bsplcov=std::vector<double>(ncoeffs*ncoeffs,0);
    gsl_matrix covmat=gsl_matrix_view_array(bsplcov.data(), ncoeffs, ncoeffs).matrix;
    bsplbreakpts=std::vector<double>(nbreak,0);
    gsl_vector gbreakpts=gsl_vector_view_array(bsplbreakpts.data(),nbreak).vector;
    cv::divide(1.,height_err,weight,CV_64F);
    cv::divide(weight,height_err,weight,CV_64F);
    fit_data_och data{
        nMes,
        ncoeffs,
        height.data(),
        duration.data(),
        weight.data(),
        bsplbreakpts.data(),
        bsplcoefs.data(),
        bsplcov.data()
    };
    gsl_vector *pars=gsl_vector_alloc(nbreak);
    for(size_t i=0;i!=nbreak;i++) gsl_vector_set(pars,i,min+i*(max-min)/(nbreak-1));
    if(nbreak==2 || !optimizeBP->val){
        bspline_f(pars, &data);
    }else{
        gsl_multifit_nlinear_fdf fdf{bspline_f,NULL,NULL,nMes,nbreak,&data,0,0,0};
        gsl_multifit_nlinear_parameters fdf_params=gsl_multifit_nlinear_default_parameters();
        gsl_multifit_nlinear_workspace* workspace=gsl_multifit_nlinear_alloc(gsl_multifit_nlinear_trust, &fdf_params, nMes, nbreak);

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

    gsl_bspline_workspace *bsplws=gsl_bspline_alloc(4, nbreak); // cubic bspline
    gsl_bspline_knots(&gbreakpts,bsplws);
    std::vector<double> fduration, fheight;
    double nplot=1000;
    double step=(bsplbreakpts.back()-bsplbreakpts.front())/(nplot-1);
    double yerr;
    for(int i=0;i!=nplot;i++){
        fheight.push_back(min+i*step);
        fduration.emplace_back();
        gsl_bspline_eval(fheight.back(), basisfun, bsplws);
        gsl_multifit_linear_est(basisfun, &coefs, &covmat, &fduration.back(), &yerr);
    }
    gsl_vector_free(basisfun);
    gsl_bspline_free(bsplws);

    if(showBP->val){
        auto& sbp=cpwin->axes.create<CvPlot::Series>(std::vector(nbreak,0), bsplbreakpts, "og");
        sbp.setColor(cv::Scalar(128, 128, 0));//olive
        sbp.setName("Breakpoints");
    }
    auto& sfit=cpwin->axes.create<CvPlot::Series>(fduration, fheight, "-r");
    sfit.setColor(cv::Scalar(0, 0, 255));//blue
    sfit.setName("Spline fit");

    linA=(fduration[nplot-1]-fduration[nplot-2])/(fheight[nplot-1]-fheight[nplot-2]);
    linX=fduration.back()-linA*fheight.back();
    std::vector<double> linduration, linheight;
    nplot=100;
    for(int i=0;i!=nplot;i++){
        linheight.push_back(fheight.back()+i*step);
        linduration.push_back(linA*linheight.back()+linX);
    }
    auto& lfit=cpwin->axes.create<CvPlot::Series>(linduration, linheight, "-");
    lfit.setColor(cv::Scalar(0, 0, 128));//navy
    lfit.setName("Linear tail");

    cpwin->redraw();
    cpwin->show();
    cpwin->activateWindow();    // takes focus
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    fpApply->setEnabled(true);
}
void pgCalib::onNBSplineCoefChanged(){
    if(fpLoadedData.empty()) return;
    if(cpwin!=nullptr) if(cpwin->isVisible()){
        onFitAndPlot();
    }
}
void pgCalib::onApply(){
    applyMenu->clear();
    for(auto& set:pgWr->settingWdg) applyMenu->addAction(QString::fromStdString(util::toString("Apply to write setting:",set->name)));
    applyMenu->popup(QCursor::pos());
}
void pgCalib::onApplyMenuTriggered(QAction* action){
    for(size_t i=0;i!=applyMenu->actions().size();i++){
        if(applyMenu->actions()[i]==action){
            pgWr->settingWdg[i]->bsplbreakpts=bsplbreakpts;
            pgWr->settingWdg[i]->bsplcoefs=bsplcoefs;
            pgWr->settingWdg[i]->bsplcov=bsplcov;
            pgWr->settingWdg[i]->constA->setValue(linA);
            pgWr->settingWdg[i]->constC->setValue(linX);
            pgWr->settingWdg[i]->usingBSpline->setEnabled(true);
            pgWr->settingWdg[i]->usingBSpline->setChecked(true);
            pgWr->settingWdg[i]->focus->setValue(focus[0]);
            pgWr->settingWdg[i]->focusXcor->setValue(focus[1]);
            pgWr->settingWdg[i]->focusYcor->setValue(focus[2]);
            pgWr->settingWdg[i]->p_ready=false;
            return;
        }
    }
}
