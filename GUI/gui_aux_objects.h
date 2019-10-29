#ifndef GUI_AUX_OBJECTS_H
#define GUI_AUX_OBJECTS_H
#include <string>
#include <vector>
#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include "UTIL/containers.h"

class QHBoxLayout;
class QVBoxLayout;
class QString;
class QDoubleSpinBox;
class QTimer;
class QPushButton;
class QScrollToolButton;

//  VAL SELECTOR

class val_selector : public QWidget{       //template for devices
    Q_OBJECT
public:
    val_selector(double initialValue, QString label, double min, double max, double precision);
    val_selector(double initialValue, QString label, double min, double max, double precision, int initialIndex, std::vector<QString> labels);
    val_selector(double initialValue, std::string varSaveName, QString label, double min, double max, double precision);
    val_selector(double initialValue, std::string varSaveName, QString label, double min, double max, double precision, int initialIndex, std::vector<QString> labels);
    const double& val{value};
    const int& index{unitIndex};
    void setValue(double nvalue);

private:
    double value;
    cc_save<double> valueSave;
    int unitIndex;
    cc_save<int> unitIndexSave;

    QHBoxLayout* layout;
    QLabel* _label;
    QDoubleSpinBox* spinbox;
    QScrollToolButton* unit;

    void init0(QString label, double min, double max, double precision);
    void init1(std::vector<QString> labels);
private Q_SLOTS:
    void on_menu_change();
    void on_value_change(double nvalue);
Q_SIGNALS:
    void changed();
    void changed(double value);
    void changed(double value, int index);
    void changed(int index);
};

//  SIMPLE SELECTOR

class smp_selector : public QWidget{       //template for devices
    Q_OBJECT
public:
    smp_selector(QString label, int initialIndex, std::vector<QString> labels);
    smp_selector(std::string varSaveName, QString label, int initialIndex, std::vector<QString> labels);
    const int& index{_index};

private:
    int _index;
    cc_save<int> indexSave;

    QHBoxLayout* layout;
    QLabel* _label;
    QScrollToolButton* _sBtn;

    void init(QString label, std::vector<QString> labels);
private Q_SLOTS:
    void on_menu_change();
Q_SIGNALS:
    void changed();
    void changed(double value);
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
    QScrollToolButton* select;
    std::vector<QWidget*> widgets;
    std::vector<QTimer*> timers;        //timers are only running when this settings tab is open : allow it to handle stuff
    int active_index=-1;
public: Q_SIGNALS:
    void changed(int index);
private Q_SLOTS:
    void on_menu_change();
};

// GUI adaptiveScrollBar

class adScrlBar : public QLabel{
    Q_OBJECT
public:
    adScrlBar(int Hsize=100, int Vsize=10);
private:
    void wheelEvent(QWheelEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    int Hsize;

Q_SIGNALS:
    void change(double magnitude);
};

// GUI expanded adaptiveScrollBar

class eadScrlBar : public QWidget{
    Q_OBJECT
public:
    eadScrlBar(QString label, int Hsize, int Vsize);
    eadScrlBar(QString label, int Hsize, int Vsize, bool locked);
    adScrlBar* abar;
    QHBoxLayout* layout;
    QPushButton* cLock;
private Q_SLOTS:
    void on_lock(bool state){abar->setEnabled(!state);}
};






// Adding scroll to QToolButton

class QScrollToolButton : public QToolButton{
    Q_OBJECT
private:
    void wheelEvent(QWheelEvent *event);
};

#endif // GUI_AUX_OBJECTS_H
