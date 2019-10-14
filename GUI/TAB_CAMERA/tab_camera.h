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

class mtlabel; //defined in mainwindow.h

class tab_camera: public QWidget{
    Q_OBJECT

public:
    tab_camera(QWidget* parent);

    void tab_entered();
    void tab_exited();

private:
    QTimer *timer;

    QHBoxLayout* layout;
    mtlabel* LDisplay;
    QTabWidget* TWCtrl;

    QWidget* pageMotion;
    QWidget* pageWriting;

    QWidget* pageSettings;
    QVBoxLayout* layoutSettings;
        val_selector* led_wl;   //LED wavelength
        val_selector* coh_len;  //coherence length
        val_selector* range;    //scan range
        val_selector* ppwl;     //points per wavelength
        val_selector* max_vel;  //maximum microscope axis velocity
        val_selector* max_acc;  //maximum microscope axis acceleration
        std::atomic<bool> changed{true};
        QLabel* calcL;


    FQ* framequeue;
    const cv::Mat* mat;

    constexpr static unsigned work_call_time=100;    //work_fun is called periodically via timer every this many milliseconds
    bool running=false;
private Q_SLOTS:
    void work_fun();
    double vsConv(val_selector* vs);

public Q_SLOTS:


};


#endif // CONFIG_TAB_CAMERA_H
