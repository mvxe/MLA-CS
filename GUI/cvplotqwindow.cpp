#include "cvplotqwindow.h"
#include <QPixmap>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFileDialog>
#include "UTIL/utility.h"
#include <fstream>

CvPlotQWindow::CvPlotQWindow(){
    setWindowFlag(Qt::Window);
    resize(QSize(1000,500));
    setMouseTracking(true);
    menu.addAction("Save plot", this, SLOT(onSavePlot()));
    menu.addAction("Save data", this, SLOT(onSaveData()));
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
    pos.x=event->pos().x();
    pos.y=event->pos().y();
    innerRect=parent->axes.getProjection(parent->wsize).innerRect();
    posProj.x=pos.x-innerRect.x;
    posProj.y=pos.y-innerRect.y;
    posUnProj=parent->axes.getProjection(parent->wsize).unproject(posProj);
}
void CvPlotQWindow::mousePressEvent(QMouseEvent *event){
    ccPress=calcCoords(event, this);
    ccMove=ccPress;
    activeMouseButton=event->button();
    switch(event->button()){
    case(Qt::RightButton):
        axes.find<selectionRect>("srect")->startp=ccPress.posUnProj;
        break;
    default:;
    }
}
void CvPlotQWindow::mouseMoveEvent(QMouseEvent *event){
    calcCoords cc(event, this);
    setWindowTitle(QString::fromStdString(util::toString("Plot; X=",cc.posUnProj.x,", Y=",cc.posUnProj.y)));

    switch(activeMouseButton){
    case(Qt::MiddleButton):
        axes.zoom(wsize,ccPress.pos,1+(ccMove.pos.x-cc.pos.x)/static_cast<double>(cc.innerRect.width),1-(ccMove.pos.y-cc.pos.y)/static_cast<double>(cc.innerRect.height));
        redraw();
        break;
    case(Qt::LeftButton):
        axes.pan(wsize,cv::Point(event->pos().x(),event->pos().y())-ccMove.pos);
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
    ccMove=cc;
}

void CvPlotQWindow::mouseReleaseEvent(QMouseEvent *event){
    calcCoords cc(event, this);

    activeMouseButton=Qt::NoButton;
    switch(event->button()){
    case(Qt::RightButton):
        if(ccPress.pos.x!=ccMove.pos.x && ccPress.pos.y!=ccMove.pos.y){
            axes.find<selectionRect>("srect")->enabled=false;
            axes.setXLim({std::min(cc.posUnProj.x,ccPress.posUnProj.x),std::max(cc.posUnProj.x,ccPress.posUnProj.x)});
            axes.setYLim({std::min(cc.posUnProj.y,ccPress.posUnProj.y),std::max(cc.posUnProj.y,ccPress.posUnProj.y)});
            redraw();
        }else menu.popup(QCursor::pos());
        break;
    case(Qt::MiddleButton):
        if(ccPress.pos.x==ccMove.pos.x || ccPress.pos.y==ccMove.pos.y){
            axes.setXLimAuto();
            axes.setYLimAuto();
            redraw();
        }
        break;
    default:;
    }
}
void CvPlotQWindow::wheelEvent(QWheelEvent *event){
    double zoom=1-0.01*event->angleDelta().y()/8;
    axes.zoom(wsize,cv::Point(event->pos().x(),event->pos().y()),zoom,zoom);
    redraw();
}
void CvPlotQWindow::onSavePlot(){
    std::string fileName=QFileDialog::getSaveFileName(this,"Save Plot to File", "","Images (*.png *.xpm *.jpg *.bmp)").toStdString();
    if(fileName.empty()) return;
    cv::imwrite(fileName, axes.render(wsize.height, wsize.width));
}
void CvPlotQWindow::onSaveData(){
    std::string fileName=QFileDialog::getSaveFileName(this,"Save Data to File", "","Text files (*.txt *.dat)").toStdString();
    if(fileName.empty()) return;

    struct seriesData{
        std::vector<cv::Point2d> points;
        std::string name;
    };
    std::vector<seriesData> seriess;

    for(auto& drawable:axes.drawables()){
        auto series=dynamic_cast<CvPlot::Series*>(drawable.get());
        if(series!=nullptr)
            seriess.push_back({series->getPoints(), series->getName()});
    }

    if(seriess.empty()) return;
    std::ofstream wfile(fileName);
    std::sort(seriess.begin(), seriess.end(), [](seriesData i,seriesData j){return (i.points.size()>j.points.size());});
    wfile<<"#";
    for(auto& series:seriess) wfile<<" \""<<series.name<<"\".x \""<<series.name<<"\".y";
    for(size_t i=0;i!=seriess[0].points.size();i++){
        wfile<<"\n";
        bool fst=true;
        for(auto& series:seriess)
            if(i<series.points.size()){
                if(fst)fst=false;
                else wfile<<" ";
                wfile<<series.points[i].x<<" "<<series.points[i].y;
            }
    }
    wfile.close();
}
