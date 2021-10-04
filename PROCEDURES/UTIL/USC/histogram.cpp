#include "histogram.h"
#include "GUI/gui_aux_objects.h"
#include "opencv2/opencv.hpp"
#include "GUI/gui_includes.h"

pgHistogrameGUI::pgHistogrameGUI(int Hsize, int Vsize, smp_selector* cm_sel, cv::Scalar& exclColor): Hsize(Hsize), Vsize(Vsize), cm_sel(cm_sel), exclColor(exclColor){
    layout=new QVBoxLayout;
    btnLayout= new QHBoxLayout;
    this->setLayout(layout);
    imgDisp= new QLabel;
    btnLW=new QWidget;
    layout->addWidget(imgDisp);
    layout->addWidget(btnLW);
    btnLW->setLayout(btnLayout);

    lPcnt=new val_selector(0, "Min: ", 0, 100, 2, 0, {"%"});
    connect(lPcnt, SIGNAL(changed(double)), this, SLOT(onValueChanged_lPcnt(double)));
    btnLayout->addWidget(lPcnt);
    hPcnt=new val_selector(100, "Max: ", 0, 100, 2, 0, {"%"});
    connect(hPcnt, SIGNAL(changed(double)), this, SLOT(onValueChanged_hPcnt(double)));
    btnLayout->addWidget(hPcnt);
    outOfRangeToExcl=new checkbox_gs(false,"Exl");
    connect(outOfRangeToExcl, SIGNAL(changed(bool)), this, SLOT(onValueChanged_cbox(bool)));
    btnLayout->addWidget(outOfRangeToExcl);
    conf["hPcnt"]=hPcnt;
    conf["lPcnt"]=lPcnt;
    conf["outOfRangeToExcl"]=outOfRangeToExcl;
    lPcnt->setMaximum(hPcnt->val);
    hPcnt->setMinimum(lPcnt->val);

    btnLayout->addStretch();
    btnLayout->setMargin(0);
    layout->setMargin(0);
}

void pgHistogrameGUI::updateImg(const pgScanGUI::scanRes* res ,double *rmin, double *rmax, double altMin, double altMax, cv::Mat* altDepth){
    if(res!=nullptr){
        float min,max;
        if(altDepth!=nullptr){
            min=altMin;
            max=altMax;
        }else{
            min=res->min;
            max=res->max;
        }

        cv::Mat hist;
        if(max<=min) max=min+1;
        int histSize[]={Hsize}; float hranges[]={min, max}; const float* ranges[]={hranges}; int channels[]={0};

        cv::calcHist((altDepth!=nullptr)?altDepth:(&res->depth), 1, channels, res->maskN, hist, 1, histSize, ranges);

        //calc min,max from _lPcnt,_hPcnt
        float sum=cv::sum(hist).val[0];
        int imin=0, imax=Hsize-1;
        float sumc=0;
        for(int i=0;i!=Hsize;i++){
            sumc+=hist.at<float>(i);
            if(sumc>=lPcnt->val/100*sum) {imin=i; break;}
        } sumc=0;
        for(int i=Hsize-1;i!=-1;i--){
            sumc+=hist.at<float>(i);
            if(sumc>=(100-hPcnt->val)/100*sum) {imax=i; break;}
        }
        if(rmin!=nullptr) *rmin=min+imin*(max-min)/(Hsize-1);
        if(rmax!=nullptr) *rmax=min+imax*(max-min)/(Hsize-1);

        cv::Mat lin(1,Hsize,CV_8U);
        for(int i=0;i!=imin;i++)      lin.at<uchar>(i)=0;
        for(int i=imin;i!=imax;i++)   lin.at<uchar>(i)=(i-imin)*255/(imax-imin);
        for(int i=imax;i!=Hsize;i++)  lin.at<uchar>(i)=255;

        cv::applyColorMap(lin, lin, OCV_CM::ids[cm_sel->index]);
        cv::cvtColor(lin, lin, cv::COLOR_BGR2RGBA);
        if(outOfRangeToExcl->val){
            for(int i=0;i!=imin;i++)      lin.at<cv::Vec4b>(i)={(uchar)exclColor.val[2],(uchar)exclColor.val[1],(uchar)exclColor.val[0],255};
            for(int i=imax;i!=Hsize;i++)  lin.at<cv::Vec4b>(i)={(uchar)exclColor.val[2],(uchar)exclColor.val[1],(uchar)exclColor.val[0],255};
        }
        cv::Mat display(Vsize+1,Hsize,CV_8UC4,{0,0,0,0});
        double minh,maxh;
        cv::minMaxIdx(hist,&minh,&maxh);
        if(maxh==0) maxh=1;
        for(int i=0;i!=Hsize;i++){
            int lim=hist.at<float>(i)/maxh*Vsize;
            int limPrev, limNext;
            if(i!=0) limPrev=hist.at<float>(i-1)/maxh*Vsize;
            else limPrev=0;
            if(i!=Hsize-1)  limNext=hist.at<float>(i+1)/maxh*Vsize;
            else limNext=0;
            if(limNext<limPrev) limNext=limPrev;
            for(int j=0;j!=lim;j++)
                display.at<cv::Vec4b>(Vsize-1-j,i)=lin.at<cv::Vec4b>(i);
            if(limNext>lim) for(int j=lim;j!=limNext;j++)
                display.at<cv::Vec4b>(Vsize-1-j,i)={0,0,0,255};
            display.at<cv::Vec4b>(Vsize,i)={0,0,0,255};
            display.at<cv::Vec4b>(Vsize-lim,i)={0,0,0,255};
        }
        QImage qimg(display.data, display.cols, display.rows, display.step, QImage::Format_RGBA8888);
        imgDisp->setPixmap(QPixmap::fromImage(qimg));
        _changed=false;
    }
}

void pgHistogrameGUI::onValueChanged_hPcnt(double value){
    lPcnt->setMaximum(hPcnt->val);
    _changed=true;
}
void pgHistogrameGUI::onValueChanged_lPcnt(double value){
    hPcnt->setMinimum(lPcnt->val);
    _changed=true;
}
void pgHistogrameGUI::onValueChanged_cbox(bool state){
    _changed=true;
}
