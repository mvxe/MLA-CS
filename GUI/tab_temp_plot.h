#ifndef TAB_TEMP_PLOT_H
#define TAB_TEMP_PLOT_H
#include "GUI/gui_includes.h"
class MainWindow;
class plotSelLabel;

class tab_temp_plot : public QWidget{
    Q_OBJECT
public:
    tab_temp_plot(QWidget* parent);
    QVBoxLayout* layout;

    QWidget* btnWidget;
    QHBoxLayout* btnLayout;
    QPushButton* load_file;
    QDoubleSpinBox* lambda;
    QSpinBox* fontSB;
    plotSelLabel* imgLabel;
    QLabel* sizeLabel;
    QCheckBox* inverted;
    QCheckBox* equalXYZ;
    QDoubleSpinBox* scale3D;
    QPushButton* replot;
    QPushButton* save;

    cv::Mat* imgMat;
    cv::Mat* imgMat8bit;
    cv::Mat* imgMatWLines;

private Q_SLOTS:
    void on_load_file_clicked();
    void on_replot_clicked();
    void on_save_clicked();
public:
    void redraw();
    void plot();
    void fsave(std::string filename);
    MainWindow* pmw;

    bool hasSmth=false;
    bool active=false;
    bool isRight=false;
    int X1,Y1,X2,Y2;
};


class plotSelLabel : public QLabel{
    using QLabel::QLabel;
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
public:
    tab_temp_plot* ttp;
};

#endif // TAB_TEMP_PLOT_H
