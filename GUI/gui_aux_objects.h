#ifndef GUI_AUX_OBJECTS_H
#define GUI_AUX_OBJECTS_H
#include <string>
#include <vector>
#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QTimer>
#include <QScrollArea>
#include "UTIL/containers.h"
#include "UTIL/utility.h"

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
    val_selector(double initialValue, QString label, double min, double max, double precision, bool addStretch=true);
    val_selector(double initialValue, QString label, double min, double max, double precision, int initialIndex, std::vector<QString> labels);
    const std::atomic<double>& val;         // thread safe
    const std::atomic<int>& index;          // thread safe

    void set(std::string valueAndUnit);     // thread safe, for toml::vsr
    std::string get();                      // thread safe, for toml::vsr

    void setMaximum(double max);
    void setMinimum(double min);
    void setLabel(QString label);
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

//  Thread safe label

class ts_label : public QLabel{
    Q_OBJECT
public:
    ts_label(QString label, QString pixmap="");
    void set(QString str);                  // thread safe
    void setPix(QString str);               // thread safe
private Q_SLOTS:
    void on_str_change(QString str);
    void on_pix_change(QString pix);
Q_SIGNALS:
    void _changed_str(QString str);         // do not use
    void _changed_pix(QString pix);         // do not use
};


//  SIMPLE SELECTOR

class smp_selector : public QWidget{       //template for devices
    Q_OBJECT
public:
    smp_selector(QString label, int initialIndex, std::vector<QString> labels);
    const std::atomic<int>& index{_index};  // thread safe
    void addWidget(QWidget* widget);
    void replaceLabels(int index, std::vector<QString> labels);

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
    void aboutToShow();
};

// TAB WIDGET DISPLAY SELECTOR - add widgets with addWidget

class twd_selector : public QScrollArea{
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
public:
    bool eventFilter(QObject *obj, QEvent *event) override;     // disable scroll wheel event that conflicts with some of our custom widgets
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

// LINEEDIT thread safe with get()/set() (compatible with rtoml)

class lineedit_gs : public QLineEdit{
    Q_OBJECT
public:
    lineedit_gs(std::string initialState);
    void set(std::string nvalue);          // thread safe, for toml::vsr
    std::string get();                     // thread safe, for toml::vsr
private:
    std::mutex smx;
private Q_SLOTS:
    void on_textChanged(const QString &text);
Q_SIGNALS:
    void changed();
    void changed(std::string text);
};

// BUTTON string thread safe with get()/set() (compatible with rtoml)
// you need to connect signal released() to your own function, and that function should then use set(std::string)

class btnlabel_gs : public QPushButton{
    Q_OBJECT
public:
    btnlabel_gs(std::string initialState, bool flat=true);
    void set(std::string nvalue);          // thread safe, for toml::vsr
    std::string get();                     // thread safe, for toml::vsr
private:
    std::mutex smx;
Q_SIGNALS:
    void changed();
    void changed(std::string text);
};

// HORIZONTAL LINE

class hline : public QLabel{
    Q_OBJECT
public:
    hline(){
        this->setFrameStyle(QFrame::HLine | QFrame::Plain);
    }
};

class vline : public QLabel{
    Q_OBJECT
public:
    vline(){
        this->setFrameStyle(QFrame::VLine | QFrame::Plain);
    }
};

// HIDDEN CONTAINER - click a button to reveal/hide content

class hidCon : public QWidget{
    Q_OBJECT
public:
    template<typename... Args> hidCon(Args... args);
    void hhidCon();
    void addWidget(QWidget* widget);
    void linkTo(hidCon* hc);
    void hide();
    void setShowLabel(QString label);
    void setHideLabel(QString label);
private:
    QString showLabel{"< show >"};
    QString hideLabel{"< hide >"};
    twid* mainTwid;
    QPushButton* showBtn;
    QVBoxLayout* layout;
    QWidget* wI;
    QVBoxLayout* layoutI;
    std::vector<hidCon*> linked;
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

// popup

class QStandardItemModel;
class wpopup : private QLabel{
    Q_OBJECT
public:
    static void show(const QPoint& pt, QString text="", int timeout_msec=0, std::string bgcolor="yellow"){
        wpopup* tmp = new wpopup;
        tmp->setWindowFlags(Qt::ToolTip);
        tmp->setStyleSheet(QString::fromStdString(util::toString("border: 1px solid black; background-color: ",bgcolor,";")));
        if(!text.isEmpty()) tmp->setText(text);
        tmp->move(pt);
        tmp->QLabel::show();
        if(timeout_msec>0) tmp->timer.singleShot(timeout_msec, tmp, SLOT(hide()));
    }
private:
    wpopup(){}
    QTimer timer;
private Q_SLOTS:
    void hide(){timer.stop(); QLabel::hide(); delete this;}
private:
    void mouseReleaseEvent(QMouseEvent *event){hide();}
};

// QLabel with wrap enabled
class WQLabel: public QLabel{
    Q_OBJECT
public:
    template<typename... Args> WQLabel(Args&&... args) : QLabel(std::forward<Args>(args)...){
        setWordWrap(true);
    }
};

namespace pixmaps{
    const QString px_online{":/emblem-ok.svg"};
    const QString px_offline{":/emblem-nowrite.svg"};
}

#endif // GUI_AUX_OBJECTS_H
