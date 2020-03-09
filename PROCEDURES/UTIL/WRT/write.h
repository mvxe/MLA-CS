#ifndef PGWRITE_H
#define PGWRITE_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
#include "PROCEDURES/UTIL/USC/scan.h"
class writeSettings;
class pgBeamAnalysis;
class pgWrite: public QObject{
    Q_OBJECT
public:
    pgWrite(pgBeamAnalysis* pgBeAn, pgMoveGUI* pgMGUI);
    QWidget* gui_activation;
    QWidget* gui_settings;
    void drawWriteArea(cv::Mat* img);
private:
    //activation
    QVBoxLayout* alayout;
    QPushButton* pulse;
    val_selector* pulseInt;
    val_selector* pulseDur;

    QPushButton* importImg;
    val_selector* depthMaxval;
    val_selector* imgUmPPx;
    val_selector* ICcor;
    QPushButton* corICor;
    QPushButton* writeDM;

    //settings
    QVBoxLayout* slayout;

    smp_selector* selectWriteSetting;
    std::vector<writeSettings*> settingWdg;
    constexpr static unsigned Nset{5};
    val_selector* focus;
    val_selector* duration;
    val_selector* constA;
    val_selector* constB;
    val_selector* constX0;
    val_selector* pointSpacing;
    val_selector* FWHMX;
    val_selector* FWHMY;
    val_selector* FWHMXYan;

    val_selector* max_vel;
    val_selector* max_acc;

    cv::Mat WRImage;
    bool drawWriteAreaOn{false};
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
    void onWriteDM();
    void onChangeDrawWriteAreaOn(bool status);
    void onCorICor();
};



class writeSettings: public QWidget{
    Q_OBJECT
    //GUI
public:
    writeSettings(uint num, pgWrite* parent);
    QVBoxLayout* slayout;
    pgWrite* parent;
    val_selector* focus;
    val_selector* duration;
    val_selector* constA;
    val_selector* constB;
    val_selector* constX0;
    val_selector* pointSpacing;
    val_selector* FWHMX;
    val_selector* FWHMY;
    val_selector* FWHMXYan;
};

#endif // PGWRITE_H
