#include "gnuplot.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include <vector>
#include "UTIL/pipe.h"
#include <QString>

tabCamGnuplot::tabCamGnuplot(){
    layout=new QVBoxLayout;
    this->setLayout(layout);

    layout->addWidget(new QLabel("2D plot:"));
    ticsFontSize=new val_selector(10, "tabCamGnuplot_ticsFontSize", "Tics Font Size: ", 1, 100, 1);
    layout->addWidget(ticsFontSize);
    xyLabelFontSize=new val_selector(10, "tabCamGnuplot_xyLabelFontSize", "XY Label Font Size: ", 1, 100, 1);
    layout->addWidget(xyLabelFontSize);
    lpColor=new val_selector(3, "tabCamGnuplot_lpColor", "Line/Point Color: ", -1, 7, 0);
    layout->addWidget(lpColor);
    lineWidth=new val_selector(1, "tabCamGnuplot_lineWidth", "Line Width: ", 0.1, 10, 1);
    layout->addWidget(lineWidth);
    pointSize=new val_selector(1, "tabCamGnuplot_pointSize", "Point Size: ", 0.1, 20, 1);
    layout->addWidget(pointSize);
    pointType=new val_selector(6, "tabCamGnuplot_pointType", "Point Type: ", 0, 15, 0);
    layout->addWidget(pointType);

    layout->addWidget(new hline);
    layout->addWidget(new QLabel("3D plot:"));
    d3ticsFontSize=new val_selector(10, "tabCamGnuplot_d3ticsFontSize", "Tics Font Size: ", 1, 100, 1);
    layout->addWidget(d3ticsFontSize);
    d3xyzLabelFontSize=new val_selector(10, "tabCamGnuplot_d3xyzLabelFontSize", "XYZ Label Font Size: ", 1, 100, 1);
    layout->addWidget(d3xyzLabelFontSize);
    d3paletteR=new val_selector( 7, "tabCamGnuplot_d3paletteR", "RGB Formula Palette R: ", -36, 36, 0);
    layout->addWidget(d3paletteR);
    d3paletteG=new val_selector( 5, "tabCamGnuplot_d3paletteG", "RGB Formula Palette G: ", -36, 36, 0);
    layout->addWidget(d3paletteG);
    d3paletteB=new val_selector(15, "tabCamGnuplot_d3paletteB", "RGB Formula Palette B: ", -36, 36, 0);
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
    viewZoom=new val_selector(1, "tabCamGnuplot_viewZoom", "View Zoom: ", 0.1, 5, 2);
    layout->addWidget(viewZoom);
    scaleZ=new val_selector(1, "tabCamGnuplot_scaleZ", "Scale Z: ", 0.01, 10, 3);
    layout->addWidget(scaleZ);
    equalizeXYZ=new checkbox_save(false,"tabCamGnuplot_equalizeXYZ","Equalize XYZ");
    layout->addWidget(equalizeXYZ);
}

