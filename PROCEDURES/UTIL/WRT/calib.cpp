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
    hc_sett=new hidCon(new QLabel("Peak - Run calibration"));
    hc_proc=new hidCon(new QLabel("Peak - Process calibration data"));
    hc_sett_pl=new hidCon(new QLabel("Plateau - Run Calibration"));
    hc_proc_pl=new hidCon(new QLabel("Plateau - Process calibration data"));
    hc_proc_cf=new hidCon(new QLabel("Calibration curve fitting (duration-height)"));
    hc_sett->linkTo(hc_proc);
    hc_sett_pl->linkTo(hc_proc);
    hc_proc_pl->linkTo(hc_proc);
    hc_proc_cf->linkTo(hc_proc);
    slayout->addWidget(hc_sett);
    slayout->addWidget(hc_proc);
    slayout->addWidget(hc_sett_pl);
    slayout->addWidget(hc_proc_pl);
    slayout->addWidget(hc_proc_cf);

    selArrayXsize=new val_selector(10, "Array Size X", 1, 1000, 0);
    conf["selArrayXsize"]=selArrayXsize;
    connect(selArrayXsize, SIGNAL(changed()), this, SLOT(updateOverlappingCalibEnabled()));
    selArrayYsize=new val_selector(10, "Array Size Y", 1, 1000, 0);
    conf["selArrayYsize"]=selArrayYsize;
    connect(selArrayYsize, SIGNAL(changed()), this, SLOT(updateOverlappingCalibEnabled()));
    hc_sett->addWidget(new twid{selArrayXsize,selArrayYsize});
    selArraySpacing=new val_selector(5, "Array Spacing", 0.001, 100, 3, 0, {"um"});
    conf["selArraySpacing"]=selArraySpacing;
    connect(selArraySpacing, SIGNAL(changed()), this, SLOT(updateOverlappingCalibEnabled()));
    hc_sett->addWidget(selArraySpacing);
    selArrayType=new smp_selector("Variable Parameters (X-Y): ", 0, {"Duration(X,Y), no repeat","Focus(X,Y), no repeat","Duration(X)-Focus(Y)", "Duration(X,Y), repeat","Focus(X,Y), repeat", "none (for prepeak)"});
    connect(selArrayType, SIGNAL(changed(int)), this, SLOT(onSelArrayTypeChanged(int)));
    conf["selArrayType"]=selArrayType;
    hc_sett->addWidget(selArrayType);
    transposeMat=new checkbox_gs(false,"Transpose matrix.");
    conf["transposeMat"]=transposeMat;
    connect(transposeMat, SIGNAL(changed()), this, SLOT(updateOverlappingCalibEnabled()));
    hc_sett->addWidget(transposeMat);
    hc_sett->addWidget(new QLabel("The Variable Parameters Will be Within the Specified Range."));
    selArrayDurA=new val_selector(1, "Duration", 0.0001, 1000, 4, 0, {"ms"});
    conf["selArrayDurA"]=selArrayDurA;
    hc_sett->addWidget(selArrayDurA);
    selArrayDurB=new val_selector(1, "Duration upper limit", 0.0001, 1000, 4, 0, {"ms"});
    conf["selArrayDurB"]=selArrayDurB;
    hc_sett->addWidget(selArrayDurB);
    selArrayFocA=new val_selector(-1, "Focus", -1000, 1000, 3, 0, {"um"});
    conf["selArrayFocA"]=selArrayFocA;
    hc_sett->addWidget(selArrayFocA);
    selArrayFocB=new val_selector(1, "Focus upper limit", -1000, 1000, 3, 0, {"um"});
    conf["selArrayFocB"]=selArrayFocB;
    hc_sett->addWidget(selArrayFocB);
    selArrayOneScanN=new val_selector(10, "Take and average This Many Scans: ", 1, 1000, 0);
    conf["selArrayOneScanN"]=selArrayOneScanN;
    hc_sett->addWidget(selArrayOneScanN);
    selArrayRandomize=new checkbox_gs(false,"Randomize Value Order");
    conf["selArrayRandomize"]=selArrayRandomize;
    hc_sett->addWidget(selArrayRandomize);
    saveMats=new checkbox_gs(true,"Extra Save Mats Containing D,F for Convenience.");
    conf["saveMats"]=saveMats;
    hc_sett->addWidget(saveMats);
    saveRF=new checkbox_gs(true,"Also save reflectivity.");
    conf["saveRF"]=saveRF;
    hc_sett->addWidget(saveRF);
    hc_sett->addWidget(new hline);
    selPlateauA=new val_selector(0, "Plateau Height", 0, 1000, 3, 0, {"nm"});
    conf["selPlateauA"]=selPlateauA;
    hc_sett->addWidget(selPlateauA);
    selPlateauB=new val_selector(0, "Plateau upper limit", 0, 1000, 3, 0, {"nm"});
    conf["selPlateauB"]=selPlateauB;
    hc_sett->addWidget(selPlateauB);
    hc_sett->addWidget(new hline);
    selMultiArrayType=new smp_selector("Multiple runs variable parameter: ", 0, {"none","Focus","Duration", "Plateau"});
    connect(selMultiArrayType, SIGNAL(changed(int)), this, SLOT(onSelMultiArrayTypeChanged(int)));
    conf["selMultiArrayType"]=selMultiArrayType;
    hc_sett->addWidget(selMultiArrayType);
    multiarrayN=new val_selector(1, "N of runs:", 1, 10000, 0);
    connect(multiarrayN, SIGNAL(changed(double)), this, SLOT(onMultiarrayNChanged(double)));
    conf["multiarrayN"]=multiarrayN;
    connect(multiarrayN, SIGNAL(changed()), this, SLOT(updateOverlappingCalibEnabled()));
    hc_sett->addWidget(multiarrayN);
    hc_sett->addWidget(new hline);
    report=new QLabel("");
    hc_sett->addWidget(new QLabel("Calibration run:"));
    hc_sett->addWidget(new twid(scheduleMultiWrite,btnWriteCalib,report));
    hc_sett->addWidget(new twid(overlappingCalib,ovl_xofs,ovl_yofs));

    btnProcessFocusMes=new QPushButton("Select Folders to Process Focus Measurements");
    connect(btnProcessFocusMes, SIGNAL(released()), this, SLOT(onProcessFocusMes()));
    hc_proc->addWidget(new twid(btnProcessFocusMes));

    fitPar_directFWHMandHeight=new checkbox_gs(true,"Direct FWHM and height from measurement");
    fitPar_directFWHMandHeight->setToolTip("If false, these are determined from the fits.");
    conf["fitPar_directFWHMandHeight"]=fitPar_directFWHMandHeight;
    hc_proc->addWidget(fitPar_directFWHMandHeight);
    fitPar_nPeakGauss=new val_selector(1, "Fit Gaussian peak number", 0, 10, 0);
    conf["fitPar_nPeakGauss"]=fitPar_nPeakGauss;
    hc_proc->addWidget(fitPar_nPeakGauss);
    fitPar_nPeakLorentz=new val_selector(0, "Fit Lorentzian peak number", 0, 10, 0);
    conf["fitPar_nPeakLorentz"]=fitPar_nPeakLorentz;
    hc_proc->addWidget(fitPar_nPeakLorentz);
    fitPar_independentWidths=new checkbox_gs(true,"Independent peak widths (Gaussian)");
    fitPar_independentWidths->setToolTip("If false, the X and Y widths of all peaks follow the same ratio.");
    conf["fitPar_independentWidths"]=fitPar_independentWidths;
    hc_proc->addWidget(fitPar_independentWidths);
    fitPar_independentAngles=new checkbox_gs(false,"Independent peak angles (Gaussian)");
    conf["fitPar_independentAngles"]=fitPar_independentAngles;
    hc_proc->addWidget(fitPar_independentAngles);
    fitPar_independentCentres=new checkbox_gs(true,"Independent peak centres (Both)");
    conf["fitPar_independentCentres"]=fitPar_independentCentres;
    hc_proc->addWidget(fitPar_independentCentres);

    plNRuns=new val_selector(10, "NRuns: ", 1, 1000, 0);
    conf["plNRuns"]=plNRuns;
    hc_sett_pl->addWidget(plNRuns);
    plRadius=new val_selector(10, "Radius", 0, 1000, 3, 0, {"um"});
    conf["plRadius"]=plRadius;
    hc_sett_pl->addWidget(plRadius);
    plMargin=new val_selector(10, "Margin", 0, 1000, 3, 0, {"um"});
    conf["plMargin"]=plMargin;
    hc_sett_pl->addWidget(plMargin);
    plSpacing=new val_selector(1, "Point spacing", 0.001, 1000, 3, 0, {"um"});
    conf["plSpacing"]=plSpacing;
    hc_sett_pl->addWidget(plSpacing);
    plDurMin=new val_selector(1, "Duration ", 0.00001, 1000, 5, 0, {"ms"});
    plDurMax=new val_selector(1, " to ", 0.00001, 1000, 5, 0, {"ms"});
    plFoc=new val_selector(0, "Focus", -1000, 1000, 3, 0, {"um"});
    conf["plDurMin"]=plDurMin;
    conf["plDurMax"]=plDurMax;
    conf["plFoc"]=plFoc;;
    hc_sett_pl->addWidget(new twid(plDurMin,plDurMax));
    hc_sett_pl->addWidget(plFoc);
    randomizeOrder=new checkbox_gs(true,"Randomize order");
    conf["randomizeOrder"]=randomizeOrder;
    hc_sett_pl->addWidget(randomizeOrder);
    hc_sett_pl->addWidget(new QLabel("(Set to same value if constant)"));
    plSetFolder=new QPushButton("Prepare matrices and select output folder");
    connect(plSetFolder, SIGNAL(released()), this, SLOT(onPlSetFolder()));
    hc_sett_pl->addWidget(new twid(plSetFolder));
    plSchedule=new HQPushButton("Schedule ?/?");
    connect(plSchedule, SIGNAL(released()), this, SLOT(onPlSchedule()));
    connect(plSchedule, SIGNAL(changed(bool)), this, SLOT(onChangePlSchedule(bool)));
    hc_sett_pl->addWidget(new twid(plSchedule));
    plSchedule->setEnabled(false);

    fpLoad_pl=new QPushButton("Select Folders to process measurements");
    connect(fpLoad_pl, SIGNAL(released()), this, SLOT(onfpLoad_pl()));
    hc_proc_pl->addWidget(new twid(fpLoad_pl));

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
    hc_proc_cf->addWidget(new twid(fpLoad,fpClear,fitAndPlot,fpApply));
    hc_proc_cf->addWidget(new twid(nBSplineCoef,showBP,optimizeBP));
    hc_proc_cf->addWidget(new twid(new QLabel("Fit upper limit:"),upperLim));
    fpList=new QLabel("");
    fpList->setVisible(false);
    hc_proc_cf->addWidget(fpList);
}
pgCalib::~pgCalib(){
    if(cpwin!=nullptr) delete cpwin;
}

