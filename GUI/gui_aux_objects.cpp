#include "gui_aux_objects.h"
#include "gui_includes.h"
#include "includes.h"

//  VAL SELECTOR

val_selector::val_selector(double initialValue, QString label, double min, double max, double precision):
        valueSave(value, initialValue, nullptr, " "), unitIndexSave(unitIndex, 0, nullptr, " "){
    init0(label, min, max, precision);
    layout->addStretch(0);
}

val_selector::val_selector(double initialValue, QString label, double min, double max, double precision, int initialIndex, std::vector<QString> labels):
        valueSave(value, initialValue, nullptr, " "), unitIndexSave(unitIndex, initialIndex, nullptr, " "){
    init0(label, min, max, precision);
    init1(labels);
    layout->addStretch(0);
}

val_selector::val_selector(double initialValue, std::string varSaveName, QString label, double min, double max, double precision):
        valueSave(value, initialValue, &go.gui_config.save,varSaveName), unitIndexSave(unitIndex, 0, nullptr, " "){
    init0(label, min, max, precision);
    layout->addStretch(0);
}

val_selector::val_selector(double initialValue, std::string varSaveName, QString label, double min, double max, double precision, int initialIndex, std::vector<QString> labels):
        valueSave(value, initialValue, &go.gui_config.save,varSaveName), unitIndexSave(unitIndex, initialIndex, &go.gui_config.save,util::toString(varSaveName,"_index")){
    init0(label, min, max, precision);
    init1(labels);
    layout->addStretch(0);
}

void val_selector::init0(QString label, double min, double max, double precision){
    _label = new QLabel(label);
    layout = new QHBoxLayout();
    spinbox = new QDoubleSpinBox();

    this->setLayout(layout);
    layout->addWidget(_label);
    layout->addWidget(spinbox);
    layout->setMargin(0);

    spinbox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    spinbox->setRange(min,max);
    spinbox->setDecimals(precision);
    spinbox->setValue(value);
    connect(spinbox, SIGNAL(valueChanged(double)), this, SLOT(on_value_change(double)));
}

void val_selector::init1(std::vector<QString> labels){
    unit = new QScrollToolButton();
    layout->addWidget(unit);
    unit->setAutoRaise(true);
    unit->setToolButtonStyle(Qt::ToolButtonTextOnly);
    unit->setPopupMode(QToolButton::InstantPopup);
    unit->setMenu(new QMenu);
    for(int i=0;i!=labels.size();i++){
        QAction* action=new QAction;
        action->setText(labels[i]);
        unit->menu()->addAction(action);
    }
    connect(unit->menu(), SIGNAL(aboutToHide()), this, SLOT(on_menu_change()));

    if (unitIndex>=labels.size()) unitIndex=0;
    unit->menu()->setActiveAction( unit->menu()->actions()[unitIndex] );
    unit->setText(unit->menu()->activeAction()->text());
}


void val_selector::on_menu_change(){
    if(unit->menu()->activeAction()==nullptr) return;
    unit->setText(unit->menu()->activeAction()->text());
    unitIndex=0;
    for (int i=0;i!=unit->menu()->actions().size();i++) if(unit->menu()->actions()[i]==unit->menu()->activeAction()) {
        unitIndex=i;
        break;
    }
    Q_EMIT changed();
    Q_EMIT changed(unitIndex);
    Q_EMIT changed(value, unitIndex);
}

void val_selector::on_value_change(double nvalue){
    value=nvalue;
    Q_EMIT changed();
    Q_EMIT changed(value);
    Q_EMIT changed(value, unitIndex);
}

void val_selector::setValue(double nvalue){
    spinbox->setValue(nvalue);
}


//  SIMPLE SELECTOR

smp_selector::smp_selector(QString label, int initialIndex, std::vector<QString> labels):
    indexSave(_index, initialIndex, nullptr, " "){
    init(label, labels);
}
smp_selector::smp_selector(std::string varSaveName, QString label, int initialIndex, std::vector<QString> labels):
    indexSave(_index, initialIndex, &go.gui_config.save,varSaveName){
    init(label, labels);
}

void smp_selector::init(QString label, std::vector<QString> labels){
    _label = new QLabel(label);
    layout = new QHBoxLayout();
    _sBtn = new QScrollToolButton();

    this->setLayout(layout);
    layout->addWidget(_label);

    layout->addWidget(_sBtn);
    layout->setMargin(0);
    layout->addStretch(0);

    _sBtn->setAutoRaise(true);
    _sBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    _sBtn->setPopupMode(QToolButton::InstantPopup);
    _sBtn->setMenu(new QMenu);
    for(int i=0;i!=labels.size();i++){
        QAction* action=new QAction;
        action->setText(labels[i]);
        _sBtn->menu()->addAction(action);
    }
    connect(_sBtn->menu(), SIGNAL(aboutToHide()), this, SLOT(on_menu_change()));

    if (_index>=labels.size()) _index=0;
    _sBtn->menu()->setActiveAction( _sBtn->menu()->actions()[_index] );
    _sBtn->setText(_sBtn->menu()->activeAction()->text());
}

