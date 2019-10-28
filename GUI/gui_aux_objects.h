#ifndef GUI_AUX_OBJECTS_H
#define GUI_AUX_OBJECTS_H
#include <string>
#include <vector>
#include <QWidget>
#include "UTIL/containers.h"

class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class QString;
class QDoubleSpinBox;
class QToolButton;
class QTimer;

//  VAL SELECTOR

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

//  SIMPLE SELECTOR

class smp_selector : public QWidget{       //template for devices
    Q_OBJECT
public:
    smp_selector(QString label, int initialIndex, std::vector<QString> labels);
    smp_selector(QString label, int initialIndex, std::vector<QString> labels, std::atomic<bool>* changed);
    smp_selector(std::string varSaveName, QString label, int initialIndex, std::vector<QString> labels);
    smp_selector(std::string varSaveName, QString label, int initialIndex, std::vector<QString> labels, std::atomic<bool>* changed);
    const int& index{_index};

private:
    std::atomic<bool>* _changed;
    int _index;
    cc_save<int> indexSave;

    QHBoxLayout* layout;
    QLabel* _label;
    QToolButton* _sBtn;

    void init(QString label, std::vector<QString> labels);
private Q_SLOTS:
    void on_menu_change();
};


// TAB WIDGET DISPLAY SELECTOR

class twd_selector : public QWidget{       //template for devices
    Q_OBJECT
public:
    twd_selector(bool showSel=true);
    void addWidget(QWidget* widget, QString label="/", QTimer* timer=nullptr);
    void timerStop();
    void timerStart();
    bool showSel;
    bool timSt=false;
private:
    QVBoxLayout* layout;
    QToolButton* select;
    std::vector<QWidget*> widgets;
    std::vector<QTimer*> timers;        //timers are only running when this settings tab is open : allow it to handle stuff
    int active_index=-1;

private Q_SLOTS:
    void on_menu_change();
};


// GUI JOYSTICK


#endif // GUI_AUX_OBJECTS_H
