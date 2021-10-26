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
    const std::atomic<double>& val;         // thread safe
    const std::atomic<int>& index;          // thread safe

    void set(std::string valueAndUnit);     // thread safe, for toml::vsr
    std::string get();                      // thread safe, for toml::vsr

    void setMaximum(double max);
    void setMinimum(double min);
public Q_SLOTS:
    void setValue(double nvalue, int index=-1); // not thread safe, no change to index if index=-1
private:
    std::atomic<double> value;
    std::atomic<int> unitIndex;

    QHBoxLayout* layout;
    QLabel* _label;
    QDoubleSpinBox* spinbox;
    QScrollToolButton* unit;
    std::vector<QString> labels;            // for get()/set()

    void init0(QString label, double min, double max, double precision);
    void init1();
private Q_SLOTS:
    void on_menu_change();
    void on_value_change(double nvalue);
Q_SIGNALS:
    void changed();
    void changed(double value);
    void changed(double value, int index);
    void changed(int index);
    void _changed(double value, int index); // do not use
};

//  SIMPLE SELECTOR

class smp_selector : public QWidget{       //template for devices
    Q_OBJECT
public:
    smp_selector(QString label, int initialIndex, std::vector<QString> labels);
    const std::atomic<int>& index{_index};  // thread safe
    void addWidget(QWidget* widget);

    void set(std::string label);            // thread safe, for toml::vsr
    std::string get();                      // thread safe, for toml::vsr
    std::string getLabel(int index);
private:
    std::atomic<int> _index;

    QHBoxLayout* layout;
    QLabel* _label;
    QScrollToolButton* _sBtn;
    std::vector<QString> labels;            // for get()/set()
private Q_SLOTS:
    void on_menu_change();
    void setIndex(int index);
Q_SIGNALS:
    void changed();
    void changed(int index);
    void _changed(int index);               // do not use
};

// TAB WIDGET DISPLAY SELECTOR - add widgets with addWidget

class twd_selector : public QWidget{
    Q_OBJECT
public:
    twd_selector(std::string menu="", std::string init="", bool twidSetMargin=true, bool addStretch=true, bool addShown=false); //if menu=="" it wont show the selector
    void addWidget(QWidget* widget, QString label="");
    void timerStop();
    void timerStart();
    bool showSel;
    const std::atomic<int>& index{active_index};    // thread safe

    void set(std::string label);            // thread safe (assuming no more calls to addWidget), for toml::vsr
    std::string get();                      // thread safe (assuming no more calls to addWidget), for toml::vsr
public Q_SLOTS:
    void setIndex(int index);
private:
    QVBoxLayout* layout;
    QVBoxLayout* layouts;
    QScrollToolButton* select;
    std::vector<QWidget*> widgets;
    std::vector<std::string> labels;        // for get()/set()
    int insertOfs;
    std::atomic<int> active_index{-1};

    QPushButton* showBtn;   //for addShown
    QWidget* wI;
Q_SIGNALS:
    void changed(int index);
    void _changed(int index);               // do not use
private Q_SLOTS:
    void on_menu_change();
    void onClicked();   //for addShown
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



//  CHECKBOX thread safe with get()/set() (compatible with rtoml)

class checkbox_gs : public QCheckBox{
    Q_OBJECT
public:
    checkbox_gs(bool initialState, QString label);
    const std::atomic<bool>& val{value};    // thread safe
    void setValue(bool nvalue);             // thread safe

    void set(bool nvalue);          // thread safe, for toml::vsr
    bool get();                     // thread safe, for toml::vsr

private:
    std::atomic<bool> value;
private Q_SLOTS:
    void on_toggled(bool state);
Q_SIGNALS:
    void changed();
    void changed(bool state);
    void _changed(bool state);      // do not use
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
    twid(bool setmargin=true, bool setstretch=true);
    twid(QWidget* widget, bool setmargin=true, bool setstretch=true);
    template<typename... Args> twid(QWidget* widget, Args... args);
    void addWidget(QWidget* widget, bool front=false);
    QHBoxLayout* layout{nullptr};
};
template<typename... Args> twid::twid(QWidget* widget, Args... args): twid(args...){
    addWidget(widget, true);
}

// VERTICAL LAYOUT WIDGET FOR CONVENIENCE (HAS NO MARGIN, AND STRETCH)

class vtwid  : public QWidget{
    Q_OBJECT
public:
    vtwid(bool setmargin=true, bool setstretch=true);
    vtwid(QWidget* widget, bool setmargin=true, bool setstretch=true);
    template<typename... Args> vtwid(QWidget* widget, Args... args);
    void addWidget(QWidget* widget, bool front=false);
private:
    QVBoxLayout* layout{nullptr};
};
template<typename... Args> vtwid::vtwid(QWidget* widget, Args... args): vtwid(args...){
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

// MORE HIDDEN CONTAINER - click a button to reveal/hide content

class mhidCon : public QWidget{
    Q_OBJECT
public:
    template<typename... Args> mhidCon(Args... args);
    void mhhidCon();
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
template<typename... Args> mhidCon::mhidCon(Args... args): mainTwid(new twid(args...)){
    mhhidCon();
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
