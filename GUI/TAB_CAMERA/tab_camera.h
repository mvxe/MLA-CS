#ifndef CONFIG_TAB_CAMERA_H
#define CONFIG_TAB_CAMERA_H
#include "GUI/gui_includes.h"
#include "colormap.h"
#include "other_settings.h"
#include "UTIL/measurement.h"
#include "gnuplot.h"
class iImageDisplay;

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
friend iImageDisplay;
public:
    tab_camera(QWidget* parent);
    ~tab_camera();

    void tab_entered();
    void tab_exited();

    mesLockProg MLP;
private:
    QTimer *timer;

    QHBoxLayout* layout;
    iImageDisplay* LDisplay;
    QWidget* tBarW;
    QVBoxLayout* layoutTBarW;
    smp_selector* selDisp;
    int oldIndex=-1;
    int oldCm=-1;
    QTabWidget* TWCtrl;

    twd_selector* pageMotion;
    twd_selector* pageWriting;
    twd_selector* pageProcessing;
    twd_selector* pageSettings;
    varShareClient<pgScanGUI::scanRes>* scanRes;

    colorMap* cMap;
    tabCamGnuplot* tCG;
    pgScanGUI* pgSGUI;      unsigned index_pgSGUI;
    pgMoveGUI* pgMGUI;
    pgTiltGUI* pgTGUI;
    pgFocusGUI* pgFGUI;     unsigned index_pgFGUI;
    pgPosRepGUI* pgPRGUI;
    cameraSett* camSet;     unsigned index_camSet;
    QLabel* addInfo;

    smp_selector* cm_sel;       //for selecting the colormap for display
    QPushButton* epc_sel;       //for selecting the excluded pixel color
    cv::Scalar exclColor;
    cc_save<double> _scolorR{exclColor.val[0], 0,&go.gui_config.save,"tab_camera-exclColor-B"};
    cc_save<double> _scolorG{exclColor.val[1], 0,&go.gui_config.save,"tab_camera-exclColor-G"};
    cc_save<double> _scolorB{exclColor.val[2], 0,&go.gui_config.save,"tab_camera-exclColor-R"};
    bool exclColorChanged{false};
    pgHistogrameGUI* pgHistGUI;
    checkbox_save* main_show_scale;
    checkbox_save* main_show_target;
    checkbox_save* main_show_bounds;
    QProgressBar* measPB;
    QProgressBar* compPB;

    QPushButton* loadRawBtn;
    QPushButton* diff2RawBtn;

    // writing
    pgBoundsGUI* pgBGUI;
    pgDepthEval* pgDpEv;
    pgCalib* pgCal;
    pgBeamAnalysis* pgBeAn;

    QCheckBox* redLaserOn;
    QCheckBox* greenLaserOn;

    QMenu* clickMenu;
    QMenu* clickMenuDepthRight;
    QMenu* clickMenuDepthLeft;

    int selStartX, selStartY;
    int selCurX, selCurY;
    int selEndX, selEndY;
    bool selectingFlag{false};
    bool lastSelectingFlag{false};
    bool selectingFlagIsLine{false};
    int dispDepthMatRows;

    FQ* framequeueDisp;
    const cv::Mat* onDisplay;

    constexpr static unsigned work_call_time=33;    //work_fun is called periodically via timer every this many milliseconds
    pgScanGUI::scanRes loadedScan; bool loadedScanChanged{false}; bool loadedOnDisplay{false};
private Q_SLOTS:
    void work_fun();
    void on_tab_change(int index);
    void on_EP_sel_released();

    void onSavePixData();
    void onSaveCameraPicture();
    void onSaveDepthMap();
    void onSaveDepthMapRaw(bool txt=false);
    void onSaveDepthMapTxt(){onSaveDepthMapRaw(true);}
    void onLoadDepthMapRaw();
    void onRotateDepthMap();
    void onDiff2Raw();
    void onPlotLine();
    void onSaveLine();
    void onPlotRect();
    void on2DFFT();

    void onRedLaserToggle(bool state);
    void onGreenLaserToggle(bool state);
};


class iImageDisplay : public QLabel{
    using QLabel::QLabel;
public:
    int isDepth{false};
    tab_camera* parent;
private:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    bool checkIfInBounds(int xcoord, int ycoord);
};

#endif // CONFIG_TAB_CAMERA_H
