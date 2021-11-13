#ifndef PGWRITE_H
#define PGWRITE_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
#include "PROCEDURES/UTIL/USC/scan.h"
#include <gsl/gsl_vector.h>
#include <gsl/gsl_bspline.h>
class writeSettings;
class pgBeamAnalysis;
class QLineEdit;
class procLockProg;
class hidCon;
class QTreeView;
class QStandardItemModel;
class QStandardItem;
class QFont;
class pgWrite: public QObject{
    Q_OBJECT
public:
    pgWrite(pgBeamAnalysis* pgBeAn, pgMoveGUI* pgMGUI, procLockProg& MLP, pgScanGUI *pgSGUI, overlay& ovl);
    ~pgWrite();
    rtoml::vsr conf;                //configuration map
    QWidget* gui_activation;
    QWidget* gui_settings;
    void drawWriteArea(cv::Mat* img);
    bool writeMat(cv::Mat* override=nullptr, double override_depthMaxval=0, double override_imgmmPPx=0, double override_pointSpacing=0, double override_focus=std::numeric_limits<double>::max(), double ov_fxcor=std::numeric_limits<double>::max(), double ov_fycor=std::numeric_limits<double>::max());
            // matrix depth, if CV_32F, is in mm, likewise override_depthMaxval is in mm too
            // override_depthMaxval overrides only if matrix is CV_8U or CV_16U, also in nm
            // return true if failed/aborted

private:
    //activation
    QVBoxLayout* alayout;
    hidCon* pulseh;
    QPushButton* pulse;
    val_selector* pulseDur;

    hidCon* importh;
    QPushButton* importImg;
    val_selector* depthMaxval;
    double lastDepth;
    val_selector* imgUmPPx;

    QPushButton* abort;
    val_selector* dTCcor;
    QPushButton* corDTCor;
    HQPushButton* writeDM;
    HQPushButton* scanB;
    QPushButton* saveB;
    QLineEdit* tagText;
    HQPushButton* writeTag;
    twid* tagSTwid;
    lineedit_gs* tagString;
    val_selector* tagUInt;
    QPushButton* guessTagUInt;
    checkbox_gs* useWriteScheduling;
    twid* schedulelwtwid;
    QTreeView* schedulelw;
    QStandardItemModel* schedulemod;
    QPushButton* itemMoveTop;
    QPushButton* itemMoveUp;
    QPushButton* itemMoveDown;
    QPushButton* itemMoveBottom;
    QPushButton* itemRemove;

    struct schItem{
        QStandardItem* ptr;
        cv::Mat src;            // write source image
        bool isWrite;           // if not, it is scan
        double coords[3];       // write/scan coordinates
        std::string conf;       // if scan, prefilled conf
        std::string filename;   // if write, source filename, if scan, save filename
        cv::Rect scanROI;       // if scan, ROI
        double writePars[6];    // write parameters
        void* overlay{nullptr};
    };
    std::vector<schItem> scheduled;

    //settings
    QVBoxLayout* slayout;

    smp_selector* selectWriteSetting;
    std::vector<writeSettings*> settingWdg;
    friend class pgCalib;
    friend class writeSettings;
    constexpr static unsigned Nset{5};
    val_selector* focus;
    val_selector* focusXcor;
    val_selector* focusYcor;
    checkbox_gs* usingBSpline;
    val_selector* constA;
    val_selector* constC;
    val_selector* plataeuPeakRatio;
    val_selector* pointSpacing;
    checkbox_gs* writeZeros;
    std::vector<double>* bsplbreakpts;
    std::vector<double>* bsplcoefs;
    std::vector<double>* bsplcov;

    hidCon* folderhcon;
    btnlabel_gs* write_default_folder;
    btnlabel_gs* scan_default_folder;
    lineedit_gs* filenaming;
    val_selector* numbericTagMinDigits;
    lineedit_gs* tagnaming;

    val_selector* scanExtraBorder;
    val_selector* scanRepeatN;
    checkbox_gs* switchBack2mirau;
    bool wasMirau;

    QPushButton* notes;
    checkbox_gs* addNotes;
    std::string notestring;

    cv::Mat WRImage;
    std::string fileName;
    cv::Mat tagImage;
    int drawWriteAreaOn{0};     //1 is img, 2 is tag, 3 is frame
    cv::Rect scanROI;
    void prepareScanROI();
    double scanCoords[3];

    pgBeamAnalysis* pgBeAn;
    pgMoveGUI* pgMGUI;
    procLockProg& MLP;
    pgScanGUI* pgSGUI;
    overlay& ovl;
    varShareClient<pgScanGUI::scanRes>* scanRes;
    const pgScanGUI::scanRes* res;

    void preparePredictor();    // call before using predictDuration
    gsl_vector* p_basisfun{nullptr};
    gsl_bspline_workspace* p_bsplws{nullptr};
    gsl_vector* p_coefs;
    gsl_matrix* p_covmat;
    gsl_vector* p_gbreakpts;
    bool p_ready{false};
    double predictDuration(double targetHeight);    // targetHeight in nm, return is in ms
    void replacePlaceholdersInString(std::string& src);
    void stripDollarSigns(std::string &str);
    bool firstImageLoaded{false};
    bool firstWritten{false};
    bool wabort;

    void saveConfig(std::string filename);
    std::string genConfig();
    QStandardItem* addScheduleItem(std::string status, std::string type, std::string name, bool toTop);
    void prepareSchedTagFrame(std::string name);

    constexpr static int maxRedoScanTries=3;
private Q_SLOTS:
    void onPulse();
    void onMenuChange(int index);
    void onLoadImg();
    void onWriteTag();
    void onChangeDrawWriteAreaOn(bool status);
    void onChangeDrawScanAreaOn(bool status);
    void onChangeDrawWriteAreaOnTag(bool status);
    void onCorPPR();
    void on_write_default_folder();
    void on_scan_default_folder();
    void onScan();
    void onSave();
    void onUseWriteScheduling(bool state);
    void onCheckTagString();
    void onRecomputeTagString();
    void onRecomputeTagUInt();
    void onRecomputeTag();
    void onWriteDM();
    void onAbort();
    void guessAndUpdateNextTagUInt();
    void onNotes();
    void onItemMoveTop();
    void onItemMoveUp();
    void onItemMoveDown();
    void onItemMoveBottom();
    void onItemRemove();
};


class writeSettings: public QWidget{
    Q_OBJECT
    //GUI
public:
    writeSettings(uint num, pgWrite* parent);
    std::string name;
    QVBoxLayout* slayout;
    pgWrite* parent;
    val_selector* focus;
    val_selector* focusXcor;
    val_selector* focusYcor;
    checkbox_gs* usingBSpline;
    val_selector* constA;
    val_selector* constC;
    val_selector* plataeuPeakRatio;
    QPushButton* corPPR;
    val_selector* pointSpacing;
    checkbox_gs* writeZeros;
    std::vector<double> bsplbreakpts;
    std::vector<double> bsplcoefs;
    std::vector<double> bsplcov;

    //tag:
    smp_selector* fontFace;
    val_selector* fontSize;
    val_selector* fontThickness;
    val_selector* imgUmPPx;
    val_selector* depthMaxval;
    val_selector* frameDis;

    bool& p_ready;

private Q_SLOTS:
    void onUsingBSpline(bool state);
};

#endif // PGWRITE_H
