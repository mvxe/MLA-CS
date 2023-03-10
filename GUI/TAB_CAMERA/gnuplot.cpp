#include "gnuplot.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include <vector>
#include "UTIL/pipe.h"
#include <QString>

tabCamGnuplot::tabCamGnuplot(checkbox_gs* showAbsHeight): showAbsHeight(showAbsHeight){
    layout=new QVBoxLayout;
    this->setLayout(layout);

    layout->addWidget(new QLabel("2D plot:"));
    ticsFontSize=new val_selector(10, "Tics Font Size: ", 1, 100, 1);
    conf["ticsFontSize"]=ticsFontSize;
    layout->addWidget(ticsFontSize);
    xyLabelFontSize=new val_selector(10, "XY Label Font Size: ", 1, 100, 1);
    conf["xyLabelFontSize"]=xyLabelFontSize;
    layout->addWidget(xyLabelFontSize);
    lpColor=new val_selector(3, "Line/Point Color: ", -1, 7, 0);
    conf["lpColor"]=lpColor;
    layout->addWidget(lpColor);
    lineWidth=new val_selector(1, "Line Width: ", 0.1, 10, 1);
    conf["lineWidth"]=lineWidth;
    layout->addWidget(lineWidth);
    pointSize=new val_selector(1, "Point Size: ", 0.1, 20, 1);
    conf["pointSize"]=pointSize;
    layout->addWidget(pointSize);
    pointType=new val_selector(6, "Point Type: ", 0, 15, 0);
    conf["pointType"]=pointType;
    layout->addWidget(pointType);

    layout->addWidget(new hline);
    layout->addWidget(new QLabel("3D plot:"));
    d3ticsFontSize=new val_selector(10, "Tics Font Size: ", 1, 100, 1);
    conf["d3ticsFontSize"]=d3ticsFontSize;
    layout->addWidget(d3ticsFontSize);
    d3xyzLabelFontSize=new val_selector(10, "XYZ Label Font Size: ", 1, 100, 1);
    conf["d3xyzLabelFontSize"]=d3xyzLabelFontSize;
    layout->addWidget(d3xyzLabelFontSize);
    d3paletteR=new val_selector( 7, "RGB Formula Palette R: ", -36, 36, 0);
    conf["d3paletteR"]=d3paletteR;
    layout->addWidget(d3paletteR);
    d3paletteG=new val_selector( 5, "RGB Formula Palette G: ", -36, 36, 0);
    conf["d3paletteG"]=d3paletteG;
    layout->addWidget(d3paletteG);
    d3paletteB=new val_selector(15, "RGB Formula Palette B: ", -36, 36, 0);
    conf["d3paletteB"]=d3paletteB;
    layout->addWidget(d3paletteB);
    std::string tooltip=util::toString( "Palette RGB values: \n",
                                        "7,5,15   ... traditional pm3d (black-blue-red-yellow)\n",
                                        "3,11,6   ... green-red-violet\n",
                                        "23,28,3  ... ocean (green-blue-white); try also all other permutations\n",
                                        "21,22,23 ... hot (black-red-yellow-white)\n",
                                        "30,31,32 ... color printable on gray (black-blue-violet-yellow-white)\n",
                                        "33,13,10 ... rainbow (blue-green-yellow-red)\n",
                                        "34,35,36 ... AFM hot (black-red-yellow-white)\n",
                                        "3,2,2    ... red-yellow-green-cyan-blue-magenta-red\n",
                                        "For more start gnuplot and run \"help palette rgbformulae\" and \"show palette rgbformulae\" ");
    d3paletteR->setToolTip(QString::fromStdString(tooltip));
    d3paletteG->setToolTip(QString::fromStdString(tooltip));
    d3paletteB->setToolTip(QString::fromStdString(tooltip));
    viewZoom=new val_selector(1, "View Zoom: ", 0.1, 5, 2);
    conf["viewZoom"]=viewZoom;
    layout->addWidget(viewZoom);
    scaleZ=new val_selector(1, "Scale Z: ", 0.01, 10, 3);
    conf["scaleZ"]=scaleZ;
    layout->addWidget(scaleZ);
    equalizeXYZ=new checkbox_gs(false,"Equalize XYZ");
    conf["equalizeXYZ"]=equalizeXYZ;
    layout->addWidget(equalizeXYZ);
}

