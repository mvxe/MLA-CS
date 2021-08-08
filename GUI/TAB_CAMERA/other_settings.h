#ifndef OTHER_SETTINGS_H
#define OTHER_SETTINGS_H
#include <QWidget>
#include "UTIL/.rtoml/rtoml.hpp"
class QVBoxLayout;
class QLabel;
class QCheckBox;
class val_selector;

class cameraSett: public QWidget{
    Q_OBJECT
public:
    cameraSett(std::atomic<bool>& getExpMinMax);
    rtoml::vsr conf;                //configuration map
    double setExposurePreset(int N);      //N=0 is normal operation, N=1 is for laser beam calibration, returns the applied setting in us
private:
    QVBoxLayout* layout;
    QCheckBox* measureFlag;
    QLabel* report0;
    val_selector* expSel;
    QLabel* report1;
    std::atomic<bool>& getExpMinMax;
    int expMin{-1}, expMax{-1};
    QCheckBox* LEDon;
public Q_SLOTS:
    void genReport();
    void onToggled(bool state);
    void doneExpMinmax(int min, int max);
    void onLEDToggle(bool state);
};


#endif // OTHER_SETTINGS_H
