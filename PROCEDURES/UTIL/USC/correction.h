#ifndef CORRECTION_H
#define CORRECTION_H

#include "PROCEDURES/procedure.h"
#include "UTIL/img_util.h"
#include "PROCEDURES/UTIL/USC/scan.h"
class QVBoxLayout;
class val_selector;
class QPushButton;
class pgMoveGUI;
class QCheckBox;
class checkbox_gs;
class QLabel;

class pgCorrection: public QObject{
    Q_OBJECT
public:
    pgCorrection(pgScanGUI* pgSGUI, pgMoveGUI* pgMGUI);
    ~pgCorrection();
    rtoml::vsr conf;                    //configuration map

    QWidget* gui_settings;

    std::mutex useCorr;                 //this will be locked when usecorr is disabled or the correction is updated
    pgScanGUI::scanRes cor;
private:
    bool useCorrLocked{false};

    QVBoxLayout* slayout;
    val_selector* selArrayXsize;
    val_selector* selArrayYsize;
    val_selector* selArraySpacing;
    val_selector* selAvgNum;
    val_selector* selExclusionSD;
    val_selector* corBlur;
    QCheckBox* preserveResult;
    QPushButton* startCalib;
    QPushButton* recalcCalib;
    QLabel* report;

    QPushButton* setAsCalib;
    checkbox_gs* enableCorrection;

    constexpr static int maxRedoScanTries=3;
    const static std::string fileName;

    pgScanGUI* pgSGUI;
    pgMoveGUI* pgMGUI;
    varShareClient<pgScanGUI::scanRes>* scanResC;
    std::vector<pgScanGUI::scanRes>* scans{nullptr};
    pgScanGUI::scanRes avg;
private Q_SLOTS:
    void onStartCalib(bool state);
    void onStartRecalc();
    void onSetAsCalib();
    void onToggle(bool state);
    void enableCorrectionChanged(bool state);
Q_SIGNALS:
    void sendToDisplay(pgScanGUI::scanRes scan);
};

#endif // CORRECTION_H