void tabCamGnuplot::plotLine(const pgScanGUI::scanRes* scan, const cv::Point& start, const cv::Point& end, int type){
    if(scan==nullptr) return;
    gnuplot a;
    a.POUT<<util::toString( "set terminal wxt enhanced\n",
                            "set tics font \"Helvetica,",ticsFontSize->val,"\"\n",
                            "set xlabel font \"Helvetica,",xyLabelFontSize->val,"\"\n",
                            "set ylabel font \"Helvetica,",xyLabelFontSize->val,"\"\n",
                            "set xlabel \"Position (um)\"\n",
                            "set ylabel \"Height (nm)\"\n",
                            "set xzeroaxis\n",
                            "plot [0:",sqrt(pow(start.x-end.x,2)+pow(start.y-end.y,2))*scan->XYnmppx/1000,"] \"-\" using 1:2 w lp lc ",lpColor->val," pt ",pointType->val," ps ",pointSize->val," lw ",lineWidth->val," notitle\n");
    streamLine(&a.POUT, scan, start, end, type);
    a.POUT<<"e\n";
    a.POUT.flush();
}
void tabCamGnuplot::saveLine(const pgScanGUI::scanRes* scan, const cv::Point& start, const cv::Point& end, int type){
    std::string fileName=QFileDialog::getSaveFileName(this,"Select file for saving line data.", "","Text (*.txt)").toStdString();
    if(scan==nullptr || fileName.empty()) return;
    if(fileName.find(".txt")==std::string::npos) fileName+=".txt";
    std::ofstream wfile(fileName);
    wfile<<"# (um) (nm)\n";
    streamLine(&wfile, scan, start, end, type);
    wfile.close();
}
void tabCamGnuplot::plotRoi (const pgScanGUI::scanRes* scan, const cv::Rect& roi, int type){
    if(scan==nullptr) return;
    gnuplot a;
    a.POUT<<util::toString( "set terminal wxt enhanced\n",
                            "set tics font \"Helvetica,",d3ticsFontSize->val,"\"\n",
                            "set xlabel font \"Helvetica,",d3xyzLabelFontSize->val,"\"\n",
                            "set ylabel font \"Helvetica,",d3xyzLabelFontSize->val,"\"\n",
                            "set cblabel font \"Helvetica,",d3xyzLabelFontSize->val,"\"\n",
                            "set xlabel \"XPosition (um)\"\n",
                            "set ylabel \"YPosition (um)\"\n");
    if(equalizeXYZ->val) a.POUT<<util::toString("set cblabel \"Height (um)\"\n",
                                                "set view equal xyz\n",
                                                "unset ztics\n");
    else a.POUT<<util::toString("set cblabel \"Height (nm)\"\n",
                                "set view equal xy\n");
    bool sah=showAbsHeight->val;
    cv::Mat data;
    cv::Mat maskN(scan->maskN, roi);
    if(type==1 || (type==2 && scan->depthSS.empty()) || (type==3 && scan->refl.empty())){
        data=cv::Mat(scan->depth, roi);
    }else if(type==2){
        cv::divide(cv::Mat(scan->depthSS, roi),scan->avgNum-1,data);
        cv::sqrt(data,data);
    }else if(type==3){
        cv::Mat(scan->refl, roi).convertTo(data,CV_32F);
        maskN.setTo(cv::Scalar(255));
    }

    double min, max;
    cv::minMaxLoc(data, &min, &max, nullptr, nullptr, maskN);
    a.POUT<<"set title \"min= "<<min<<"\"\n";
    a.POUT<<util::toString( "set xyplane at 0\n",
                            "set hidden3d\n",
                            "set view ,,",viewZoom->val,",",scaleZ->val,"\n",
                            "set palette rgb ",d3paletteR->val,",",d3paletteG->val,",",d3paletteB->val,"\n",
                            "splot [0:",(cv::Mat(scan->depth, roi).cols-1)*scan->XYnmppx/1000,"][0:",(cv::Mat(scan->depth, roi).rows-1)*scan->XYnmppx/1000,"] \"-\" matrix using ($1*",scan->XYnmppx/1000,"):($2*",scan->XYnmppx/1000,"):3 w pm3d pt 6 notitle\n");
    for(int j=data.rows-1;j>=0;j--){
        for(int i=0;i!=data.cols;i++){
            if(i!=0) a.POUT<<" ";
            if(!maskN.at<uchar>(j,i)) a.POUT<<"nan";
            else a.POUT<<(data.at<float>(j,i)-(sah?0:min))*(equalizeXYZ->val?0.001:1);
        }
        a.POUT<<"\n";
    }
    a.POUT<<"e\ne\n";
    a.POUT.flush();
}

