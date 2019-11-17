#ifndef OTHER_SETTINGS_H
#define OTHER_SETTINGS_H
#include <QWidget>
class QVBoxLayout;
class QLabel;
class QCheckBox;
class val_selector;

class cameraSett: public QWidget{
    Q_OBJECT
public:
    cameraSett(std::atomic<bool>& getExpMinMax);
private:
    QVBoxLayout* layout;
    QCheckBox* measureFlag;
    QLabel* report0;
    val_selector* expSel;
    QLabel* report1;
    std::atomic<bool>& getExpMinMax;
    int expMin{-1}, expMax{-1};
public Q_SLOTS:
    void genReport();
    void onToggled(bool state);
    void doneExpMinmax(int min, int max);
};


#endif // OTHER_SETTINGS_H