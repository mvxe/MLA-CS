#include "deptheval.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgDepthEval::pgDepthEval(pgBoundsGUI* pgBGUI): pgBGUI(pgBGUI){
    layout=new QVBoxLayout;
    this->setLayout(layout);
    QLabel* text=new QLabel("This section is for debugging only. Here you can check how various filters applied to the image using parameters below work. (some of) These functions are used in (some of) the methods in Write Calibration (however the parameters below are overriden by those there).");
    text->setWordWrap(true);
    layout->addWidget(text);
    debugDisp=new smp_selector("Display debug:", 0, {"none","Blur","Blur+Laplacian","Blur+Laplacian+Thresh","Blur+Laplacian+Thresh+Dilate","Blur+Laplacian+Thresh+Dilate+Exclusion","Blur+Laplacian+Thresh+Dilate+Exclusion+Border","Exclusion","Exclusion+DilateSQ","Exclusion+DilateSQ+Border"});
    connect(debugDisp, SIGNAL(changed(int)), this, SLOT(onDebugIndexChanged(int)));
    layout->addWidget(debugDisp);
    layout->addWidget(new hline);
    findf_Blur=new val_selector(2, "Gaussian Blur Sigma: ", 0, 100, 1, 0, {"px"});
    conf["findf_Blur"]=findf_Blur;
    connect(findf_Blur, SIGNAL(changed()), this, SLOT(onChanged()));
    findf_Thrs=new val_selector(0.2, "2nd Derivative Exclusion Threshold: ", 0, 1, 4);
    conf["findf_Thrs"]=findf_Thrs;
    connect(findf_Thrs, SIGNAL(changed()), this, SLOT(onChanged()));
    findf_Dill=new val_selector(0, "Dillation: ", 0, 1000, 0, 0, {"px"});
    conf["findf_Dill"]=findf_Dill;
    connect(findf_Dill, SIGNAL(changed()), this, SLOT(onChanged()));
    findf_Rand=new val_selector(0, "Randomize: ", 0, 1000, 0, 0, {"px"});
    conf["findf_Rand"]=findf_Rand;
    findf_Dilly=new val_selector(0, "Dillation Y: ", 0, 1000, 0, 0, {"px"});
    conf["findf_Dilly"]=findf_Dilly;
    connect(findf_Dilly, SIGNAL(changed()), this, SLOT(onChanged()));
    btnGoToNearestFree=new QPushButton("Go to nearest free");
    btnGoToNearestFree->setToolTip("This adheres to write boundaries!");
    connect(btnGoToNearestFree, SIGNAL(released()), this, SLOT(onGoToNearestFree()));
    layout->addWidget(findf_Blur);
    layout->addWidget(findf_Thrs);
    layout->addWidget(findf_Dill);
    layout->addWidget(findf_Rand);
    layout->addWidget(findf_Dilly);
    layout->addWidget(new QLabel("If dillation y!=0, use rect with(dil,dily)."));
    layout->addWidget(new twid{btnGoToNearestFree});
}
void pgDepthEval::onGoToNearestFree(){
    Q_EMIT sigGoToNearestFree(findf_Dill->val, findf_Rand->val, findf_Blur->val, findf_Thrs->val, findf_Dilly->val, true);
}

void pgDepthEval::onDebugIndexChanged(int index){
    debugIndex=index;
    _debugChanged=true;
}
void pgDepthEval::onChanged(){
    _debugChanged=true;
}

