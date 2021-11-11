#include "cvplotqwindow.h"
#include <QPixmap>
#include <QMouseEvent>
#include <QWheelEvent>
#include "UTIL/utility.h"

CvPlotQWindow::CvPlotQWindow(){
    setWindowFlag(Qt::Window);
    resize(QSize(1000,500));
    setMouseTracking(true);
}
void CvPlotQWindow::redraw(){
    wsize=cv::Size(size().width(), size().height());
    cv::Mat displayMat=axes.render(wsize.height, wsize.width);
    axes.findOrCreate<selectionRect>("srect");
    setPixmap(QPixmap::fromImage(QImage(displayMat.data, displayMat.cols, displayMat.rows, displayMat.step, QImage::Format_RGB888)));
}
void CvPlotQWindow::resizeEvent(QResizeEvent *event){
    QLabel::resizeEvent(event);
    wsize=cv::Size(size().width(), size().height());
    redraw();
}
CvPlotQWindow::calcCoords::calcCoords(QMouseEvent* event, CvPlotQWindow* parent){
    innerRect=parent->axes.getProjection(parent->wsize).innerRect();
    posProj.x=event->pos().x()-innerRect.x;
    posProj.y=event->pos().y()-innerRect.y;
    posUnProj=parent->axes.getProjection(parent->wsize).unproject(posProj);
}
void CvPlotQWindow::mousePressEvent(QMouseEvent *event){
    calcCoords cc(event, this);

    coordPress.x=event->pos().x();
    coordPress.y=event->pos().y();
    coordMove=coordPress;
    activeMouseButton=event->button();
    switch(event->button()){
    case(Qt::RightButton):
        axes.find<selectionRect>("srect")->startp=cc.posUnProj;
        break;
    default:;
    }
}
void CvPlotQWindow::mouseMoveEvent(QMouseEvent *event){
    calcCoords cc(event, this);
    setWindowTitle(QString::fromStdString(util::toString("Plot; X=",cc.posUnProj.x,", Y=",cc.posUnProj.y)));

    switch(activeMouseButton){
    case(Qt::MiddleButton):
        axes.zoom(wsize,coordPress,1+(coordMove.x-event->pos().x())/static_cast<double>(cc.innerRect.width),1-(coordMove.y-event->pos().y())/static_cast<double>(cc.innerRect.height));
        redraw();
        break;
    case(Qt::LeftButton):
        axes.pan(wsize,cv::Point(event->pos().x(),event->pos().y())-coordMove);
        redraw();
        break;
    case(Qt::RightButton):
        {auto srect=axes.find<selectionRect>("srect");
        srect->_rect.x=std::min(srect->startp.x,cc.posUnProj.x);
        srect->_rect.y=std::min(srect->startp.y,cc.posUnProj.y);
        srect->_rect.width=std::abs(srect->startp.x-cc.posUnProj.x);
        srect->_rect.height=std::abs(srect->startp.y-cc.posUnProj.y);
        srect->enabled=true;}
        redraw();
        break;
    default:;
    }
    coordMove.x=event->pos().x();
    coordMove.y=event->pos().y();
}

void CvPlotQWindow::mouseReleaseEvent(QMouseEvent *event){
    calcCoords cc(event, this);

    activeMouseButton=Qt::NoButton;
    switch(event->button()){
    case(Qt::RightButton):
        if(coordPress.x!=coordMove.x && coordPress.y-coordMove.y){
            axes.find<selectionRect>("srect")->enabled=false;
            axes.zoom(wsize,(coordPress+coordMove)/2,std::abs(coordPress.x-coordMove.x)/static_cast<double>(cc.innerRect.width),std::abs(coordPress.y-coordMove.y)/static_cast<double>(cc.innerRect.height));
            redraw();
        }else{
            std::cerr<<"TODO implement menu\n";
        }
        break;
    default:;
    }
}
void CvPlotQWindow::wheelEvent(QWheelEvent *event){
    double zoom=1-0.01*event->angleDelta().y()/8;
    axes.zoom(wsize,cv::Point(event->position().x(),event->position().y()),zoom,zoom);
    redraw();
}

