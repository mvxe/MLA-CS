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
    val_selector* constC;
    val_selector* pointSpacing;

    cv::Mat WRImage;
    bool drawWriteAreaOn{false};

    pgBeamAnalysis* pgBeAn;
    pgMoveGUI* pgMGUI;
private Q_SLOTS:
    void onPulse();
    void onMenuChange(int index);
    void onLoadImg();
    void onWriteDM();
    void onChangeDrawWriteAreaOn(bool status);
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
    val_selector* constC;
    val_selector* pointSpacing;
};

#endif // PGWRITE_H
