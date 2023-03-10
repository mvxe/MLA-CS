#ifndef TABCAMGNUPLOT_H
#define TABCAMGNUPLOT_H
#include <iostream>
#include "opencv2/opencv.hpp"
#include "PROCEDURES/UTIL/USC/scan.h"

class tabCamGnuplot: public QWidget{
    Q_OBJECT
public:
    tabCamGnuplot(checkbox_gs* showAbsHeight);
    rtoml::vsr conf;                //configuration map
    void plotLine(const pgScanGUI::scanRes* scan, const cv::Point& start, const cv::Point& end, int type=1);
    void saveLine(const pgScanGUI::scanRes* scan, const cv::Point& start, const cv::Point& end, int type=1);
    void plotRoi (const pgScanGUI::scanRes* scan, const cv::Rect& roi, int type=1);     // save ROI is already implemented elsewhere

    void streamLine(std::ostream *stream, const pgScanGUI::scanRes* scan, const cv::Point& start, const cv::Point& end, int type);
private:
    QVBoxLayout* layout;
    checkbox_gs* showAbsHeight;

    //2D plot
    val_selector* ticsFontSize;
    val_selector* xyLabelFontSize;
    val_selector* lpColor;
    val_selector* lineWidth;
    val_selector* pointSize;
    val_selector* pointType;

    //3D plot
    val_selector* d3ticsFontSize;
    val_selector* d3xyzLabelFontSize;
    val_selector* d3paletteR;
    val_selector* d3paletteG;
    val_selector* d3paletteB;
    val_selector* viewZoom;
    val_selector* scaleZ;
    checkbox_gs* equalizeXYZ;


};

#endif // TABCAMGNUPLOT_H
