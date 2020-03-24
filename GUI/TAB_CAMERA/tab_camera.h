#ifndef CONFIG_TAB_CAMERA_H
#define CONFIG_TAB_CAMERA_H
#include "GUI/gui_includes.h"
#include "colormap.h"
#include "other_settings.h"
#include "UTIL/measurement.h"
#include "gnuplot.h"
class iImageDisplay;

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
    QPixmap* LDisplayPixmap{nullptr};
    QWidget* tBarW;
    QVBoxLayout* layoutTBarW;
    smp_selector* selDisp;
    int oldIndex=-1;
    int oldCm=-1;
    val_selector* dispScale;
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
    pgCorrection* pgCor;
    QLabel* addInfo;

    smp_selector* cm_sel;       //for selecting the colormap for display
    QPushButton* epc_sel;       //for selecting the excluded pixel color
    checkbox_save* showAbsHeight;
    cv::Scalar exclColor;
    cc_save<double> _scolorR{exclColor.val[0], 0,&go.gui_config.save,"tab_camera-exclColor-B"};
    cc_save<double> _scolorG{exclColor.val[1], 0,&go.gui_config.save,"tab_camera-exclColor-G"};
    cc_save<double> _scolorB{exclColor.val[2], 0,&go.gui_config.save,"tab_camera-exclColor-R"};
    bool redrawHistClrmap{false};
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
    pgWrite* pgWrt;
    //pgWritePredictor* pgWrtPrd;

    QCheckBox* redLaserOn;
    QCheckBox* greenLaserOn;

    QMenu* clickMenu;
    QMenu* clickMenuDepthRight;
    QMenu* clickMenuDepthLeft;

    double selStartX, selStartY;
    double selCurX, selCurY;
    double selEndX, selEndY;
    bool selectingFlag{false};
    bool lastSelectingFlag{false};
    bool selectingFlagIsLine{false};
    int dispDepthMatRows;
    int dispDepthMatCols;

    FQ* framequeueDisp;
    const cv::Mat* onDisplay;

    constexpr static unsigned work_call_time=33;    //work_fun is called periodically via timer every this many milliseconds
    pgScanGUI::scanRes loadedScan; bool updateDisp{false}; bool loadedOnDisplay{false};

    void scaleDisplay(cv::Mat img, QImage::Format format);
private Q_SLOTS:
    void work_fun();
    void on_tab_change(int index);
    void on_EP_sel_released();
    void onShowAbsHeightChanged();

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

    void updateImgF();

    void onRedLaserToggle(bool state);
    void onGreenLaserToggle(bool state);

    void showScan(pgScanGUI::scanRes scan);
};


class iImageDisplay : public QLabel{
    using QLabel::QLabel;
public:
    int isDepth{false};
    tab_camera* parent;
private:
    void calsVars(QMouseEvent *event, double* xcoord, double* ycoord, double* pwidth, double* pheight);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
};

#endif // CONFIG_TAB_CAMERA_H