void smp_selector::addWidget(QWidget* widget){layout->insertWidget(layout->count()-1, widget);}

void smp_selector::on_menu_change(){
    if(_sBtn->menu()->activeAction()==nullptr) return;
    _sBtn->setText(_sBtn->menu()->activeAction()->text());
    _index=0;
    for (int i=0;i!=_sBtn->menu()->actions().size();i++) if(_sBtn->menu()->actions()[i]==_sBtn->menu()->activeAction()) {
        _index=i;
        break;
    }
    Q_EMIT changed();
    Q_EMIT changed(_index);
}

// TAB WIDGET DISPLAY SELECTOR


twd_selector::twd_selector(bool showSel): showSel(showSel){
    layout=new QVBoxLayout;
    layout->addStretch(0);
    layout->setMargin(0);
    this->setLayout(layout);
    if(showSel){
        select=new QScrollToolButton();
        select->setAutoRaise(true);
        select->setToolButtonStyle(Qt::ToolButtonTextOnly);
        select->setPopupMode(QToolButton::InstantPopup);
        select->setMenu(new QMenu);
        connect(select->menu(), SIGNAL(aboutToHide()), this, SLOT(on_menu_change()));
        layout->insertWidget(layout->count()-1, select);
    }
}
void twd_selector::addWidget(QWidget* widget, QString label){
    widgets.push_back(widget);
    if(showSel){
        widget->hide();
        QAction* action=new QAction;
        action->setText(label);
        select->menu()->addAction(action);
        select->setText("Select settings");
    }
    layout->insertWidget(layout->count()-1, widget);
}
void twd_selector::on_menu_change(){
    if(select->menu()->activeAction()==nullptr) return;
    select->setText(select->menu()->activeAction()->text());
    for (int i=0;i!=select->menu()->actions().size();i++) if(select->menu()->actions()[i]==select->menu()->activeAction()) {
        if(active_index!=-1){
            widgets[active_index]->hide();
        }
        active_index=i;
        widgets[active_index]->show();
        Q_EMIT changed(active_index);
        return;
    }
}

// GUI adaptiveScrollBar

adScrlBar::adScrlBar(int Hsize, int Vsize): Hsize(Hsize){
    this->setMouseTracking(false);
    this->setFrameShape(QFrame::Box);
    this->setFrameShadow(QFrame::Plain);
    this->setScaledContents(false);
    this->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    cv::Mat mat(Vsize,Hsize,CV_8U);
    for(int i=0;i!=Hsize;i++) for(int j=0;j!=Vsize;j++)
        mat.at<uchar>(j,i)=255./Hsize*(Hsize-i);
    this->setPixmap(QPixmap::fromImage(QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Indexed8)));
}
void adScrlBar::wheelEvent(QWheelEvent *event){
    Q_EMIT change(event->delta()*(Hsize-event->pos().x()));
}
void adScrlBar::mouseReleaseEvent(QMouseEvent *event){
    Q_EMIT change(((event->button()==Qt::LeftButton)?-1:1)*100*(Hsize-event->pos().x()));
}

// GUI expanded adaptiveScrollBar

eadScrlBar::eadScrlBar(QString label, int Hsize, int Vsize, bool locked) : eadScrlBar(label, Hsize, Vsize){
    cLock=new QPushButton;
    cLock->setCheckable(true);
    cLock->setChecked(locked);
    cLock->setIcon(QPixmap(":/locked.svg"));
    layout->insertWidget(layout->count()-1, cLock);
    connect(cLock, SIGNAL(toggled(bool)), this, SLOT(on_lock(bool)));
    abar->setEnabled(!locked);
}
eadScrlBar::eadScrlBar(QString label, int Hsize, int Vsize){
    layout=new QHBoxLayout;
    abar=new adScrlBar(Hsize, Vsize);
    QLabel* Label=new QLabel(label);
    layout->addWidget(Label);
    layout->addStretch(0);
    layout->addWidget(abar);
    layout->setMargin(0);
    this->setLayout(layout);
}

// CHECKBOX WITH SAVE

checkbox_save::checkbox_save(bool initialState, std::string varSaveName, QString label): valueSave(value, initialState, &go.gui_config.save,varSaveName){
    this->setChecked(value);
    this->setText(label);
    connect(this, SIGNAL(toggled(bool)), this, SLOT(on_toggled(bool)));
}
void checkbox_save::on_toggled(bool state){
    value=state;
    Q_EMIT changed(value);
    Q_EMIT changed();
}
void checkbox_save::setValue(bool nvalue){
    value=nvalue;
    this->setChecked(value);
}



// Adding scroll to QToolButton

void QScrollToolButton::wheelEvent(QWheelEvent *event){
    int index=-1;
    for(int i=0;i!=this->menu()->actions().size();i++){
        if(this->menu()->actions()[i]==this->menu()->activeAction())
        {index=i; break;}
    }
    index-=event->delta()/120;
    if(index>=0 && index<this->menu()->actions().size()){
        this->menu()->setActiveAction(this->menu()->actions()[index]);
        Q_EMIT this->menu()->aboutToHide();
    }
}

