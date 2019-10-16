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
    ~tab_camera();

    void tab_entered();
    void tab_exited();

private:
    QTimer *timer;

    QHBoxLayout* layout;
    mtlabel* LDisplay;
    QTabWidget* TWCtrl;

    QWidget* pageMotion;
    QVBoxLayout* motionSettings;
        QPushButton* scanOne;

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
    FQ* framequeueDisp;
    const cv::Mat* mat;
    const cv::Mat* onDisplay;

    constexpr static unsigned work_call_time=33;    //work_fun is called periodically via timer every this many milliseconds
    bool running=false;


    bool isOffset=false;    // false=we are centered, true=we are offset and ready to start
    double setOffset=0;
    PVTobj* PVTtoPos[2];    // [0] is downward, [1] is upward
    PVTobj* PVTmeasure[2];  // [0] is downward, [1] is upward
    exec_ret PVTret;
    bool PVTsRdy=false;
    int dir=0;              // to be used as PVT[dir] and flipped after each move (valid values: 0 and 1)
    void updatePVTs(std::string &report);   // update PVTs whenever measurement paramaters are changed, returns true if PVT fails or accels/speeds are to high
    void doOneRound();      // this automatically does the offset in case we are not offset
    void getCentered();     // this recenters

private Q_SLOTS:
    void work_fun();
    double vsConv(val_selector* vs);
    void onScanOneReleased() {doOneRound();}

public Q_SLOTS:


};


#endif // CONFIG_TAB_CAMERA_H