const pgScanGUI::scanRes* pgDepthEval::getDebugImage(const pgScanGUI::scanRes* src){
    _debugChanged=false;
    if(src==nullptr || debugIndex<0 || debugIndex==0 || debugIndex>9) return src;

    src->copyTo(res);
    double sigma=findf_Blur->val;
    int ksize=sigma*5;
    if(!(ksize%2)) ksize++;
    res.depth.setTo(res.max+abs(res.max-res.min)/2,res.mask);    //if you blur values with infinite... it spills out of the mask
    cv::GaussianBlur(res.depth, res.depth, cv::Size(ksize, ksize), sigma, sigma);
    cv::Point  ignore;
    int dilation_size=findf_Dill->val;
    int dilation_sizey=findf_Dilly->val;
    if(debugIndex<=6){
        if(debugIndex==1) {res.depth.setTo(std::numeric_limits<float>::max(),res.mask); cv::minMaxLoc(res.depth, &res.min, &res.max, &ignore, &ignore, res.maskN); return &res;}
        cv::Laplacian(res.depth, res.depth, CV_32F);
        cv::divide(res.depth,8,res.depth);  //this should make the colorbar unit nm/nm^2
        res.depth.setTo(std::numeric_limits<float>::max(),res.mask);
        cv::minMaxLoc(res.depth, &res.min, &res.max, &ignore, &ignore, res.maskN);
        if(debugIndex==2) return &res;
        cv::Mat mask, maskT;
        double thrs=findf_Thrs->val;
        cv::compare(res.depth,  thrs, mask , cv::CMP_GT);
        cv::compare(res.depth, -thrs, maskT, cv::CMP_LT);
        res.mask.setTo(cv::Scalar::all(255), mask);
        res.mask.setTo(cv::Scalar::all(255), maskT);
        if(debugIndex==3) {bitwise_not(res.mask, res.maskN); return &res;}

        if(dilation_size>0){
            cv::Mat element;
            if(dilation_sizey>0) element=cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2*dilation_size+1,2*dilation_sizey+1), cv::Point(dilation_size,dilation_sizey));
            else element=cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2*dilation_size+1,2*dilation_size+1), cv::Point(dilation_size,dilation_size));
            cv::dilate(res.mask, res.mask, element);
        }
        bitwise_not(res.mask, res.maskN);
        if(debugIndex==4)  return &res;
    }

    pgBGUI->drawBound(&res.mask, res.XYnmppx, true);
    if(debugIndex==5 || debugIndex==7)  return &res;

    if(debugIndex==8 || debugIndex==9) if(dilation_size>0){
        cv::Mat element;
        if(dilation_sizey>0) element=cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2*dilation_size+1,2*dilation_sizey+1), cv::Point(dilation_size,dilation_sizey));
        else element=cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2*dilation_size+1,2*dilation_size+1), cv::Point(dilation_size,dilation_size));
        cv::Mat mask2(res.mask.rows,res.mask.cols,res.mask.type(),cv::Scalar(0));
        pgBGUI->drawBound(&mask2, res.XYnmppx, true);
        cv::dilate(mask2, mask2, element);
        cv::bitwise_or(mask2, res.mask, res.mask);
        if(debugIndex==8) return &res;
    }

    int red;
    if(dilation_sizey>0) red=dilation_sizey<res.mask.rows?dilation_sizey:res.mask.rows;
    else red=dilation_size<res.mask.rows?dilation_size:res.mask.rows;
    res.mask.rowRange(0,red).setTo(cv::Scalar(255));
    res.mask.rowRange(res.mask.rows-red,res.mask.rows).setTo(cv::Scalar(255));
    red=dilation_size<res.mask.cols?dilation_size:res.mask.cols;
    res.mask.colRange(0,red).setTo(cv::Scalar(255));
    res.mask.colRange(res.mask.cols-red,res.mask.cols).setTo(cv::Scalar(255));
    if(debugIndex==6 || debugIndex==9)  return &res;
}

cv::Mat pgDepthEval::getMaskFlatness(const pgScanGUI::scanRes* src, int dil, double thresh, double blur, int dily){         // used in Auto Array Calibration method
    cv::Mat retMask;
    src->mask.copyTo(retMask);

    cv::Mat mat;
    src->depth.copyTo(mat);
    double sigma=blur;
    int ksize=sigma*5;
    if(!(ksize%2)) ksize++;
    mat.setTo(src->max+abs(src->max-src->min)/2,src->mask);
    cv::GaussianBlur(mat, mat, cv::Size(ksize, ksize), sigma, sigma);
    cv::Laplacian(mat, mat, CV_32F);
    cv::divide(mat,8,mat);  //should make the unit mm/mm, if its too slow remove this
    cv::Mat umask, umaskT;
    cv::compare(mat,  thresh, umask , cv::CMP_GT);
    cv::compare(mat, -thresh, umaskT, cv::CMP_LT);
    retMask.setTo(cv::Scalar::all(255), umask);
    retMask.setTo(cv::Scalar::all(255), umaskT);
    if(dil>0){
        cv::Mat element;
        if(dily==0) element=cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2*dil+1,2*dil+1), cv::Point(dil,dil));
        else element=cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2*dil+1,2*dily+1), cv::Point(dil,dily));
        cv::dilate(retMask, retMask, element);
        int red=dil<retMask.rows?dil:retMask.rows;
        retMask.rowRange(0,red).setTo(cv::Scalar(255));
        retMask.rowRange(retMask.rows-red,retMask.rows).setTo(cv::Scalar(255));
        red=dil<retMask.cols?dil:retMask.cols;
        retMask.colRange(0,red).setTo(cv::Scalar(255));
        retMask.colRange(retMask.cols-red,retMask.cols).setTo(cv::Scalar(255));
    }
    pgBGUI->drawBound(&retMask, res.XYnmppx, true);
    return retMask;
}
