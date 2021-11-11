#ifndef CVPLOTQWINDOW_H
#define CVPLOTQWINDOW_H
#include "opencv2/opencv.hpp"
#include <QLabel>
#include <CvPlot/cvplot.h>
#include <QMenu>


class CvPlotQWindow : public QLabel{
    Q_OBJECT
public:
    CvPlotQWindow();
    void redraw();
    CvPlot::Axes axes;
protected:
    void resizeEvent(QResizeEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    Qt::MouseButton activeMouseButton{Qt::NoButton};
    cv::Size wsize;
    QMenu menu;

    class selectionRect : public CvPlot::Drawable{
    public:
        cv::Rect2d _rect;
        cv::Point2d startp;
        bool enabled{false};
        void render(CvPlot::RenderTarget &renderTarget) override {
            if(!enabled) return;
            auto p1=renderTarget.project({_rect.x,_rect.y});
            auto p2=renderTarget.project({_rect.x+_rect.width,_rect.y+_rect.height});
            cv::rectangle(renderTarget.innerMat(), cv::Rect2d(p1, p2), cv::Scalar(0,0,0), 1);
        }
        bool getBoundingRect(cv::Rect2d &rect) override {
            if(!enabled) return false;
            rect=_rect;
            return true;
        }
    };
    class calcCoords{
    public:
        calcCoords(){};
        calcCoords(QMouseEvent* event, CvPlotQWindow* parent);
        cv::Point pos;
        cv::Rect innerRect;
        cv::Point posProj;
        cv::Point2d posUnProj;
    };

    calcCoords ccPress;
    calcCoords ccMove;
private Q_SLOTS:
    void onSavePlot();
    void onSaveData();
};



#endif // CVPLOTQWINDOW_H