void pgCalib::onPlSetFolder(){
    // make array
    std::vector<double> varValues;  // in ms
    int nvals=plNRuns->val;
    if(nvals<=1) return;
    double min=plDurMin->val;
    double step=(plDurMax->val-min)/(nvals-1);
    for(int i=0;i<nvals;i++)
        varValues.push_back(min+step*i);
    if(randomizeOrder->val){
        std::mt19937 rnd(std::random_device{}());
        std::shuffle(varValues.begin(), varValues.end(), rnd);
    }

    // calculate cell size (max ni,nj that can be done in one write/scan)
    int ni,nj;
    int freepx[4]; // left,right,up,down

    int rows=go.pGCAM->iuScope->camRows;
    int cols=go.pGCAM->iuScope->camCols;
    freepx[0]=cols/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsX)-pgWr->getScanExtraBorderPx();
    freepx[1]=cols-pgWr->getScanExtraBorderPx()-freepx[0];
    freepx[2]=rows/2-pgMGUI->mm2px(pgBeAn->writeBeamCenterOfsY)-pgWr->getScanExtraBorderPx();
    freepx[3]=rows-pgWr->getScanExtraBorderPx()-freepx[2];
    double xysizeum=2*(plRadius->val+plMargin->val);
    int xysizepx=pgMGUI->mm2px(xysizeum*0.001);
    int xysizepxwr=xysizeum/plSpacing->val;
    cv::Mat src=cv::Mat::zeros(xysizepxwr,xysizepxwr,CV_32F);
    cv::circle(src, cv::Point(xysizepxwr/2,xysizepxwr/2), plRadius->val/plSpacing->val, 1, -1);
    ni=floor(2.0*std::min(freepx[0],freepx[1])/xysizepx);
    nj=floor(2.0*std::min(freepx[2],freepx[3])/xysizepx);
    //std::cerr<<"Will write matrices of "<<ni<<" x "<<nj<<"\n";
    //std::cerr<<"Single matrix size: "<<xysizepx<<" px^2\n";
    //std::cerr<<"Total matrix size: "<<ni*xysizepx<<" x "<<nj*xysizepx<<" px\n";
    //std::cerr<<"Image size: "<<cols<<" x "<<rows<<" px\n";

    if(ni<1 || nj<1) return;

    std::vector<std::vector<std::vector<double>>> values; // matrix,row,col
    for(;;){
        values.emplace_back();
        for(int j=0;j<nj;j++){
            values.back().emplace_back();
            for(int i=0;i<ni;i++){
                values.back().back().push_back(varValues.back());
                varValues.pop_back();
                if(varValues.empty()) goto out;
            }
        }
    }
    out:;

    plNTotal=values.size();
    plNScheduled=0;

    // construct matrices
    plMats.clear();
    plTexts.clear();
    cv::Mat tmp;
    for(auto& val:values){
        plMats.push_back(cv::Mat::zeros(xysizepxwr*val.size(),xysizepxwr*val.back().size(),CV_32F));
        plTexts.emplace_back();
        plTexts.back()+="#This is a calibration measurement!\n";
        plTexts.back()+="#Radius (um):\n";
        plTexts.back()+=std::to_string(plRadius->val);
        plTexts.back()+="\n#Margin (um):\n";
        plTexts.back()+=std::to_string(plMargin->val);
        plTexts.back()+="\n#Spacing (um):\n";
        plTexts.back()+=std::to_string(plSpacing->val);
        plTexts.back()+="\n#Focus (mm):\n";
        plTexts.back()+=std::to_string(plFoc->val/1000);
        plTexts.back()+="\n#Durations (ms):\n";
        for(size_t j=0;j<val.size();j++){
            for(size_t i=0;i<val.back().size();i++){
                tmp=src*val[j][i];
                tmp.copyTo(plMats.back()(cv::Rect(i*xysizepxwr, j*xysizepxwr, xysizepxwr, xysizepxwr)));
                if(i!=0) plTexts.back()+=" ";
                plTexts.back()+=std::to_string(val[j][i]);
            }
            plTexts.back()+="\n";
        }
    }

    // choose save folder

    std::time_t time=std::time(nullptr); std::tm ltime=*std::localtime(&time);
    std::stringstream ifolder;
    ifolder<<lastFolder;
    ifolder<<std::put_time(&ltime, "%Y-%m-%d-%H-%M-%S");
    plFolder=QFileDialog::getSaveFileName(this, tr("Select Folder for Saving Calibration Data"),QString::fromStdString(ifolder.str()),tr("Folders")).toStdString();
    if(plFolder.empty()){
        plSchedule->setEnabled(false);
        plSchedule->setText("Schedule ?/?");
        return;
    }
    lastFolder=plFolder.substr(0,plFolder.find_last_of("/")+1);
    plFolder+="/";
    cv::utils::fs::createDirectory(plFolder);

    plSchedule->setEnabled(true);
    plSchedule->setText(QString::fromStdString(util::toString("Schedule ",plNScheduled+1,"/",plNTotal)));
    pgWr->setScheduling(true);
}
void pgCalib::onChangePlSchedule(bool status){
    if(plSchedule->isEnabled() && !plMats.empty())
        pgWr->changeDrawAreaOnExternal(status,plSpacing->val*plMats.back().cols,plSpacing->val*plMats.back().rows);
}
void pgCalib::onPlSchedule(){
    plNScheduled++;
    if(plNScheduled>=plNTotal)
        plSchedule->setEnabled(false);
    else
        plSchedule->setText(QString::fromStdString(util::toString("Schedule ",plNScheduled+1,"/",plNTotal)));
    pgWr->changeDrawAreaOnExternal(false,0,0);

    pgWrite::writePars wpars;
    wpars.pointSpacing_mm=plSpacing->val/1000;
    wpars.imgmmPPx=plSpacing->val/1000;
    wpars.focus_mm=plFoc->val/1000;
    wpars.focusXcor_mm=0;
    wpars.focusYcor_mm=0;
    wpars.depthScale=1;
    wpars.gradualW=0;
    wpars.matrixIsDuration=true;

    pgWr->scheduleScan(plMats.back(), wpars.imgmmPPx, util::toString(plFolder,plNScheduled-1,"-before"), "");
    pgWr->scheduleWriteScan(plMats.back(), wpars, util::toString(plFolder,plNScheduled-1,"-after"), plTexts.back());
    plMats.pop_back();
    plTexts.pop_back();
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
            pgWrite::writePars wps;
            wps.imgmmPPx=selArraySpacing->val/1000;
            if(pgWr->writeMat(&mplateau, wps)){delete scanRes;return true;}    //abort/fail
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
    const size_t gaussPeakNum=fitPar_nPeakGauss->val;
    const size_t lorentzPeakNum=fitPar_nPeakLorentz->val;
    const bool independentWidths=(gaussPeakNum<=1)|fitPar_independentWidths->val;
    const bool independentAngles=fitPar_independentAngles->val;
    const bool independentCentres=fitPar_independentCentres->val;
    size_t v=1;
    wfile<<"# gaussPeakNum="<<gaussPeakNum<<"\n";
    wfile<<"# lorentzPeakNum="<<lorentzPeakNum<<"\n";
    wfile<<"# independentWidths="<<independentWidths<<"\n";
    wfile<<"# independentAngles="<<independentAngles<<"\n";
    wfile<<"# independentCentres="<<independentCentres<<"\n";
    wfile<<"# "<<v++<<" Valid Measurement (=1)\n";
    wfile<<"# "<<v++<<" Focus Distance (um)\n";
    wfile<<"# "<<v++<<" Duration (ms)\n";
    wfile<<"# "<<v++<<" Plateau (nm)\n";
    wfile<<"# "<<v++<<" Peak Center Reflectivity change (averaged within FWHM)(a.u.)\n";
    wfile<<"# "<<v++<<" Max absolute 1st der. (nm/um)\n";
    wfile<<"# "<<v++<<" Min Laplacian (nm/um^2)\n";
    wfile<<"# "<<v++<<" Max Laplacian (nm/um^2)\n";
    wfile<<"# "<<v++<<" Peak Height (nm)(max)\n";
    wfile<<"# "<<v++<<" Peak Height (nm)(max-min)\n";
    wfile<<"# "<<v++<<" Prepeak (nm)\n";
    wfile<<"# "<<v++<<" PrepeakXoffs (um)\n";
    wfile<<"# "<<v++<<" PrepeakYoffs (um)\n";
    wfile<<"# "<<v++<<" Mirror Tilt X (nm/nm)\n";
    wfile<<"# "<<v++<<" Mirror Tilt Y (nm/nm)\n";
    wfile<<"# "<<v++<<" result: Combined peak height (nm)\n";
    wfile<<"# "<<v++<<" error: Combined peak height (nm)\n";
    wfile<<"# "<<v++<<" result: Combined peak FWHM (um)\n";
    wfile<<"# "<<v++<<" error: Combined peak FWHM (um)\n";
    if(!independentCentres){
        wfile<<"# "<<v++<<" FIT result: X offset (um)\n";
        wfile<<"# "<<v++<<" FIT Asymptotic standard error: X offset (um)\n";
        wfile<<"# "<<v++<<" FIT result: Y offset (um)\n";
        wfile<<"# "<<v++<<" FIT Asymptotic standard error: Y offset (um)\n";
    }
    if(gaussPeakNum>0 && !independentAngles){
        wfile<<"# "<<v++<<" FIT result: Gaussian Ellipse Angle (rad)\n";
        wfile<<"# "<<v++<<" FIT Asymptotic standard error: Gaussian Ellipse Angle (rad)\n";
    }
    if(gaussPeakNum>0 && !independentWidths){
        wfile<<"# "<<v++<<" FIT result: Gaussian Width Aspect Ratio Wx/Wy\n";
        wfile<<"# "<<v++<<" FIT Asymptotic standard error: Gaussian Width Aspect Ratio Wx/Wy\n";
    }
    wfile<<"# "<<v++<<" FIT result: Background Height (nm)\n";
    wfile<<"# "<<v++<<" FIT Asymptotic standard error: Background Height (nm)\n";


    for(size_t i=0;i!=gaussPeakNum;i++){
        wfile<<"# "<<v++<<" FIT result for Gaussian peak #"<<i<<": Peak Height (nm)\n";
        wfile<<"# "<<v++<<" FIT Asymptotic standard error for Gaussian peak #"<<i<<": Peak Height (nm)\n";
        if(independentCentres){
            wfile<<"# "<<v++<<" FIT result for Gaussian peak #"<<i<<": X offset (um)\n";
            wfile<<"# "<<v++<<" FIT Asymptotic standard error for Gaussian peak #"<<i<<": X offset (um)\n";
            wfile<<"# "<<v++<<" FIT result for Gaussian peak #"<<i<<": Y offset (um)\n";
            wfile<<"# "<<v++<<" FIT Asymptotic standard error for Gaussian peak #"<<i<<": Y offset (um)\n";
        }
        if(independentAngles){
            wfile<<"# "<<v++<<" FIT result for Gaussian peak #"<<i<<": Gaussian Ellipse Angle (rad)\n";
            wfile<<"# "<<v++<<" FIT Asymptotic standard error for Gaussian peak #"<<i<<": Gaussian Ellipse Angle (rad)\n";
        }
        wfile<<"# "<<v++<<" FIT result for Gaussian peak #"<<i<<": X width (1/e^2)(um)\n";
        wfile<<"# "<<v++<<" FIT Asymptotic standard error for Gaussian peak #"<<i<<": X width (1/e^2)(um)\n";
        wfile<<"# "<<v++<<" FIT result for Gaussian peak #"<<i<<": X width (FWHM)(um)\n";
        wfile<<"# "<<v++<<" FIT Asymptotic standard error for Gaussian peak #"<<i<<": X width (FWHM)(um)\n";
        if(independentWidths){
            wfile<<"# "<<v++<<" FIT result for Gaussian peak #"<<i<<": Y width (1/e^2)(um)\n";
            wfile<<"# "<<v++<<" FIT Asymptotic standard error for Gaussian peak #"<<i<<": Y width (1/e^2)(um)\n";
            wfile<<"# "<<v++<<" FIT result for Gaussian peak #"<<i<<": Y width (FWHM)(um)\n";
            wfile<<"# "<<v++<<" FIT Asymptotic standard error for Gaussian peak #"<<i<<": Y width (FWHM)(um)\n";
        }
    }

    for(size_t i=0;i!=lorentzPeakNum;i++){
        wfile<<"# "<<v++<<" FIT result for Lorentzian peak #"<<i<<": Peak Height (nm)\n";
        wfile<<"# "<<v++<<" FIT Asymptotic standard error for Lorentzian peak #"<<i<<": Peak Height (nm)\n";
        if(independentCentres){
            wfile<<"# "<<v++<<" FIT result for Lorentzian peak #"<<i<<": X offset (um)\n";
            wfile<<"# "<<v++<<" FIT Asymptotic standard error for Lorentzian peak #"<<i<<": X offset (um)\n";
            wfile<<"# "<<v++<<" FIT result for Lorentzian peak #"<<i<<": Y offset (um)\n";
            wfile<<"# "<<v++<<" FIT Asymptotic standard error for Lorentzian peak #"<<i<<": Y offset (um)\n";
        }
        wfile<<"# "<<v++<<" FIT result for Lorentzian peak #"<<i<<": Width (FWHM)(um)\n";
        wfile<<"# "<<v++<<" FIT Asymptotic standard error for Lorentzian peak #"<<i<<": Width (FWHM)(um)\n";
    }
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

void pgCalib::onfpLoad_pl(){
    std::vector<std::filesystem::path> readFolders; //folders still yet to be checked
    struct mesfn{
        std::filesystem::directory_entry before;
        std::filesystem::directory_entry after;
        std::filesystem::directory_entry cfg;
    };
    std::vector<mesfn> measurementsfn;

    std::string root=QFileDialog::getExistingDirectory(this, "Select Folder Containing Measurements. It will be Searched Recursively.", QString::fromStdString(lastFolder)).toStdString();
    if(root.empty()) return;

    std::lock_guard<std::mutex>lock(pgSGUI->MLP._lock_comp);
    pgSGUI->MLP.progress_comp=0;

    readFolders.push_back(std::filesystem::path(root));
    lastFolder=readFolders.back().parent_path().string()+"/";
    while(!readFolders.empty()){
        std::vector<std::filesystem::directory_entry> files;
        auto current=readFolders.back();
        for(auto const& dir_entry: std::filesystem::directory_iterator{current}) files.push_back(dir_entry);
        readFolders.pop_back();

        for(auto& path: files){
            if(path.is_directory()) readFolders.push_back(path);
            else{
                if(path.path().extension()==".pfm"){
                    if(path.path().stem().string().size()>6){
                        if(path.path().stem().string().substr(path.path().stem().string().size()-6)=="-after"){
                            mesfn a;
                            a.after=path;
                            std::string str_before=path.path().stem().string();
                            str_before.replace(path.path().stem().string().size()-6,6,"-before");
                            for(auto& path2: files){
                                if(path2.path().stem()==path.path().stem() && path2.path().extension()==".cfg"){
                                    a.cfg=path2;
                                    if(!a.before.path().string().empty()) break;
                                }else if(path2.path().stem()==str_before && path2.path().extension()==".pfm"){
                                    a.before=path2;
                                    if(!a.cfg.path().string().empty()) break;
                                }
                            }
                            if(!a.before.path().string().empty() && !a.cfg.path().string().empty())
                                measurementsfn.push_back(a);
                        }
                    }
                }
            }
        }
    }

    struct vs{
        double val=0;
        double SE=0;
        double SD=0;
        double n=0;
    };
    struct res{
        double duration_ms;
        vs height;
        double focus_um;
        double radius_um;
        double margin_um;
        double spacing_um;
        double xofs_um;
        double yofs_um;
        std::vector<vs> heights;
    };
    std::vector<res> results;

    std::string tmp;
    size_t idx,ti ;
    for(size_t n=0;n!=measurementsfn.size(); n++){
        auto& mes=measurementsfn[n];
        //std::cerr<<"pfm file:"<<mes.scan.path().stem()<<"\n";
        double radius_um, margin_um, spacing_um, focus_mm;
        std::vector<std::vector<double>> durs_ms;

        std::ifstream ifile(mes.cfg.path());
        while(ifile.peek()!=EOF){
            std::getline(ifile,tmp);
            back:;
            if(tmp.find("#Radius (um)")!=std::string::npos){
                ifile>>radius_um;
            }else if(tmp.find("#Margin (um)")!=std::string::npos){
                ifile>>margin_um;
            }else if(tmp.find("#Spacing (um)")!=std::string::npos){
                ifile>>spacing_um;
            }else if(tmp.find("#Focus (mm)")!=std::string::npos){
                ifile>>focus_mm;
            }else if(tmp.find("#Durations (ms)")!=std::string::npos){
                while(ifile.peek()!=EOF){
                    std::getline(ifile,tmp);
                    if(tmp[0]=='#') goto back;
                    if(tmp.empty()) break;
                    durs_ms.emplace_back();
                    idx=0;
                    for(;;){
                        durs_ms.back().push_back(std::stod(tmp.substr(idx),&ti));
                        idx+=ti;
                        if(idx==tmp.size()) break;
                    }
                }
            }
        }
        ifile.close();
        std::cerr<<"got matrix "<<durs_ms.size()<<" x "<<durs_ms.back().size()<<" from "<<mes.cfg.path()<<"\n";

        pgScanGUI::scanRes before, after;
        pgScanGUI::loadScan(&before, mes.before.path().string());
        pgScanGUI::loadScan(&after, mes.after.path().string());
        pgScanGUI::scanRes dif=pgSGUI->difScans(&before, &after);
        //pgScanGUI::saveScan(&dif,util::toString(mes.before.path().string(),"-dif.pfm"),false,false,false);
        double umppx=after.XYnmppx/1000;
        // slice up scan
        double xysize_um=2*(radius_um+margin_um);
        double xtotal_um=durs_ms.back().size()*xysize_um;
        double ytotal_um=durs_ms.size()*xysize_um;
        int xysize_px=xysize_um/umppx;
        int xtotal_px=xtotal_um/umppx;
        int ytotal_px=ytotal_um/umppx;
        int xofs_px=(after.depth.cols-xtotal_px)/2;
        int yofs_px=(after.depth.rows-ytotal_px)/2;

        std::vector<std::vector<pgScanGUI::scanRes>> scans;
        for(size_t j=0;j!=durs_ms.size();j++){
            scans.emplace_back();
            auto& dscans=scans.back();
            for(size_t i=0;i!=durs_ms.back().size();i++){
                dscans.emplace_back();
                auto& ddscans=dscans.back();

                cv::Rect roi(xofs_px+i*xysize_px, yofs_px+j*xysize_px,xysize_px,xysize_px);
                dif.copyTo(ddscans,roi);
                // correct min
                cv::Mat mask=cv::Mat(ddscans.depth.size(), CV_8U, 255);
                cv::circle(mask, cv::Point(mask.cols/2,mask.rows/2), (radius_um+margin_um)/umppx, 0, -1);
                //pgSGUI->correctTilt(&ddscans,&mask);
                //pgSGUI->applyTiltCorrection(&ddscans);
                double min=cv::mean(ddscans.depth,mask)[0];
                ddscans.depth-=min;
                cv::minMaxIdx(ddscans.depth,&ddscans.min, &ddscans.max);
            }
        }
        // calculate center of mass
        for(size_t v=0;v!=scans.size();v++) for(size_t u=0;u!=scans[v].size();u++){
            auto& scan=scans[v][u];
            double mid=scan.max/2;
            cv::Mat mask;
            cv::compare(scan.depth,mid,mask,cv::CMP_GE);
            auto cm=cv::moments(mask,true);
            double x,y;
            x=cm.m10/cm.m00;
            y=cm.m01/cm.m00;
            // calculate radial profile
            double rmax=radius_um+margin_um;
            const double rstep=spacing_um;
            const size_t nsteps=std::ceil(rmax/rstep);
            std::vector<vs> heights(nsteps,{0,0,0,0});
            double r, f; size_t nr;
            vs A;
            size_t nA=radius_um/2/rstep;

            // get radial profile
            for(int j=0;j!=scan.depth.rows;j++) for(int i=0;i!=scan.depth.cols;i++){
                r=sqrt(pow((i-x)*umppx,2)+pow((j-y)*umppx,2));
                nr=floor(r/rstep);
                f=1.-r/rstep+nr;     // 0 - 1
                if(nr<nsteps){
                    heights[nr].val+=f*scan.depth.at<float>(j,i);
                    heights[nr].n+=f;
                }
                if(nr+1<nsteps){
                    heights[nr+1].val+=(1-f)*scan.depth.at<float>(j,i);
                    heights[nr+1].n+=(1-f);
                }
            }
            for(size_t i=0;i<heights.size();i++){
                auto& el=heights[i];
                if(i<nA){
                    A.val+=el.val;
                    A.n+=el.n;
                }
                el.val/=el.n;
            }
            A.val/=A.n;

            // calculate SD,SE
            for(int j=0;j!=scan.depth.rows;j++) for(int i=0;i!=scan.depth.cols;i++){
                r=sqrt(pow((i-x)*umppx,2)+pow((j-y)*umppx,2));
                nr=floor(r/rstep);
                f=1.-r/rstep+nr;     // 0 - 1
                if(nr<nsteps){
                    heights[nr].SD+=pow(f*(scan.depth.at<float>(j,i)-heights[nr].val),2);
                    if(nr<nA) A.SD+=pow(f*(scan.depth.at<float>(j,i)-A.val),2);
                }
                if(nr+1<nsteps){
                    heights[nr+1].SD+=pow((1-f)*(scan.depth.at<float>(j,i)-heights[nr+1].val),2);
                    if(nr+1<nA) A.SD+=pow((1-f)*(scan.depth.at<float>(j,i)-A.val),2);
                }
            }
            for(auto& el:heights){
                el.SD=sqrt(el.SD/(el.n-1));
                el.SE=el.SD/sqrt(el.n);
            }
            A.SD=sqrt(A.SD/(A.n-1));
            A.SE=A.SD/sqrt(A.n);

            results.push_back({durs_ms[v][u],A,focus_mm*1000,radius_um,margin_um,spacing_um,x*umppx-xysize_um/2,y*umppx-xysize_um/2,heights});
        }

        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        pgSGUI->MLP.progress_comp=100./measurementsfn.size()*(n+1);
    }

    // sort data

    std::sort(results.begin(), results.end(), [](res& i,res& j){
        if(i.radius_um!=j.radius_um) return i.radius_um<j.radius_um;
        if(i.margin_um!=j.margin_um) return i.margin_um<j.margin_um;
        if(i.spacing_um!=j.spacing_um)  return i.spacing_um<j.spacing_um;
        if(i.focus_um!=j.focus_um)  return i.focus_um<j.focus_um;
        return i.duration_ms<j.duration_ms;
    });


    // export data

    std::ofstream wfile_a(util::toString(root,"/","proc.txt"));
    std::ofstream wfile_b(util::toString(root,"/","proc-all.txt"));
    wfile_a<<"# 1 - Duration (ms)\n";
    wfile_a<<"# 2 - Peak Avg. Height (nm)\n";
    wfile_a<<"# 3 - Peak Avg. Height SE (nm)\n";
    wfile_a<<"# 4 - Focus (um)\n";
    wfile_a<<"# 5 - Radius (um)\n";
    wfile_a<<"# 6 - Margin (um)\n";
    wfile_a<<"# 7 - Spacing (um)\n";
    wfile_a<<"# 8 - X Offset (um)\n";
    wfile_a<<"# 9 - Y Offset (um)\n";
    wfile_a<<"# 10 - Peak Avg. Height SD (nm)\n";
    wfile_a<<"# Peak Avg. Height is calculated from pixels withing radius/2\n";
    wfile_a<<"# Results in res-all.dat: each row corresponds to a row here, the spacing equals 6-Spacing\n";
    wfile_b<<"# cols : radius from 0 - rmax with step specified in res.dat for given row: odd are avg, even are SE\n";
    wfile_b<<"# rows : individual measurements (specific duration and focus, see res.dat)\n";

    for(auto& res:results){
        wfile_a<<res.duration_ms<<" ";
        wfile_a<<res.height.val<<" ";
        wfile_a<<res.height.SE<<" ";
        wfile_a<<res.focus_um<<" ";
        wfile_a<<res.radius_um<<" ";
        wfile_a<<res.margin_um<<" ";
        wfile_a<<res.spacing_um<<" ";
        wfile_a<<res.xofs_um<<" ";
        wfile_a<<res.yofs_um<<" ";
        wfile_a<<res.height.SD<<"\n";

        for(auto& el:res.heights){
            wfile_b<<el.val<<" "<<el.SE<<" ";
        }
        wfile_b<<"\n";
    }

    wfile_a.close();
    wfile_b.close();

    pgSGUI->MLP.progress_comp=100;
}


int pgCalib::gauss2De_f (const gsl_vector* pars, void* data, gsl_vector* f){
    const size_t n=((struct fit_data*)data)->n;
    const size_t gaussPeakNum=((struct fit_data*)data)->gaussPeakNum;
    const size_t lorentzPeakNum=((struct fit_data*)data)->lorentzPeakNum;
    const bool independentWidths=((struct fit_data*)data)->independentWidths;
    const bool independentAngles=((struct fit_data*)data)->independentAngles;
    const bool independentCentres=((struct fit_data*)data)->independentCentres;
    double* x=((struct fit_data*)data)->x;
    double* y=((struct fit_data*)data)->y;
    double* h=((struct fit_data*)data)->h;

    double a[gaussPeakNum+lorentzPeakNum];
    double wx[gaussPeakNum+lorentzPeakNum];
    double wy[gaussPeakNum+lorentzPeakNum];
    double x0[independentCentres?(gaussPeakNum+lorentzPeakNum):1];
    double y0[independentCentres?(gaussPeakNum+lorentzPeakNum):1];
    double an[independentAngles?(gaussPeakNum+lorentzPeakNum):1];
    double ar, a0;
    size_t v=0;
    if(!independentCentres){
        x0[0]=gsl_vector_get(pars, v++); // x0
        y0[0]=gsl_vector_get(pars, v++); // y0
    }
    if(gaussPeakNum>0 && !independentAngles){
        an[0]=gsl_vector_get(pars, v++); // an (angle x/y for Gauss)
    }
    if(gaussPeakNum>0 && !independentWidths){
        ar=gsl_vector_get(pars, v++);    // ar (aspect ratio wx/wy)
    }
    a0=gsl_vector_get(pars, v++);        // a0 (background height)

    double A[gaussPeakNum];
    double B[gaussPeakNum];
    double C[gaussPeakNum];
    for(size_t i=0;i!=gaussPeakNum+lorentzPeakNum;i++){
        a[i]=gsl_vector_get(pars, v++);         // a (height)
        if(independentCentres){
            x0[i]=gsl_vector_get(pars, v++);    // x0
            y0[i]=gsl_vector_get(pars, v++);    // y0
        }
        if(i<gaussPeakNum && independentAngles){
            an[i]=gsl_vector_get(pars, v++);    // an (angle x/y for Gauss)
        }
        wx[i]=gsl_vector_get(pars, v++);        // w=wx (width 1/e for Gauss or HWHM for Lortentz)
        if(i<gaussPeakNum && independentWidths){
            wy[i]=gsl_vector_get(pars, v++);    // wy (width 1/e for Gauss)
        }else{
            wy[i]=wx[i]/ar;
        }

        if(i<gaussPeakNum){
            double _an=an[independentAngles?i:0];
            A[i]=pow(cos(_an),2)/2/pow(wx[i],2)+pow(sin(_an),2)/2/pow(wy[i],2);
            B[i]=sin(2*_an)/2/pow(wx[i],2)-sin(2*_an)/2/pow(wy[i],2);
            C[i]=pow(sin(_an),2)/2/pow(wx[i],2)+pow(cos(_an),2)/2/pow(wy[i],2);
        }
    }


    for (size_t i=0; i!=n; i++){
        double model=a0;
        for(size_t j=0;j!=gaussPeakNum+lorentzPeakNum;j++){
            double _x0=x0[independentCentres?j:0];
            double _y0=y0[independentCentres?j:0];
            if(j<gaussPeakNum)
                model+=abs(a[j])*exp(-A[j]*pow(x[i]-_x0,2)-B[j]*(x[i]-_x0)*(y[i]-_y0)-C[j]*pow(y[i]-_y0,2));
            else
                model+=abs(a[j])*pow(wx[j],2)/(pow((x[i]-_x0),2)+pow((y[i]-_y0),2)+pow(wx[j],2));
        }

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
    const size_t gaussPeakNum=fitPar_nPeakGauss->val;
    const size_t lorentzPeakNum=fitPar_nPeakLorentz->val;
    if(gaussPeakNum+lorentzPeakNum==0) return;
    const bool independentWidths=(gaussPeakNum<=1)|fitPar_independentWidths->val;
    const bool independentAngles=fitPar_independentAngles->val;
    const bool independentCentres=fitPar_independentCentres->val;
    const bool directFWHMandHeight=fitPar_directFWHMandHeight->val;
    double initval[100];
    size_t v=0;
    if(!independentCentres){
        initval[v++]=scanDif.depth.cols/2.; // x0
        initval[v++]=scanDif.depth.rows/2.; // y0
    }
    if(gaussPeakNum>0 && !independentAngles){
        initval[v++]=0.01;      // an (angle x/y for Gauss)
    }
    if(gaussPeakNum>0 && !independentWidths){
        initval[v++]=1;         // ar (aspect ratio wx/wy)
    }
    initval[v++]=scanDif.min;   // a0 (background height)


    for(size_t i=0;i!=gaussPeakNum+lorentzPeakNum;i++){
        initval[v++]=(scanDif.max-scanDif.min)/(gaussPeakNum+lorentzPeakNum);   // a (height)
        if(independentCentres){
            initval[v++]=scanDif.depth.cols/((i>=gaussPeakNum)?10.:2.);  // x0
            initval[v++]=scanDif.depth.rows/((i>=gaussPeakNum)?10.:2.);  // y0
        }
        if(i<gaussPeakNum && independentAngles){
            initval[v++]=0.01;      // an (angle x/y for Gauss)
        }
        initval[v++]=4000/scanDif.XYnmppx;              // w=wx (width 1/e for Gauss or HWHM for Lortentz)
        if(i<gaussPeakNum && independentWidths){
            initval[v++]=4000/scanDif.XYnmppx;          // wy (width 1/e for Gauss)
        }
    }
    const size_t parN=v;
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

    struct fit_data data{ptN,gaussPeakNum,lorentzPeakNum,independentWidths,independentAngles,independentCentres,x,y,h};
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
                output_mat->at<float>(j,i)=gsl_vector_get(workspace->f, n++);
                if(n>ptN) throw std::out_of_range("FIXME: in pgCalib::calcParameters when calculation output_mat: too many points.\n");
            }else{
                output_mat->at<float>(j,i)=0;
            }
        }
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

    double tiltX=scanDif.tiltCor[0]/scanDif.XYnmppx;    // these tilts are actually from scanBefore
    double tiltY=scanDif.tiltCor[1]/scanDif.XYnmppx;

    std::vector<double> shared;
    std::vector<std::vector<double>> gpeaks, lpeaks;
    size_t w=0;
    bool flip=false;
    int valid=1;
    if(!independentCentres){
        if(res[w]<0 || res[w]>scanDif.depth.cols) valid=0;
        shared.push_back(res[w]*XYumppx);                           // FIT result: X offset(um)
        shared.push_back(err[w++]*XYumppx);                         // FIT Asymptotic standard error: X offset(um)
        if(res[w]<0 || res[w]>scanDif.depth.rows) valid=0;
        shared.push_back(res[w]*XYumppx);                           // FIT result: Y offset(um)
        shared.push_back(err[w++]*XYumppx);                         // FIT Asymptotic standard error: Y offset(um)
    }
    if(gaussPeakNum>0 && !independentAngles){
        double an=res[w];
        while(an>M_PI) an-=M_PI;
        while(an<0) an+=M_PI;
        flip=(an>M_PI/2);
        if(flip) an-=M_PI/2;
        shared.push_back(an);                                       // FIT result: Gaussian Ellipse Angle(rad)
        shared.push_back(err[w++]);                                 // FIT Asymptotic standard error: Gaussian Ellipse Angle(rad)
    }
    if(gaussPeakNum>0 && !independentWidths){
        if(flip){
            shared.push_back(1/res[w]);
            shared.push_back(1/pow(res[w],2)*err[w]);
            w++;
        }else{
            shared.push_back(res[w]);                               // FIT result: Gaussian Width Aspect Ratio Wx/Wy
            shared.push_back(err[w++]);                             // FIT Asymptotic standard error: Gaussian Width Aspect Ratio Wx/Wy
        }
    }
    shared.push_back(res[w]);                                       // FIT result: Background Height (nm)
    shared.push_back(err[w++]);                                     // FIT Asymptotic standard error: Background Height (nm)
    for(size_t i=0;i!=gaussPeakNum;i++){
        gpeaks.emplace_back();
        gpeaks.back().push_back(abs(res[w]));                       // FIT result for Gaussian peak i: Peak Height (nm)
        gpeaks.back().push_back(err[w++]);                          // FIT Asymptotic standard error for Gaussian peak i: Peak Height (nm)
        if(independentCentres){
            if(res[w]<0 || res[w]>scanDif.depth.cols) valid=0;
            gpeaks.back().push_back(res[w]*XYumppx);                // FIT result for Gaussian peak i: X offset(um)
            gpeaks.back().push_back(err[w++]*XYumppx);              // FIT Asymptotic standard error for Gaussian peak i: X offset(um)
            if(res[w]<0 || res[w]>scanDif.depth.rows) valid=0;
            gpeaks.back().push_back(res[w]*XYumppx);                // FIT result for Gaussian peak i: Y offset(um)
            gpeaks.back().push_back(err[w++]*XYumppx);              // FIT Asymptotic standard error for Gaussian peak i: Y offset(um)
        }
        if(independentAngles){
            double an=res[w];
            while(an>M_PI) an-=M_PI;
            while(an<0) an+=M_PI;
            flip=(an>M_PI/2);
            if(flip) an-=M_PI/2;
            gpeaks.back().push_back(an);                            // FIT result for Gaussian peak i: Gaussian Ellipse Angle(rad)
            gpeaks.back().push_back(err[w++]);                      // FIT Asymptotic standard error for Gaussian peak i: Gaussian Ellipse Angle(rad)
        }
        gpeaks.back().push_back(2*abs(res[w])*XYumppx);                             // FIT result for Gaussian peak i: X width (1/e^2)(um)
        gpeaks.back().push_back(2*err[w++]*XYumppx);                                // FIT Asymptotic standard error for Gaussian peak i: X width (1/e^2)(um)
        gpeaks.back().push_back(gpeaks.back()[gpeaks.back().size()-2]*toFWHM);      // FIT result for Gaussian peak i: X width (FWHM)(um)
        gpeaks.back().push_back(gpeaks.back()[gpeaks.back().size()-2]*toFWHM);      // FIT Asymptotic standard error for Gaussian peak i: X width (FWHM)(um)
        if(independentWidths){
            gpeaks.back().push_back(2*abs(res[w])*XYumppx);                         // FIT result for Gaussian peak i: Y width (1/e^2)(um)
            gpeaks.back().push_back(2*err[w++]*XYumppx);                            // FIT Asymptotic standard error for Gaussian peak i: Y width (1/e^2)(um)
            gpeaks.back().push_back(gpeaks.back()[gpeaks.back().size()-2]*toFWHM);  // FIT result for Gaussian peak i: Y width (FWHM)(um)
            gpeaks.back().push_back(gpeaks.back()[gpeaks.back().size()-2]*toFWHM);  // FIT Asymptotic standard error for Gaussian peak i: Y width (FWHM)(um)
        }
        if(flip){
            if(independentWidths){
                double tmp;
                for(int j:{1,2,3,4}){
                    tmp=gpeaks.back()[gpeaks.back().size()-j];
                    gpeaks.back()[gpeaks.back().size()-j]=gpeaks.back()[gpeaks.back().size()-j-4];
                    gpeaks.back()[gpeaks.back().size()-j-4]=tmp;
                }
            }else{
                for(int j:{1,2,3,4})
                    gpeaks.back()[gpeaks.back().size()-j]*=shared[shared.size()-4];
            }
        }
    }
    for(size_t i=0;i!=lorentzPeakNum;i++){
        lpeaks.emplace_back();
        lpeaks.back().push_back(abs(res[w]));                       // FIT result for Lorentzian peak i: Peak Height (nm)
        lpeaks.back().push_back(err[w++]);                          // FIT Asymptotic standard error for Lorentzian peak i: Peak Height (nm)
        if(independentCentres){
            lpeaks.back().push_back(res[w]*XYumppx);                // FIT result for Lorentzian peak i: X offset(um)
            lpeaks.back().push_back(err[w++]*XYumppx);              // FIT Asymptotic standard error for Lorentzian peak i: X offset(um)
            lpeaks.back().push_back(res[w]*XYumppx);                // FIT result for Lorentzian peak i: Y offset(um)
            lpeaks.back().push_back(err[w++]*XYumppx);              // FIT Asymptotic standard error for Lorentzian peak i: Y offset(um)
        }
        lpeaks.back().push_back(2*abs(res[w])*XYumppx);             // FIT result for Lorentzian peak i: Width (FWHM)(um)
        lpeaks.back().push_back(2*err[w++]*XYumppx);                // FIT Asymptotic standard error for Lorentzian peak i: Width (FWHM)(um)
    }
    if(gpeaks.size()>1)
        std::sort(gpeaks.begin(), gpeaks.end(), [](std::vector<double>& i,std::vector<double>& j){return (i.front()>j.front());});      // sort by peak height
    if(lpeaks.size()>1)
        std::sort(lpeaks.begin(), lpeaks.end(), [](std::vector<double>& i,std::vector<double>& j){return (i.front()>j.front());});      // sort by peak height

    double comboHeight{0}, comboHeightErr{0};
    double comboFWHM{0}, comboFWHMErr{0};
    if(directFWHMandHeight){
        int n=0;
        double bgnd=0;
        for(int i=0;i!=scanDif.depth.cols;i++) for(int j=0;j!=scanDif.depth.rows;j++){
            if(sqrt(pow(i-scanDif.depth.cols/2.,2)+pow(j-scanDif.depth.rows/2.,2))>scanDif.depth.cols*0.45 && scanDif.mask.at<uchar>(j,i)==0){
                bgnd+=scanDif.depth.at<float>(j,i);
                n++;
            }
        }
        bgnd/=n;
        double min,max;
        cv::minMaxIdx(scanDif.depth, &min, &max, nullptr, nullptr,scanDif.maskN);
        comboHeightErr=0;
        comboHeight=max-bgnd;
        cv::Mat ring;
        cv::compare(scanDif.depth,bgnd+comboHeight/2,ring,cv::CMP_GE);
        auto mom=cv::moments(ring, true);
        comboFWHM=2*sqrt(mom.m00/M_PI)*XYumppx;
        comboFWHMErr=0;
    }else if(gaussPeakNum+lorentzPeakNum==1){
        std::vector<double> peak=(gaussPeakNum==1)?gpeaks.front():lpeaks.front();
        size_t u=0;
        comboHeight=peak[u++];
        comboHeightErr=peak[u++];
        if(independentCentres) u+=4;
        if(lorentzPeakNum==1){
            comboFWHM=peak[u++];
            comboFWHMErr=peak[u++];
        }else{
            if(independentAngles) u+=2;
            u+=2;
            comboFWHM=(peak[u]+peak[u+4])/2;
            comboFWHMErr=sqrt(pow(peak[u+1],2)+pow(peak[u+5],2))/2;
        }
    }else{
        cv::Mat hMat=scanDif.depth.clone();
        int n=0;
        for(int i=0;i!=scanDif.depth.cols;i++) for(int j=0;j!=scanDif.depth.rows;j++){
            if(scanDif.mask.at<uchar>(j,i)==0){
                hMat.at<float>(j,i)+=gsl_vector_get(workspace->f, n++);
            }
        }
        hMat-=shared[shared.size()-2];  // subtract background
        cv::Point ctr;
        cv::minMaxLoc(hMat, nullptr, &comboHeight,nullptr, &ctr, scanDif.maskN);
        cv::Mat ring=cv::Mat::zeros(scanDif.depth.rows, scanDif.depth.cols, CV_8U);
        cv::Mat tmp;
        cv::compare(hMat,comboHeight/2,ring,cv::CMP_GE);
        cv::compare(hMat,comboHeight/2,tmp,cv::CMP_LE);
        cv::dilate(ring,ring,cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3), cv::Point(1,1)));
        cv::dilate(tmp,tmp,cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3), cv::Point(1,1)));
        ring=ring&tmp;
        n=0;
        for(int i=0;i!=scanDif.depth.cols;i++) for(int j=0;j!=scanDif.depth.rows;j++){
            if(scanDif.mask.at<uchar>(j,i)==0 && ring.at<uchar>(j,i)==255){
                comboFWHM+=(sqrt(pow(i-ctr.x,2)+pow(j-ctr.y,2))*2*XYumppx);
                n++;
            }
        }
        comboFWHM/=n;

        // now calculate errors, we estimate using weights
        for(int i=0;i!=scanDif.depth.cols;i++) for(int j=0;j!=scanDif.depth.rows;j++){
            if(scanDif.mask.at<uchar>(j,i)==0 && ring.at<uchar>(j,i)==255){
                comboFWHMErr+=pow((sqrt(pow(i-ctr.x,2)+pow(j-ctr.y,2))*2*XYumppx)-comboFWHM,2);
            }
        }
        comboFWHMErr=sqrt(comboFWHMErr/n/(n-1));    //SE

        double sumAmp=0;
        for(auto& peaks: {gpeaks,lpeaks}) for(auto& peak: peaks)
            sumAmp+=peak[0];
        for(auto& peaks: {gpeaks,lpeaks}) for(auto& peak: peaks){
            double weight=peak[0]/sumAmp;
            comboHeightErr+=weight*peak[1];
            if(&peaks==&lpeaks) comboFWHMErr+=weight*peak[3+(independentCentres?4:0)];      // we add these errors as well to get a conservative estimate
            else{
                size_t u=2+(independentCentres?4:0)+(independentAngles?2:0)+2+1;
                if(independentWidths)
                    comboFWHMErr+=weight*sqrt(pow(peak[u],2)+pow(peak[u+4],2))/2;
                else{
                    size_t v=(independentCentres?0:4)+((gaussPeakNum>0 && !independentAngles)?2:0);
                    comboFWHMErr+=weight*sqrt(pow((1+1/shared[v])*peak[u]/2,2)+pow(shared[v+1]*peak[u-1]/pow(shared[v],2)/2,2));
                }
            }

        }
        comboHeightErr=sqrt(pow(comboHeightErr,2)+pow(shared.back(),2));    // add background height error

    }
    for(auto& var: shared) if(var>1e6 || var<-1e6) valid=0;
    for(auto& _var: gpeaks) for(auto& var: _var) if(var>1e6 || var<-1e6) valid=0;
    for(auto& _var: lpeaks) for(auto& var: _var) if(var>1e6 || var<-1e6) valid=0;

    gsl_multifit_nlinear_free (workspace);
    gsl_matrix_free (covar);

    *output+=util::toString(
        valid," ",                      // Valid Measurement (=1)
        focus," ",                      // Focus Distance (um)
        duration," ",                   // Duration (ms)
        plateau," ",                    // Plateau (nm)
        peakRefl," ",                   // Peak Center Reflectivity change (averaged within FWHM)(a.u.)
        maxDepthDer," ",                // Max Absolute 1st Der. (nm/um)
        minDepthLaplacian," ",          // Min Laplacian (nm/um^2)
        maxDepthLaplacian," ",          // Max Laplacian (nm/um^2)
        scanDif.max," ",                // Peak Height (nm)(max)
        scanDif.max-scanDif.min," ",    // Peak Height (nm)(max-min)
        _prepeak," ",                   // Prepeak (nm)
        prepeakXofs," ",                // PrepeakXoffs (um)
        prepeakYofs," ",                // PrepeakYoffs (um)
        tiltX," ",                      // Mirror Tilt X (nm/nm)
        tiltY," ",                      // Mirror Tilt Y (nm/nm)
        comboHeight," ",                // result: Combined peak height(nm)\n";
        comboHeightErr," ",             // error: Combined peak height(nm)\n";
        comboFWHM," ",                  // result: Combined peak FWHM(um)\n";
        comboFWHMErr," ");              // error: Combined peak FWHM(um)\n";
    for(auto& var: shared) *output+=util::toString(var," ");
    for(auto& _var: gpeaks) for(auto& var: _var) *output+=util::toString(var," ");
    for(auto& _var: lpeaks) for(auto& var: _var) *output+=util::toString(var," ");
    *output+="\n";

    if(prepeak!=nullptr)*prepeak=comboHeight;

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
            else if(path.path().stem().string()=="proc" && path.path().extension().string()==".txt") measFiles.push_back(path);
        }
    }

    std::array<size_t,7> pos;   // indices in data(starting from 1): valid, duration, height, height_err, focus, xofs, yofs
    pos.fill(0);
    for(auto& item:measFiles){
        std::ifstream ifs(item);
        for(std::string line; std::getline(ifs, line);){
            if(line[0]=='#'){
                if(std::isdigit(line[2])){
                    size_t idx=std::stoi(line.substr(2));
                    if(line.find("Valid Measurement (=1)")!=std::string::npos) pos[0]=idx;
                    else if(line.find("Duration (ms)")!=std::string::npos) pos[1]=idx;
                    else if(line.find("result: Combined peak height (nm)")!=std::string::npos) pos[2]=idx;
                    else if(line.find("error: Combined peak height (nm)")!=std::string::npos) pos[3]=idx;
                    else if(line.find("Focus Distance (um)")!=std::string::npos) pos[4]=idx;
                    else if(line.find("FIT result for Gaussian peak #0: X offset (um)")!=std::string::npos) pos[5]=idx;
                    else if(line.find("FIT result for Gaussian peak #0: Y offset (um)")!=std::string::npos) pos[6]=idx;
                    else if(line.find("FIT result for Lorentzian peak #0: X offset (um)")!=std::string::npos && pos[5]==0) pos[5]=idx;      // if no gaussian
                    else if(line.find("FIT result for Lorentzian peak #0: Y offset (um)")!=std::string::npos && pos[6]==0) pos[6]=idx;
                    else if(line.find("Peak Avg. Height (nm)")!=std::string::npos) pos[2]=idx;
                    else if(line.find("Peak Avg. Height SE (nm)")!=std::string::npos) pos[3]=idx;
                    else if(line.find("Focus (um)")!=std::string::npos) pos[4]=idx;
                    else if(line.find("X Offset (um)")!=std::string::npos) pos[5]=idx;
                    else if(line.find("Y Offset (um)")!=std::string::npos) pos[6]=idx;
                }
                continue;
            }
            size_t sx=0, sxi;
            bool valid=true;
            double _duration, _height, _height_err, _focus[3];
            for(size_t i=1;;i++){
                std::string substr=line.substr(sx);
                if(std::none_of(substr.begin(), substr.end(), [](char c){return std::isalnum(c);})) break;
                double tmp=std::stod(substr,&sxi);
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
    for(size_t i:{0,1,2}) focus[i]=0;
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

    gsl_bspline_workspace *bsplws=gsl_bspline_alloc(4, nbreak); // cubic bspline
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

    for(auto& he: height)
        if(he<0) he=0;      // dirty fix (we fit bsplines in an inverted yx, so it cannot fit to points y<0)

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
            pgWr->settingWdg[i]->focus->setValue(focus[0]/fpLoadedData.size());
            pgWr->settingWdg[i]->focusXcor->setValue(focus[1]/fpLoadedData.size());
            pgWr->settingWdg[i]->focusYcor->setValue(focus[2]/fpLoadedData.size());
            pgWr->settingWdg[i]->p_ready=false;
            return;
        }
    }
}

