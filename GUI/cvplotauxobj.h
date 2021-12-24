#ifndef CVPLOTAUXOBJ_H
#define CVPLOTAUXOBJ_H
#include "opencv2/opencv.hpp"
#include <CvPlot/cvplot.h>
#include "gui_aux_objects.h"

class cvp_pointrange : public QWidget{
    Q_OBJECT
public:
    cvp_pointrange(unsigned height, unsigned width, std::string unit="");
    struct pt{
        double val;
        bool en;
    };

    bool eval(double val);  // returns true if in range
    double getMin();
    double getMax();
    void set_points(std::vector<pt>& points);
private:
    CvPlot::Axes axes;
    unsigned height,width;
    double min,max;

    class cvp_disp : public QLabel{
    protected:
        //void mouseMoveEvent(QMouseEvent *event);
        //void mousePressEvent(QMouseEvent *event);
        //void mouseReleaseEvent(QMouseEvent *event);
    } img;

};




#endif // CVPLOTAUXOBJ_H
