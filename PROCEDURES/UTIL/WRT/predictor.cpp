#include "predictor.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgWritePredictor::pgWritePredictor(pgMoveGUI* pgMGUI): pgMGUI(pgMGUI){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);

    predictorSel=new twds_selector("pgWritePredictor_predictorSel",0,"Select predictor:", "Gaussian", false, false, false);
    slayout->addWidget(predictorSel);
    slayout->addWidget(new hline);
    test=new QPushButton("Test selected predictor on depthmap in memory.");
    connect(test, SIGNAL(released()), this, SLOT(onTest()));
    slayout->addWidget(new twid(test));
    testTargetHeight=new val_selector(10, "pgWritePredictor_testTargetHeight", "Pulse FWHM", 0.001, 1000, 3, 0, {"nm"});
    slayout->addWidget(testTargetHeight);
    predictorSel->addWidget(prd_gaussian.gui_settings, "Gaussian");
    //.. add other predictors here
    predictorSel->doneAddingWidgets();
}
void pgWritePredictor::onTest(){
    _debugChanged=true;
}
const pgScanGUI::scanRes* pgWritePredictor::getDebugImage(const pgScanGUI::scanRes* src){
    if(!_debugChanged) return src;
    _debugChanged=false;
    if(src==nullptr) return src;
    int x,y;
    prd_gaussian.getBestSize(&x, &y, pgMGUI->getNmPPx());
    src->copyTo(res);
    prd_gaussian.evalm(&res,testTargetHeight->val);
    return &res;
}


//------------------------

gaussian_predictor::gaussian_predictor(){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    pulseDur=new val_selector(1, "gaussian_predictor_pulseDur", "Pulse Duration", 0.001, 1000, 3, 0, {"ms"});
    slayout->addWidget(pulseDur);
    pulseFWHM=new val_selector(6, "gaussian_predictor_pulseFWHM", "Pulse FWHM", 0.001, 1000, 3, 0, {"um"});
    slayout->addWidget(pulseFWHM);
    bestSizeCutoffPercent=new val_selector(0.1, "gaussian_predictor_bestSizeCutoffPercent", "Best Mat size cutoff (% of Max peak)", 0.001, 100, 3, 0, {"%"});
    slayout->addWidget(bestSizeCutoffPercent);
}
void gaussian_predictor::getBestSize(int* xSize, int* ySize, double XYnmppx){
    const double precisionLimit=0.000001;
    double w=pulseFWHM->val*1000/2/sqrt(2*log(2))/XYnmppx;
    double reqX=w;
    double oldReqX=0;
    double tmp;
    double target=bestSizeCutoffPercent->val/100;       // 0-1, so the gaussian amp should be 1
    while(target<gaussian(reqX,0,1,w,w)) reqX*=2;
    int N=0;
    while(abs(target-gaussian(reqX,0,1,w,w))>precisionLimit){
        tmp=reqX;
        if(target<gaussian(reqX,0,1,w,w)) reqX+=abs(reqX-oldReqX)/2;
        else reqX-=abs(reqX-oldReqX)/2;
        oldReqX=tmp;
        N++;
    }
    *xSize=ceil(reqX*2);
    *ySize=ceil(reqX*2);
    return;
}
cv::Mat gaussian_predictor::eval(const pgScanGUI::scanRes* pre, double centerHChange, double* intensity, double* duration){

}
void gaussian_predictor::evalm(pgScanGUI::scanRes* pre, double centerHChange, double* intensity, double* duration){

}
double gaussian_predictor::gaussian(double x, double y, double a, double wx, double wy, double an){
    double A=pow(cos(an),2)/2/pow(wx,2)+pow(sin(an),2)/2/pow(wy,2);
    double B=sin(2*an)/2/pow(wx,2)-sin(2*an)/2/pow(wy,2);
    double C=pow(sin(an),2)/2/pow(wx,2)+pow(cos(an),2)/2/pow(wy,2);
    return a*exp(-A*pow(x,2)-B*(x)*(y)-C*pow(y,2));
}
