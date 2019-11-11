#include "colormap.h"
#include "GUI/gui_includes.h"

colorMap::colorMap(smp_selector* cm_sel, cv::Scalar& exclColor): cm_sel(cm_sel), exclColor(exclColor){
    layout=new QVBoxLayout;
    this->setLayout(layout);
    fontFace=new smp_selector("colorBar_fontFace", "Font: ", 0, OCV_FF::qslabels());
    connect(fontFace, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(fontFace);
    fontSize=new val_selector(1., "colorBar_fontSize", "Font Size for Display: ", 0, 10., 2);
    connect(fontSize, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(fontSize);
    fontSizeExport=new val_selector(1., "colorBar_fontSiz_exporte", "Font Size for Export: ", 0, 10., 2);
    layout->addWidget(fontSizeExport);
    fontThickness=new val_selector(1, "colorBar_fontThickness", "Font Thickness: ", 1, 10, 0, 0, {"px"});
    connect(fontThickness, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(fontThickness);
    barWidth=new val_selector(20, "colorBar_barWidth", "ColorBar width: ", 3, 100, 0, 0, {"px"});
    connect(barWidth, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(barWidth);
    barGap=new val_selector(10, "colorBar_Gap", "Image-ColorBar Gap: ", 0, 100, 0, 0, {"px"});
    connect(barGap, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(barGap);
    textOffset=new val_selector(2, "colorBar_textOffset", "Text Offset: ", 0, 100, 0, 0, {"px"});
    connect(textOffset, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(textOffset);
    displayANTicks=new val_selector(10, "colorBar_displayANTicks", "Approximate Number of Ticks for Display: ", 0, 100, 0);
    connect(displayANTicks, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(displayANTicks);
    exportANTicks=new val_selector(10, "colorBar_exportANTicks", "Approximate Number of Ticks for Export: ", 0, 100, 0);
    layout->addWidget(exportANTicks);
}

void colorMap::colormappize(cv::Mat* src, cv::Mat* dst, cv::Mat* mask, double min, double max, bool excludeOutOfRange, bool isForExport){
    int vsize=src->rows+2;
    int hsize=src->cols+2;
    double range=max-min;
    double div;
    double tick=1;
    double diff[2]={DBL_MAX,DBL_MAX};
    bool done=false; int ignore;

    int nticks=-1;
    if(isForExport && exportANTicks->val!=0) div=range/exportANTicks->val;
    else if(displayANTicks->val!=0) div=range/displayANTicks->val;
    else nticks=0;

    cv::Mat cblabel;
    if(nticks!=0){
        for(int mult=-2;mult!=100;mult++){
            for(auto& val:stdTicks){
                diff[0]=abs(div-val*pow(10,mult));
                if(diff[0]>diff[1]){done=true;break;}
                tick=val*pow(10,mult);
                diff[1]=diff[0];
            }
            if(done)break;
        }
        nticks=range/tick+1;  //rounds down
        //std::cout<<"range= "<<range<<" ,div= "<<div<<" ,tick= "<<tick<<"\n";
        cv::Size size=cv::getTextSize(util::toString("Depth[nm]"), OCV_FF::ids[fontFace->index], isForExport?fontSizeExport->val:fontSize->val, fontThickness->val, &ignore);
        cblabel=cv::Mat(size.height*2,size.width,CV_8UC4,{0,0,0,0});
        cv::putText(cblabel,util::toString("Depth[nm]"), {0,4*size.height/3}, OCV_FF::ids[fontFace->index], isForExport?fontSizeExport->val:fontSize->val, {0,0,0,255}, fontThickness->val, cv::LINE_AA);
        cv::rotate(cblabel,cblabel,cv::ROTATE_90_COUNTERCLOCKWISE);
    }
    int textMaxWidth=1;
    int textMaxHeight=1;  //all should be the same though
    for(int i=0;i!=nticks;i++){
        cv::Size size=cv::getTextSize(util::toString(i*tick), OCV_FF::ids[fontFace->index], isForExport?fontSizeExport->val:fontSize->val, fontThickness->val, &ignore);
        if(textMaxWidth<size.width) textMaxWidth=size.width;
        if(textMaxHeight<size.height) textMaxHeight=size.height;
    }
    int marginY=textMaxHeight;  //we make it twice the text max height

    int cbhsize=barWidth->val, Gap=barGap->val, textHDis=textOffset->val;
    *dst=cv::Mat(vsize+2*marginY, hsize+Gap+cbhsize+textHDis+textMaxWidth+textHDis+cblabel.cols, CV_8UC4, {255,255,255,0});
    cv::Mat temp;
    src->convertTo(temp, CV_8U, ((1<<8)-1)/(max-min),-min*((1<<8)-1)/(max-min));
    cv::applyColorMap(temp, temp, OCV_CM::ids[cm_sel->index]);
    temp.setTo(exclColor, *mask);
    if(excludeOutOfRange){
        cv::Mat mask;
        cv::compare(*src, min, mask, cv::CMP_LT);
        temp.setTo(exclColor, mask);
        cv::compare(*src, max, mask, cv::CMP_GT);
        temp.setTo(exclColor, mask);
    }
    cv::cvtColor(temp, cv::Mat(*dst,{1,marginY+1,hsize-2,vsize-2}), cv::COLOR_BGR2RGBA);
    cv::rectangle(*dst, {0,marginY}, {hsize-1,marginY+vsize-1}, {0,0,0,255}, 1);
    cv::Mat amat(vsize, 1, CV_8U);
    for(int i=1;i!=vsize-1;i++) amat.at<uchar>(vsize-1-i)=i*255/(vsize-2);
    cv::applyColorMap(amat, amat, OCV_CM::ids[cm_sel->index]);
    cv::cvtColor(amat, amat, cv::COLOR_BGR2RGBA);
    cv::repeat(amat, 1, cbhsize, cv::Mat(*dst,{hsize+Gap,marginY,cbhsize,vsize}));
    cv::rectangle(*dst, {hsize+Gap,marginY}, {hsize+Gap+cbhsize-1,vsize-1+marginY}, {0,0,0,255}, 1);

    for(int i=0;i!=nticks;i++){
        int ypos=vsize-1+marginY- (i*tick*(vsize-1)/range);
        cv::putText(*dst,util::toString(i*tick), cv::Point(hsize+Gap+textHDis+cbhsize,ypos+textMaxHeight/2), OCV_FF::ids[fontFace->index], isForExport?fontSizeExport->val:fontSize->val, {0,0,0,255}, fontThickness->val, cv::LINE_AA);
        cv::line(*dst, {hsize+Gap,ypos}, {hsize+Gap+(cbhsize-1)/3,ypos}, {0,0,0,255}, 1);
        cv::line(*dst, {hsize+Gap+2*(cbhsize-1)/3,ypos}, {hsize+Gap+cbhsize-1,ypos}, {0,0,0,255}, 1);
    }
    if(nticks!=0) cblabel.copyTo(cv::Mat(*dst,{hsize+Gap+textHDis+cbhsize+textMaxWidth+textHDis,dst->rows/2-cblabel.rows/2,cblabel.cols,cblabel.rows}));
    _changed=false;
}

void colorMap::onChanged(){
    _changed=true;
}
