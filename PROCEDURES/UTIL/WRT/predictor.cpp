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
    testTargetHeight=new val_selector(10, "pgWritePredictor_testTargetHeight", "Pulse central height gain", 0.001, 1000, 3, 0, {"nm"});
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
    //prd_gaussian.getBestSize(&x, &y, pgMGUI->getNmPPx());
    src->copyTo(res);
    prd_gaussian.evalm(&res, cv::Point2d(res.depth.cols/2.,res.depth.rows/2.),testTargetHeight->val);
    return &res;
}


//------------------------

cv::Mat _predictor::eval(const pgScanGUI::scanRes* pre, cv::Point2f center, float centerHChange, double* intensity, double* duration){
    if(pre==nullptr) return cv::Mat();
    cv::Mat ret(pre->depth.rows,pre->depth.cols,pre->depth.type());
    _eval(pre, &ret, center, centerHChange, intensity, duration);
    return ret;
}
void _predictor::evalm(pgScanGUI::scanRes* pre, cv::Point2f center, float centerHChange, double* intensity, double* duration){
    if(pre==nullptr) return;
    _eval(pre, &pre->depth, center, centerHChange, intensity, duration);
    cv::minMaxLoc(pre->depth, &pre->min, &pre->max, nullptr, nullptr, pre->maskN);
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
void gaussian_predictor::_getBestSize(int* xSize, int* ySize, double XYnmppx){   //this is for performance
    const float precisionLimit=0.000001;
    float w=pulseFWHM->val*1000/2/sqrt(2*log(2))/XYnmppx;
    float reqX=w;
    float oldReqX=0;
    float tmp;
    float target=bestSizeCutoffPercent->val/100;       // 0-1, so the gaussian amp should be 1
    while(target<gaussian(reqX,0,1,w,w)) reqX*=2;
    int N=0;
    while(abs(target-gaussian(reqX,0,1,w,w))>precisionLimit){
        tmp=reqX;
        if(target<gaussian(reqX,0,1,w,w)) reqX+=abs(reqX-oldReqX)/2;
        else reqX-=abs(reqX-oldReqX)/2;
        oldReqX=tmp;
        N++;
    }
    *xSize=ceilf(reqX*2);
    *ySize=ceilf(reqX*2);
    return;
}
void gaussian_predictor::_eval(const pgScanGUI::scanRes* pre, cv::Mat* post, cv::Point2f center, float centerHChange, double* intensity, double* duration){
    int xSize, ySize;
    _getBestSize(&xSize,&ySize,pre->XYnmppx);
    cv::Rect roi(center.x-xSize/2., center.y-ySize/2.,xSize,ySize);
    roi&=cv::Rect(0,0,pre->depth.cols,pre->depth.rows); //crop the roi to image
    double w=pulseFWHM->val*1000/2/sqrt(2*log(2))/pre->XYnmppx;
    for(int i=0;i!=roi.width;i++) for(int j=0;j!=roi.height;j++)
        (*post)(roi).at<float>(j,i)+=gaussian(roi.x+i-center.x, roi.y+j-center.y, centerHChange,w,w);
}
float gaussian_predictor::gaussian(float x, float y, float a, float wx, float wy, float an){
    float A=powf(cosf(an),2)/2/powf(wx,2)+powf(sinf(an),2)/2/powf(wy,2);
    float B=sinf(2*an)/2/powf(wx,2)-sinf(2*an)/2/powf(wy,2);
    float C=powf(sinf(an),2)/2/powf(wx,2)+powf(cosf(an),2)/2/powf(wy,2);
    return a*expf(-A*powf(x,2)-B*(x)*(y)-C*powf(y,2));
}
