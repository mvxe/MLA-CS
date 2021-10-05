#ifndef PGWRITE_H
#define PGWRITE_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
#include "PROCEDURES/UTIL/USC/scan.h"
class writeSettings;
class pgBeamAnalysis;
class QLineEdit;
class pgWrite: public QObject{
    Q_OBJECT
public:
    pgWrite(pgBeamAnalysis* pgBeAn, pgMoveGUI* pgMGUI);
    rtoml::vsr conf;                //configuration map
    QWidget* gui_activation;
    QWidget* gui_settings;
    void drawWriteArea(cv::Mat* img);
private:
    //activation
    QVBoxLayout* alayout;
    QPushButton* pulse;
    val_selector* pulseDur;

    QPushButton* importImg;
    val_selector* depthMaxval;
    val_selector* imgUmPPx;
    val_selector* ICcor;
    QPushButton* corICor;
    QPushButton* writeDM;
    QPushButton* writeFrame;
    QLineEdit* tagText;
    HQPushButton* writeTag;

    //settings
    QVBoxLayout* slayout;

    smp_selector* selectWriteSetting;
    std::vector<writeSettings*> settingWdg;
    friend class writeSettings;
    constexpr static unsigned Nset{5};
    val_selector* focus;
    val_selector* focusXcor;
    val_selector* focusYcor;
    val_selector* duration;
    val_selector* constA;
    val_selector* constB;
    val_selector* constX0;
    val_selector* plataeuPeakRatio;
    val_selector* pointSpacing;

    val_selector* max_vel;
    val_selector* max_acc;

    cv::Mat WRImage;
    cv::Mat tagImage;
    int drawWriteAreaOn{0};     //1 is img, 2 is tag, 3 is frame
    const double servoCycle{0.0001};    // The XPS servo cycle (in s)

    pgBeamAnalysis* pgBeAn;
    pgMoveGUI* pgMGUI;

    uint getInt(float post, float pre=0);
    float calcH(float Int, float pre);
    float gaussian(float x, float y, float a, float wx, float wy, float an);
private Q_SLOTS:
    void onPulse();
    void onMenuChange(int index);
    void onLoadImg();
    void onWriteDM(cv::Mat* override=nullptr, double override_depthMaxval=0, double override_imgUmPPx=0, double override_pointSpacing=0, double override_duration=0, double override_focus=0, double ov_fxcor=0, double ov_fycor=0);  //if you override override mat, you must override them all
    void onWriteFrame();
    void onWriteTag();
    void onChangeDrawWriteAreaOn(bool status);
    void onChangeDrawWriteAreaOnTag(bool status);
    void onChangeDrawWriteFrameAreaOn(bool status);
    void onCorICor();
    void onCorPPR();
};



class writeSettings: public QWidget{
    Q_OBJECT
    //GUI
public:
    writeSettings(uint num, pgWrite* parent);
    QVBoxLayout* slayout;
    pgWrite* parent;
    val_selector* focus;
    val_selector* focusXcor;
    val_selector* focusYcor;
    val_selector* duration;
    val_selector* constA;
    val_selector* constB;
    val_selector* constX0;
    val_selector* plataeuPeakRatio;
    QPushButton* corPPR;
    val_selector* pointSpacing;

    //tag:
    smp_selector* fontFace;
    val_selector* fontSize;
    val_selector* fontThickness;
    val_selector* imgUmPPx;
    val_selector* depthMaxval;
    val_selector* frameDis;
};

#endif // PGWRITE_H
