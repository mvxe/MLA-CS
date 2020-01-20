#ifndef GUI_AUX_OBJECTS_H
#define GUI_AUX_OBJECTS_H
#include <string>
#include <vector>
#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QCheckBox>
#include <QPushButton>
#include "UTIL/containers.h"

class QHBoxLayout;
class QVBoxLayout;
class QString;
class QDoubleSpinBox;
class QTimer;
class QScrollToolButton;
class QDial;

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
    void addWidget(QWidget* widget);

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
    void changed(int index);
};

// TAB WIDGET DISPLAY SELECTOR

class twd_selector : public QWidget{
    Q_OBJECT
public:
    twd_selector(std::string menu="", std::string init="", bool twidSetMargin=true, bool addStretch=true, bool addShown=false); //if menu=="" it wont show the selector
    void addWidget(QWidget* widget, QString label="");
    void timerStop();
    void timerStart();
    bool showSel;
    const int& index{active_index};
    void setIndex(int index);
private:
    QVBoxLayout* layout;
    QVBoxLayout* layouts;
    QScrollToolButton* select;
    std::vector<QWidget*> widgets;
    int insertOfs;
    int active_index=-1;

    QPushButton* showBtn;   //for addShown
    QWidget* wI;
Q_SIGNALS:
    void changed(int index);
private Q_SLOTS:
    void on_menu_change();
    void onClicked();   //for addShown
};

// TAB WIDGET DISPLAY SELECTOR WITH INDEX SAVE

class twds_selector : public twd_selector{
    Q_OBJECT
public:
    twds_selector(std::string varSaveName, int initialIndex=-1, std::string menu="", std::string init="", bool twidSetMargin=true, bool addStretch=true, bool addShown=false);
    ~twds_selector();
    void doneAddingWidgets();   //call this after youve added all widgets with addWidget
private:
    bool _index;
    cc_save<bool> indexSave;
};

// GUI adaptiveScrollBar

class adScrlBar : public QLabel{
    Q_OBJECT
public:
    adScrlBar(int Hsize=100, int Vsize=10);
private:
    void wheelEvent(QWheelEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void emit(double value);
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
    void on_lock(bool state){abar->setEnabled(!state); Q_EMIT lock(state);}
Q_SIGNALS:
    void lock(bool locked);
};



//  CHECKBOX WITH SAVE

class checkbox_save : public QCheckBox{       //template for devices
    Q_OBJECT
public:
    checkbox_save(bool initialState, std::string varSaveName, QString label);
    const bool& val{value};
    void setValue(bool nvalue);

private:
    bool value;
    cc_save<bool> valueSave;
private Q_SLOTS:
    void on_toggled(bool state);
Q_SIGNALS:
    void changed();
    void changed(bool state);
};


// Adding scroll to QToolButton

class QScrollToolButton : public QToolButton{
    Q_OBJECT
public:
    QAction* activeAction{nullptr};
private:
    void wheelEvent(QWheelEvent *event);
};


// MOVE DIAL

class moveDial : public QWidget{
    Q_OBJECT
public:
    moveDial();

    QHBoxLayout* layout;
    QWidget* lw;
    QVBoxLayout* layoutv;
    QPushButton* move;
    QDial* dial;
    QDoubleSpinBox* dis;
    QDoubleSpinBox* ang;
private Q_SLOTS:
    void onMove();
    void onDialChanged(int val);
    void onAngChanged(double val);
Q_SIGNALS:
    void doMove(double dx, double dy);
};

// HORIZONTAL LAYOUT WIDGET FOR CONVENIENCE (HAS NO MARGIN, AND STRETCH)

class twid  : public QWidget{
    Q_OBJECT
public:
    twid(bool setmargin=true);
    twid(QWidget* widget, bool setmargin=true);
    template<typename... Args> twid(QWidget* widget, Args... args);
    void addWidget(QWidget* widget, bool front=false);
private:
    QHBoxLayout* layout{nullptr};
};
template<typename... Args> twid::twid(QWidget* widget, Args... args): twid(args...){
    addWidget(widget, true);
}

// VERTICAL LAYOUT WIDGET FOR CONVENIENCE (HAS NO MARGIN, AND STRETCH)

class vtwid  : public QWidget{
    Q_OBJECT
public:
    vtwid(bool setmargin=true);
    vtwid(QWidget* widget, bool setmargin=true);
    template<typename... Args> vtwid(QWidget* widget, Args... args);
    void addWidget(QWidget* widget, bool front=false);
private:
    QVBoxLayout* layout{nullptr};
};
template<typename... Args> vtwid::vtwid(QWidget* widget, Args... args): twid(args...){
    addWidget(widget, true);
}

// HORIZONTAL LINE

class hline : public QLabel{
    Q_OBJECT
public:
    hline(){
        this->setFrameStyle(QFrame::HLine | QFrame::Plain);
    }
};

// HIDDEN CONTAINER - click a button to reveal/hide content

class hidCon : public QWidget{
    Q_OBJECT
public:
    template<typename... Args> hidCon(Args... args);
    void hhidCon();
    void addWidget(QWidget* widget);
private:
    twid* mainTwid;
    QPushButton* showBtn;
    QVBoxLayout* layout;
    QWidget* wI;
    QVBoxLayout* layoutI;
private Q_SLOTS:
    void onClicked();
};
template<typename... Args> hidCon::hidCon(Args... args): mainTwid(new twid(args...)){
    hhidCon();
}

// GUI HOVER QPushButton

class HQPushButton : public QPushButton{
    Q_OBJECT
public:
    using QPushButton::QPushButton;
private:
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
Q_SIGNALS:
    void changed(bool hovering);
};

#endif // GUI_AUX_OBJECTS_H
