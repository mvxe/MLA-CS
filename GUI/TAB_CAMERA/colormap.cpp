#include "colormap.h"
#include "GUI/gui_includes.h"

colorMap::colorMap(smp_selector* cm_sel, cv::Scalar& exclColor, checkbox_gs* showAbsHeight, pgScanGUI* pgSGUI, pgTiltGUI* pgTGUI): cm_sel(cm_sel), exclColor(exclColor), showAbsHeight(showAbsHeight), pgSGUI(pgSGUI), pgTGUI(pgTGUI){
    layout=new QVBoxLayout;
    this->setLayout(layout);
    fontFace=new smp_selector("Font: ", 0, OCV_FF::qslabels());
    conf["fontFace"]=fontFace;
    connect(fontFace, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(fontFace);
    fontSize=new val_selector(1., "Font Size for Display: ", 0, 10., 2);
    conf["fontSize"]=fontSize;
    connect(fontSize, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(fontSize);
    fontThickness=new val_selector(1, "Font Thickness: ", 1, 10, 0, 0, {"px"});
    conf["fontThickness"]=fontThickness;
    connect(fontThickness, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(fontThickness);
    barWidth=new val_selector(20, "ColorBar width: ", 3, 100, 0, 0, {"px"});
    conf["barWidth"]=barWidth;
    connect(barWidth, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(barWidth);
    barGap=new val_selector(10, "Image-ColorBar Gap: ", 0, 100, 0, 0, {"px"});
    conf["barGap"]=barGap;
    connect(barGap, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(barGap);
    textOffset=new val_selector(2, "Text Offset: ", 0, 100, 0, 0, {"px"});
    conf["textOffset"]=textOffset;
    connect(textOffset, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(textOffset);
    displayANTicks=new val_selector(10, "Approximate Number of Ticks for Display: ", 0, 100, 0);
    conf["displayANTicks"]=displayANTicks;
    connect(displayANTicks, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(displayANTicks);

    xysbar_unit=new val_selector(0, "XY Scalebar Unit: ", -1000, 1000, 2, 0, {"um"});
    conf["xysbar_unit"]=xysbar_unit;
    connect(xysbar_unit, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(xysbar_unit);
    xysbar_thck=new val_selector(1, "XY Scalebar Thickness: ", 1, 100, 0, 0, {"px"});
    conf["xysbar_thck"]=xysbar_thck;
    connect(xysbar_thck, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(xysbar_thck);
    xysbar_txtoffset=new val_selector(2, "XY Scalebar Text Offset: ", 0, 100, 0, 0, {"px"});
    conf["xysbar_txtoffset"]=xysbar_txtoffset;
    connect(xysbar_txtoffset, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(xysbar_txtoffset);
    xysbar_corner=new smp_selector("Select XY Scalebar reference position: ", 0, {"center","top","bottom","left","right","top-left","top-right","bottom-left","bottom-right"});
    conf["xysbar_corner"]=xysbar_corner;
    connect(xysbar_corner, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(xysbar_corner);
    xysbar_xoffset=new val_selector(0, "Horizontal Offset for XY Scalebar: ", -10000, 10000, 0, 0, {"px"});
    conf["xysbar_xoffset"]=xysbar_xoffset;
    connect(xysbar_xoffset, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(xysbar_xoffset);
    xysbar_yoffset=new val_selector(0, "Vertical Offset for XY Scalebar: ", -10000, 10000, 0, 0, {"px"});
    conf["xysbar_yoffset"]=xysbar_yoffset;
    connect(xysbar_yoffset, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(xysbar_yoffset);
    xysbar_color_inv=new checkbox_gs(false,"Invert XY Scalebar Color");
    conf["xysbar_inverclr"]=xysbar_color_inv;
    connect(xysbar_color_inv, SIGNAL(changed()), this, SLOT(onChanged()));
    layout->addWidget(xysbar_color_inv);

    layout->addWidget(new hline);
    exportSet4Whole=new checkbox_gs(false,"Use Setting Above When Exporting Whole Image");
    conf["exportSet4Whole"]=exportSet4Whole;
    exportSet4WholeVal=&exportSet4Whole->val;
    layout->addWidget(exportSet4Whole);
    fontSizeExport=new val_selector(1., "Font Size for Export: ", 0, 10., 2);
    conf["fontSizeExport"]=fontSizeExport;
    layout->addWidget(fontSizeExport);
    exportANTicks=new val_selector(10, "Approximate Number of Ticks for Export: ", 0, 100, 0);
    conf["exportANTicks"]=exportANTicks;
    layout->addWidget(exportANTicks);
    xysbar_unit_Export=new val_selector(0, "XY Scalebar Unit for Export: ", -1000, 1000, 2, 0, {"um"});
    conf["xysbar_unit_Export"]=xysbar_unit_Export;
    layout->addWidget(xysbar_unit_Export);
    xysbar_corner_Export=new smp_selector("Select XY Scalebar reference position: ", 0, {"center","top","bottom","left","right","top-left","top-right","bottom-left","bottom-right"});
    conf["xysbar_corner_Export"]=xysbar_corner_Export;
    layout->addWidget(xysbar_corner_Export);
    xysbar_xoffset_Export=new val_selector(0, "Horizontal Offset for XY Scalebar for Export: ", -10000, 10000, 0, 0, {"px"});
    conf["xysbar_xoffset_Export"]=xysbar_xoffset_Export;
    layout->addWidget(xysbar_xoffset_Export);
    xysbar_yoffset_Export=new val_selector(0, "Vertical Offset for XY Scalebar for Export: ", -10000, 10000, 0, 0, {"px"});
    conf["xysbar_yoffset_Export"]=xysbar_yoffset_Export;
    layout->addWidget(xysbar_yoffset_Export);

    layout->addWidget(new hline);
    QLabel* tlab=new QLabel("This calibration is not used, however, since it is calculated using the depth measurement it may be used to verify the correctness of the depth calibration by comparing it to the calibration done in Move settings.");
    tlab->setWordWrap(true);
    layout->addWidget(tlab);
    XYnmppx=new val_selector(10, "XY calibration: ", 0, 1000, 6, 0, {"nm/px"});
    conf["XYnmppx"]=XYnmppx;
    layout->addWidget(XYnmppx);
    calibXY=new QPushButton;
    calibXY->setText("Calibrate YZ Scan");
    calibXY->setCheckable(true);
    connect(calibXY, SIGNAL(toggled(bool)), this, SLOT(onCalibrateXY(bool)));
    tilt=new val_selector(1., "Ammount of tilt: ", -10000., 10000., 0);
    conf["tilt"]=tilt;
    movTilt=new QPushButton;
    movTilt->setText("Tilt");
    movTilt->setCheckable(true);
    connect(movTilt, SIGNAL(toggled(bool)), this, SLOT(onMovTilt(bool)));
    layout->addWidget(new twid(calibXY, tilt, movTilt));

    QLabel* txt=new QLabel("(Scan -> Tilt -> Refocus -> Scan -> Tilt -> Refocus)");
    layout->addWidget(txt);
}

void colorMap::colormappize(const cv::Mat* src, cv::Mat* dst, const cv::Mat* mask, double min, double max, double XYnmppx, bool excludeOutOfRange, bool isForExport, std::string label, bool isReflectivity){
    int vsize=src->rows+2;
    int hsize=src->cols+2;
    double range=max-min;
    double div;
    double tick=1;
    double diff[2]={DBL_MAX,DBL_MAX};
    bool done=false; int ignore;

    int nticks=-1;
    int maxNticks=0;
    if(isForExport && exportANTicks->val!=0){
        div=range/exportANTicks->val;
        maxNticks=5*exportANTicks->val;
    }
    else if(displayANTicks->val!=0){
        div=range/displayANTicks->val;
        maxNticks=5*displayANTicks->val;
    }
    else nticks=0;

    int tmp = cv::getTextSize("0", OCV_FF::ids[fontFace->index], isForExport?fontSizeExport->val:fontSize->val, fontThickness->val, &ignore).height+2;
    double minDiv=(double)tmp/(src->rows-tmp)*range;
    if(div<minDiv){
        div=minDiv;
    }

    bool sah;   //show abs height, else show relative starting from 0
    double minRnd;  //fist tick if abs height

    int textMaxWidth=1;
    int textMaxHeight=1;  //all should be the same though
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
        cv::Size size=cv::getTextSize(label, OCV_FF::ids[fontFace->index], isForExport?fontSizeExport->val:fontSize->val, fontThickness->val, &ignore);
        cblabel=cv::Mat(size.height*2,size.width+2,CV_8UC4,{0,0,0,0});
        cv::putText(cblabel,label, {0,4*size.height/3}, OCV_FF::ids[fontFace->index], isForExport?fontSizeExport->val:fontSize->val, {0,0,0,255}, fontThickness->val, cv::LINE_AA);
        cv::rotate(cblabel,cblabel,cv::ROTATE_90_COUNTERCLOCKWISE);
        sah=showAbsHeight->val|isReflectivity;
        if(sah) minRnd=ceil(min/tick)*tick;
        nticks=0;
        for(int i=0;i!=maxNticks;i++){
            double tickn=sah?(minRnd+i*tick):(i*tick);
            if(tickn>(sah?max:range)) break;
            nticks++;
            cv::Size size=cv::getTextSize(util::toString(tickn), OCV_FF::ids[fontFace->index], isForExport?fontSizeExport->val:fontSize->val, fontThickness->val, &ignore);
            if(textMaxWidth<size.width) textMaxWidth=size.width;
            if(textMaxHeight<size.height) textMaxHeight=size.height;
        }
    }
    int marginY=textMaxHeight;  //we make it twice the text max height

    int cbhsize=barWidth->val, Gap=barGap->val, textHDis=textOffset->val;
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
    *dst=cv::Mat(vsize+2*marginY, hsize+Gap+cbhsize+textHDis+textMaxWidth+textHDis+cblabel.cols, CV_8UC4, {255,255,255,0});

    // adding scalebar
    double lxysbar_unit=isForExport?xysbar_unit_Export->val:xysbar_unit->val;
    if(lxysbar_unit!=0 && XYnmppx){
        cv::Size size=cv::getTextSize(util::toString(xysbar_unit->val," um"), OCV_FF::ids[fontFace->index], isForExport?fontSizeExport->val:fontSize->val, fontThickness->val, &ignore);
        int xofs=temp.cols/2+(isForExport?xysbar_xoffset_Export->val:xysbar_xoffset->val);
        int yofs=temp.rows/2+(isForExport?xysbar_yoffset_Export->val:xysbar_yoffset->val);

        int in=isForExport?xysbar_corner_Export->index:xysbar_corner->index;
        if(in==1 || in==5 || in==6) yofs-=temp.rows/2;  //top
        if(in==2 || in==7 || in==8) yofs+=temp.rows/2;  //bottom
        if(in==3 || in==5 || in==7) xofs-=temp.cols/2;  //left
        if(in==4 || in==6 || in==8) xofs+=temp.cols/2;  //right

        cv::Scalar frontC,backC;
        frontC=xysbar_color_inv->val?cv::Scalar{255,255,255}:cv::Scalar{0,0,0};
        backC=xysbar_color_inv->val?cv::Scalar{0,0,0}:cv::Scalar{255,255,255};
        for(int i=1;i>=0;i--){
            cv::putText(temp,util::toString(lxysbar_unit," um"), {xofs-size.width/2, yofs-(int)xysbar_txtoffset->val}, OCV_FF::ids[fontFace->index], isForExport?fontSizeExport->val:fontSize->val, i?backC:frontC, fontThickness->val+i, cv::LINE_AA);
            cv::rectangle(temp, {xofs-(int)(lxysbar_unit*1000/XYnmppx/2)-i, yofs-i, (int)(lxysbar_unit*1000/XYnmppx)+2*i, (int)xysbar_thck->val+2*i}, i?backC:frontC,-1);
        }
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
        double tickn=sah?(minRnd+i*tick):(i*tick);
        int ypos=vsize-1+marginY-((tickn-(sah?min:0))*(vsize-1)/range);
        cv::putText(*dst,util::toString(tickn), cv::Point(hsize+Gap+textHDis+cbhsize,ypos+textMaxHeight/2), OCV_FF::ids[fontFace->index], isForExport?fontSizeExport->val:fontSize->val, {0,0,0,255}, fontThickness->val, cv::LINE_AA);
        cv::line(*dst, {hsize+Gap,ypos}, {hsize+Gap+(cbhsize-1)/3,ypos}, {0,0,0,255}, 1);
        cv::line(*dst, {hsize+Gap+2*(cbhsize-1)/3,ypos}, {hsize+Gap+cbhsize-1,ypos}, {0,0,0,255}, 1);
    }
    if(nticks!=0)
        if(dst->cols>=hsize+Gap+textHDis+cbhsize+textMaxWidth+textHDis+cblabel.cols && dst->rows>=dst->rows/2-cblabel.rows/2+cblabel.rows)
            cblabel.copyTo(cv::Mat(*dst,{hsize+Gap+textHDis+cbhsize+textMaxWidth+textHDis,dst->rows/2-cblabel.rows/2,cblabel.cols,cblabel.rows}));
    _changed=false;
}

void colorMap::draw_bw_target(cv::Mat* src, float dX, float dY){
    const int targWidth=2;
    const int targLenght=20;
    const int targDis=10;
    int xofs=src->cols/2+dX;
    int yofs=src->rows/2+dY;
    cv::Scalar frontC,backC;
    frontC=xysbar_color_inv->val?cv::Scalar{255}:cv::Scalar{0};
    backC=xysbar_color_inv->val?cv::Scalar{0}:cv::Scalar{255};
    for(int i=1;i>=0;i--){
        cv::rectangle(*src, {xofs-targLenght-targDis-i, yofs-targWidth/2-i, targLenght+2*i, targWidth+2*i}, i?backC:frontC,-1);
        cv::rectangle(*src, {xofs+targDis-i, yofs-targWidth/2-i, targLenght+2*i, targWidth+2*i}, i?backC:frontC,-1);
        cv::rectangle(*src, {xofs-targWidth/2-i, yofs-targLenght-targDis-i, targWidth+2*i, targLenght+2*i}, i?backC:frontC,-1);
        cv::rectangle(*src, {xofs-targWidth/2-i, yofs+targDis-i, targWidth+2*i, targLenght+2*i}, i?backC:frontC,-1);
    }
}
void colorMap::draw_bw_scalebar(cv::Mat* src, double XYnmppx){
    double lxysbar_unit=xysbar_unit->val; int ignore;
    if(lxysbar_unit!=0 && XYnmppx!=0){
        cv::Size size=cv::getTextSize(util::toString(xysbar_unit->val," um"), OCV_FF::ids[fontFace->index], fontSize->val, fontThickness->val, &ignore);
        int xofs=src->cols/2+xysbar_xoffset->val;
        int yofs=src->rows/2+xysbar_yoffset->val;
        int in=xysbar_corner->index;
        if(in==1 || in==5 || in==6) yofs-=src->rows/2;  //top
        if(in==2 || in==7 || in==8) yofs+=src->rows/2;  //bottom
        if(in==3 || in==5 || in==7) xofs-=src->cols/2;  //left
        if(in==4 || in==6 || in==8) xofs+=src->cols/2;  //right
        cv::Scalar frontC,backC;
        frontC=xysbar_color_inv->val?cv::Scalar{255}:cv::Scalar{0};
        backC=xysbar_color_inv->val?cv::Scalar{0}:cv::Scalar{255};
        for(int i=1;i>=0;i--){
            cv::putText(*src,util::toString(lxysbar_unit," um"), {xofs-size.width/2, yofs-(int)xysbar_txtoffset->val}, OCV_FF::ids[fontFace->index], fontSize->val, i?backC:frontC, fontThickness->val+i, cv::LINE_AA);
            cv::rectangle(*src, {xofs-(int)(lxysbar_unit*1000/XYnmppx/2)-i, yofs-i, (int)(lxysbar_unit*1000/XYnmppx)+2*i, (int)xysbar_thck->val+2*i}, i?backC:frontC,-1);
        }
    }
}
void colorMap::draw_color_box(cv::Mat* src, int x0, int x1, int y0, int y1){
    cv::rectangle(*src, {x0+1,y0+1},{x1,y1}, exclColor);
}

void colorMap::onChanged(){
    _changed=true;
}

void colorMap::onCalibrateXY(bool state){
    varShareClient<pgScanGUI::scanRes>* res=pgSGUI->result.getClient();
    if(state){      //start
        pgSGUI->doOneRound();
        while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
        const pgScanGUI::scanRes* ress=res->get();
        phiX0=ress->tiltCor[0];
        phiY0=ress->tiltCor[1];
        phiXR=0;
        phiYR=0;
    }else{
        pgSGUI->doOneRound();
        while(pgSGUI->measurementInProgress) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);   //wait till measurement is done
        const pgScanGUI::scanRes* ress=res->get();
        phiX1=ress->tiltCor[0];
        phiY1=ress->tiltCor[1];
        double phiD, phiR;
        phiD=sqrt(pow(phiX1-phiX0,2)+pow(phiY1-phiY0,2));
        phiR=sqrt(pow(phiXR,2)+pow(phiYR,2));
        double result;
        result=phiD/phiR;
        XYnmppx->setValue(abs(result));
    }
    delete res;
}

void colorMap::onMovTilt(bool state){
    if(state) {
        pgTGUI->doTilt( tilt->val, tilt->val, false);
        phiXR+=tilt->val/62230;
        phiYR+=tilt->val/62230;
    }
    else{ pgTGUI->doTilt(-tilt->val,-tilt->val, false);
        phiXR-=tilt->val/62230;
        phiYR-=tilt->val/62230;
    }
}