void tabCamGnuplot::plotLine(const pgScanGUI::scanRes* scan, const cv::Point& start, const cv::Point& end, bool useSD){
    if(scan==nullptr) return;
    gnuplot a;
    a.POUT<<util::toString( "set terminal wxt enhanced\n",
                            "set tics font \"Helvetica,",ticsFontSize->val,"\"\n",
                            "set xlabel font \"Helvetica,",xyLabelFontSize->val,"\"\n",
                            "set ylabel font \"Helvetica,",xyLabelFontSize->val,"\"\n",
                            "set xlabel \"Position [um]\"\n",
                            "set ylabel \"Height [nm]\"\n",
                            "plot \"-\" using 1:2 w lp lc ",lpColor->val," pt ",pointType->val," ps ",pointSize->val," lw ",lineWidth->val," notitle\n");
    streamLine(&a.POUT, scan, start, end, useSD);
    a.POUT<<"e\n";
    a.POUT.flush();
}
void tabCamGnuplot::saveLine(const pgScanGUI::scanRes* scan, const cv::Point& start, const cv::Point& end, bool useSD){
    std::string fileName=QFileDialog::getSaveFileName(this,"Select file for saving line data.", "","Text (*.txt)").toStdString();
    if(scan==nullptr || fileName.empty()) return;
    if(fileName.find(".txt")==std::string::npos) fileName+=".txt";
    std::ofstream wfile(fileName);
    wfile<<"# [um] [nm]\n";
    streamLine(&wfile, scan, start, end, useSD);
    wfile.close();
}
void tabCamGnuplot::plotRoi (const pgScanGUI::scanRes* scan, const cv::Rect& roi, bool useSD){
    if(scan==nullptr) return;
    gnuplot a;
    a.POUT<<util::toString( "set terminal wxt enhanced\n",
                            "set tics font \"Helvetica,",d3ticsFontSize->val,"\"\n",
                            "set xlabel font \"Helvetica,",d3xyzLabelFontSize->val,"\"\n",
                            "set ylabel font \"Helvetica,",d3xyzLabelFontSize->val,"\"\n",
                            "set cblabel font \"Helvetica,",d3xyzLabelFontSize->val,"\"\n",
                            "set xlabel \"XPosition/ um\"\n",
                            "set ylabel \"YPosition/ um\"\n");
    if(equalizeXYZ->val) a.POUT<<util::toString("set cblabel \"Height / um\"\n",
                                                "set view equal xyz\n",
                                                "unset ztics\n");
    else a.POUT<<util::toString("set cblabel \"Height / nm\"\n",
                                "set view equal xy\n");
    a.POUT<<util::toString( "set xyplane 0\n",
                            "set hidden3d\n",
                            "set view ,,",viewZoom->val,",",scaleZ->val,"\n",
                            "set palette rgb ",d3paletteR->val,",",d3paletteG->val,",",d3paletteB->val,"\n",
                            "splot \"-\" matrix w pm3d pt 6 notitle\n");
    cv::Mat data;
    if(useSD && !scan->depthSS.empty()){
        cv::divide(cv::Mat(scan->depthSS, roi),scan->avgNum-1,data);
        cv::sqrt(data,data);
    }
    else data=cv::Mat(scan->depth, roi);
    cv::Mat mask(scan->mask, roi);
    double min;
    cv::minMaxLoc(data, &min);
    for(int j=data.rows-1;j>=0;j--){
        for(int i=0;i!=data.cols;i++){
            if(i!=0) a.POUT<<" ";
            if(mask.at<uchar>(j,i)) a.POUT<<"nan";
            else a.POUT<<(data.at<float>(j,i)-min)*(equalizeXYZ->val?0.001:1);
        }
        a.POUT<<"\n";
    }
    a.POUT<<"e\ne\n";
    a.POUT.flush();
}

void tabCamGnuplot::streamLine(std::ostream *stream, const pgScanGUI::scanRes* scan, const cv::Point& start, const cv::Point& end, bool useSD){
    if(scan==nullptr) return;
    cv::Mat data;
    if(useSD && !scan->depthSS.empty()){
        cv::divide(scan->depthSS,scan->avgNum-1,data);
        cv::sqrt(data,data);
    }else data=scan->depth;

    float len;
    len=sqrt(pow(end.x-start.x,2)+pow(end.y-start.y,2));
    std::cerr<<"len= "<<len<<"\n";
    if((int)len<=0) return;
    std::vector<float> lineData;
    std::vector<bool> lineMask;
    lineData.reserve((int)len+1);
    for(int i=0;i<=len;i++){
        float _X=end.x+(start.x-end.x)*(len-i+0.01)/len;  //the 0.01 fixes a weird floor/ceil bug
        float _Y=end.y+(start.y-end.y)*(len-i+0.01)/len;
        lineData.emplace_back(
            (1-(_X-floor(_X)))*(1-(_Y-floor(_Y)))*(data.at<float>(floor(_Y),floor(_X)))+
            (1-(ceil(_X) -_X))*(1-(ceil(_Y) -_Y))*(data.at<float>( ceil(_Y), ceil(_X)))+
            (1-(_X-floor(_X)))*(1-(ceil(_Y) -_Y))*(data.at<float>( ceil(_Y),floor(_X)))+
            (1-(ceil(_X) -_X))*(1-(_Y-floor(_Y)))*(data.at<float>(floor(_Y), ceil(_X))) );
        lineMask.emplace_back(scan->mask.at<uchar>(floor(_Y),floor(_X))||
                              scan->mask.at<uchar>( ceil(_Y), ceil(_X))||
                              scan->mask.at<uchar>( ceil(_Y),floor(_X))||
                              scan->mask.at<uchar>(floor(_Y), ceil(_X)));
    }
    float minval=std::numeric_limits<float>::max();
    for(int i=0;i<=len;i++) if(lineData[i]<minval) minval=lineData[i];
    for(int i=0;i<=len;i++) lineData[i]-=minval;

    for(int i=0;i<=len;i++)
        *stream<<i*scan->XYnmppx/1000<<" "<<(lineMask[i]?"nan":util::toString(lineData[i]))<<"\n";
}
