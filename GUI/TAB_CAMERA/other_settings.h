#ifndef OTHER_SETTINGS_H
#define OTHER_SETTINGS_H
#include <QWidget>
#include "UTIL/.rtoml/rtoml.hpp"
class QVBoxLayout;
class QLabel;
class QCheckBox;
class val_selector;
class pgMoveGUI;

class cameraSett: public QWidget{
    Q_OBJECT
public:
    cameraSett(std::atomic<bool>& getExpMinMax, pgMoveGUI* pgMov);
    rtoml::vsr conf;                //configuration map
private:
    QVBoxLayout* layout;
    QCheckBox* measureFlag;
    QLabel* report0;
    val_selector* expMirau;
    val_selector* expWriting;
    std::atomic<bool>& getExpMinMax;
    int expMin{-1}, expMax{-1};
    QCheckBox* LEDon;
    bool isMirau=true;
public Q_SLOTS:
    void genReport();
    void onToggled(bool state);
    void doneExpMinmax(int min, int max);
    void onLEDToggle(bool state);
    void changeObjective(bool isMirau);
    void _changeObjectiveMirau(){if(isMirau)changeObjective(true);};
    void _changeObjectiveWriting(){if(!isMirau)changeObjective(false);};
};


#endif // OTHER_SETTINGS_H
