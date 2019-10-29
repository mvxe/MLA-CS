#ifndef CONFIG_TAB_CAMERA_H
#define CONFIG_TAB_CAMERA_H
#include "GUI/gui_includes.h"

    // These are from the old camera tab; TODO remove when replaced!
class tab_camera_old{
        private:
            std::mutex dismx;
        protected:
            cv::Mat* onDisplay;
        public:
            tsbool disable{&dismx};

            double xps_x_sen, xps_y_sen, xps_z_sen, xps_f_sen;
            cc_save<double> xps_x_sen_save{xps_x_sen, 100000,&go.gui_config.save,"xps_x_sen"};
            cc_save<double> xps_y_sen_save{xps_y_sen, 100000,&go.gui_config.save,"xps_y_sen"};
            cc_save<double> xps_z_sen_save{xps_z_sen, 100000,&go.gui_config.save,"xps_z_sen"};
            cc_save<double> xps_f_sen_save{xps_f_sen, 100000,&go.gui_config.save,"xps_f_sen"};
};

class tab_camera: public QWidget{
    Q_OBJECT

public:
    tab_camera(QWidget* parent);
    ~tab_camera();

    void tab_entered();
    void tab_exited();

private:
    QTimer *timer;

    QHBoxLayout* layout;
    QLabel* LDisplay;
    QWidget* tBarW;
    QVBoxLayout* layoutTBarW;
    smp_selector* selDisp;
    int oldIndex=-1;
    int oldCm=-1;
    QTabWidget* TWCtrl;

    twd_selector* pageMotion;

    QWidget* pageWriting;

    twd_selector* pageSettings;
    pgScanGUI* pgSGUI;  constexpr static unsigned index_pgSGUI{0};
    pgMoveGUI* pgMGUI;
    pgTiltGUI* pgTGUI;

    smp_selector* cm_sel;       //for selecting the colormap for display

    FQ* framequeueDisp;
    const cv::Mat* mat;
    const cv::Mat* onDisplay;

    constexpr static unsigned work_call_time=33;    //work_fun is called periodically via timer every this many milliseconds



private Q_SLOTS:
    void work_fun();
    void on_tab_change(int index);

};


#endif // CONFIG_TAB_CAMERA_H
