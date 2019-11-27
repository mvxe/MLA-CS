#include "deptheval.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgDepthEval::pgDepthEval(pgBoundsGUI* pgBGUI): pgBGUI(pgBGUI){
    layout=new QVBoxLayout;
    this->setLayout(layout);
    debugDisp=new smp_selector("Display debug:", 0, {"none","Blur","^^+Laplacian","^^+Thresh","^^+Dilate","^^+Exclusion"});
    connect(debugDisp, SIGNAL(changed(int)), this, SLOT(onDebugIndexChanged(int)));
    layout->addWidget(debugDisp);
    layout->addWidget(new hline);
    findf_Blur=new val_selector(2, "pgDepthEval_findf_Blur", "Gaussian Blur Sigma: ", 0, 100, 1, 0, {"px"});
    connect(findf_Blur, SIGNAL(changed()), this, SLOT(onChanged()));
    findf_Thrs=new val_selector(0.2, "pgDepthEval_findf_Thrs", "2nd Derivative Exclusion Threshold: ", 0, 1, 4);
    connect(findf_Thrs, SIGNAL(changed()), this, SLOT(onChanged()));
    findf_Dill=new val_selector(0, "pgDepthEval_findf_Dill", "Dillation: ", 0, 100, 0, 0, {"px"});                  //TODO turn this into radius in um / add option
    connect(findf_Dill, SIGNAL(changed()), this, SLOT(onChanged()));
    findf_Dill->setToolTip("Only used for testing: each call to depth eval should specify a dilation radius corresponding to the beam radius or whatever.");
    layout->addWidget(findf_Blur);
    layout->addWidget(findf_Thrs);
    layout->addWidget(findf_Dill);
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
    if(src==nullptr || debugIndex<0 || debugIndex==0 || debugIndex>5) return src;

    res.max=src->max; res.min=src->min;
    res.pos[0]=src->pos[0]; res.XYnmppx=src->XYnmppx;
    res.pos[1]=src->pos[1]; res.pos[2]=src->pos[2];
    res.tiltCor[0]=src->tiltCor[0]; res.tiltCor[1]=src->tiltCor[1];
    src->depth.copyTo(res.depth);
    src->mask.copyTo(res.mask);
    src->maskN.copyTo(res.maskN);
    double sigma=findf_Blur->val;
    int ksize=sigma*5;
    if(!(ksize%2)) ksize++;
    res.depth.setTo(res.max+abs(res.max-res.min)/2,res.mask);    //if you blur values with infinite... it spills out of the mask
    cv::GaussianBlur(res.depth, res.depth, cv::Size(ksize, ksize), sigma, sigma);
    cv::Point  ignore;
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
    int dilation_size=findf_Dill->val;
    if(dilation_size>0){
        cv::Mat element=cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2*dilation_size+1,2*dilation_size+1), cv::Point(dilation_size,dilation_size));
        cv::dilate(res.mask, res.mask, element);
    }
    bitwise_not(res.mask, res.maskN);
    if(debugIndex==4)  return &res;
    pgBGUI->drawBound(&res.mask, res.XYnmppx, true);
    if(debugIndex==5)  return &res;
}

cv::Mat pgDepthEval::getMaskFlatness(const pgScanGUI::scanRes* src, double XYnmppx, int dil, double thresh, double blur){
    if(dil==-1) dil=findf_Dill->val;
    if(thresh==-1) thresh=findf_Thrs->val;
    if(blur==-1) blur=findf_Blur->val;

    cv::Mat retMask;
    src->mask.copyTo(retMask);

    cv::UMat mat;
    src->depth.copyTo(mat);
    double sigma=blur;
    int ksize=sigma*5;
    if(!(ksize%2)) ksize++;
    mat.setTo(src->max+abs(src->max-src->min)/2,src->mask);
    cv::GaussianBlur(mat, mat, cv::Size(ksize, ksize), sigma, sigma);
    cv::Point ignore;
    cv::Laplacian(mat, mat, CV_32F);
    cv::divide(mat,8,mat);  //should make the unit mm/mm, if its too slow remove this
    cv::UMat umask, umaskT;
    cv::compare(mat,  thresh, umask , cv::CMP_GT);
    cv::compare(mat, -thresh, umaskT, cv::CMP_LT);
    retMask.setTo(cv::Scalar::all(255), umask);
    retMask.setTo(cv::Scalar::all(255), umaskT);
    if(dil>0){
        cv::Mat element=cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2*dil+1,2*dil+1), cv::Point(dil,dil));
        cv::dilate(retMask, retMask, element);
    }
    pgBGUI->drawBound(&retMask, XYnmppx, true);
    return retMask;
}

