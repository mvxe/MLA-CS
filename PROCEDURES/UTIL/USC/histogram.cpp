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
    _lPcnt_sel=new QDoubleSpinBox; _lPcnt_sel->setPrefix("Min: "); _lPcnt_sel->setSuffix("%"); _lPcnt_sel->setMinimum(0);      _lPcnt_sel->setMaximum(_hPcnt); _lPcnt_sel->setValue(_lPcnt);
    connect(_lPcnt_sel, SIGNAL(valueChanged(double)), this, SLOT(onValueChanged_lPcnt(double)));
    _hPcnt_sel=new QDoubleSpinBox; _hPcnt_sel->setPrefix("Max: "); _hPcnt_sel->setSuffix("%"); _hPcnt_sel->setMinimum(_lPcnt); _hPcnt_sel->setMaximum(100);    _hPcnt_sel->setValue(_hPcnt);
    connect(_hPcnt_sel, SIGNAL(valueChanged(double)), this, SLOT(onValueChanged_hPcnt(double)));
    btnLayout->addWidget(_lPcnt_sel);
    btnLayout->addWidget(_hPcnt_sel);
    outOfRangeToExcl=new QCheckBox("Exl.");
    outOfRangeToExcl->setChecked(cbOORtE);
    connect(outOfRangeToExcl, SIGNAL(stateChanged(int)), this, SLOT(onValueChanged_cbox(int)));
    btnLayout->addWidget(outOfRangeToExcl);
    btnLayout->addStretch();
    btnLayout->setMargin(0);
    layout->setMargin(0);
}

void pgHistogrameGUI::updateImg(const pgScanGUI::scanRes* res ,double *rmin, double *rmax){
    if(res!=nullptr){
        cv::Mat hist;
        int histSize[]={Hsize}; float hranges[]={(float)res->min, (float)res->max}; const float* ranges[]={hranges}; int channels[]={0};
        cv::calcHist(&res->depth, 1, channels, res->maskN, hist, 1, histSize, ranges);

        //calc min,max from _lPcnt,_hPcnt
        float sum=cv::sum(hist).val[0];
        int imin=0, imax=Hsize-1;
        float sumc=0;
        for(int i=0;i!=Hsize;i++){
            sumc+=hist.at<float>(i);
            if(sumc>=_lPcnt/100*sum) {imin=i; break;}
        } sumc=0;
        for(int i=Hsize-1;i!=-1;i--){
            sumc+=hist.at<float>(i);
            if(sumc>=(100-_hPcnt)/100*sum) {imax=i; break;}
        }
        if(rmin!=nullptr) *rmin=res->min+imin*(res->max-res->min)/(Hsize-1);
        if(rmax!=nullptr) *rmax=res->min+imax*(res->max-res->min)/(Hsize-1);

        cv::Mat lin(1,Hsize,CV_8U);
        for(int i=0;i!=imin;i++)      lin.at<uchar>(i)=0;
        for(int i=imin;i!=imax;i++)   lin.at<uchar>(i)=(i-imin)*255/(imax-imin);
        for(int i=imax;i!=Hsize;i++)  lin.at<uchar>(i)=255;

        cv::applyColorMap(lin, lin, OCV_CM::ids[cm_sel->index]);
        cv::cvtColor(lin, lin, cv::COLOR_BGR2RGBA);
        if(cbOORtE){
            for(int i=0;i!=imin;i++)      lin.at<cv::Vec4b>(i)={(uchar)exclColor.val[2],(uchar)exclColor.val[1],(uchar)exclColor.val[0],255};
            for(int i=imax;i!=Hsize;i++)  lin.at<cv::Vec4b>(i)={(uchar)exclColor.val[2],(uchar)exclColor.val[1],(uchar)exclColor.val[0],255};
        }
        cv::Mat display(Vsize+1,Hsize,CV_8UC4,{0,0,0,0});
        double minh,maxh;
        cv::minMaxIdx(hist,&minh,&maxh);
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
    _hPcnt=value;
    _lPcnt_sel->setMaximum(_hPcnt);
    _changed=true;
}
void pgHistogrameGUI::onValueChanged_lPcnt(double value){
    _lPcnt=value;
    _hPcnt_sel->setMinimum(_lPcnt);
    _changed=true;
}
void pgHistogrameGUI::onValueChanged_cbox(int state){
    cbOORtE=outOfRangeToExcl->isChecked();
    _changed=true;
}
