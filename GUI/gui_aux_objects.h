#ifndef GUI_AUX_OBJECTS_H
#define GUI_AUX_OBJECTS_H
#include <string>
#include <vector>
#include <QWidget>
#include "UTIL/containers.h"

class QHBoxLayout;
class QLabel;
class QString;
class QDoubleSpinBox;
class QToolButton;

class val_selector : public QWidget{       //template for devices
    Q_OBJECT
public:
    val_selector(double initialValue, std::string varSaveName, QString label, double min, double max);
    val_selector(double initialValue, std::string varSaveName, QString label, double min, double max, int initialIndex, std::vector<QString> labels);
    val_selector(double initialValue, std::string varSaveName, QString label, double min, double max, std::atomic<bool>* changed);
    val_selector(double initialValue, std::string varSaveName, QString label, double min, double max, int initialIndex, std::vector<QString> labels, std::atomic<bool>* changed);
    const double& val{value};
    const int& index{unitIndex};

private:
    double value;
    std::atomic<bool>* _changed;
    cc_save<double> valueSave;
    int unitIndex;
    cc_save<int> unitIndexSave;

    QHBoxLayout* layout;
    QLabel* _label;
    QDoubleSpinBox* spinbox;
    QToolButton* unit;

    void init0(QString label, double min, double max);
    void init1(std::vector<QString> labels);
private Q_SLOTS:
    void on_menu_change();
    void on_value_change(double nvalue);
};


#endif // GUI_AUX_OBJECTS_H