void tabCamGnuplot::streamLine(std::ostream *stream, const pgScanGUI::scanRes* scan, const cv::Point& start, const cv::Point& end, int type){
    if(scan==nullptr) return;
    cv::Mat data;
    cv::Mat mask(scan->mask);
    if(type==1 || (type==2 && scan->depthSS.empty()) || (type==3 && scan->refl.empty())){
        data=scan->depth;
    }else if(type==2){
        cv::divide(scan->depthSS,scan->avgNum-1,data);
        cv::sqrt(data,data);
    }else if(type==3){
        scan->refl.convertTo(data,CV_32F);
        mask.setTo(cv::Scalar(0));
    }

    float len;
    len=sqrt(pow(end.x-start.x,2)+pow(end.y-start.y,2));
    std::cerr<<"len= "<<len<<"\n";
    if((int)len<=0) return;
    std::vector<float> lineData;
    std::vector<bool> lineMask;
    lineData.reserve((int)len+1);
    double angle=atan2(end.y-start.y,end.x-start.x);
    double cosang=cos(angle);
    double sinang=sin(angle);
    for(int i=0;i<=len;i++){
        float _X,_Y;
        float _Xfrac=std::modf(start.x+i*cosang,&_X);
        float _Yfrac=std::modf(start.y+i*sinang,&_Y);
        float val=data.at<float>(_Y,_X)*(1-_Xfrac)*(1-_Yfrac);
        bool bmask=mask.at<uchar>(_Y,_X);
        if(_Xfrac!=0){
            val+=data.at<float>(_Y,_X+1)*_Xfrac*(1-_Yfrac);
            bmask|=mask.at<uchar>(_Y,_X+1);
        }
        if(_Yfrac!=0){
            val+=data.at<float>(_Y+1,_X)*(1-_Xfrac)*_Yfrac;
            bmask|=mask.at<uchar>(_Y+1,_X);
        }
        if(_Xfrac!=0&&_Yfrac!=0){
            val+=data.at<float>(_Y+1,_X+1)*_Xfrac*_Yfrac;
            bmask|=mask.at<uchar>(_Y+1,_X+1);
        }
        lineData.emplace_back(val);
        lineMask.emplace_back(bmask);
    }
    if(!showAbsHeight->val){
        float minval=std::numeric_limits<float>::max();
        for(int i=0;i<=len;i++) if(lineData[i]<minval) minval=lineData[i];
        for(int i=0;i<=len;i++) lineData[i]-=minval;
    }
    for(int i=0;i<=len;i++)
        *stream<<i*scan->XYnmppx/1000<<" "<<(lineMask[i]?"nan":util::toString(lineData[i]))<<"\n";
}
