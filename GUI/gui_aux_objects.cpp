#include "gui_aux_objects.h"
#include "gui_includes.h"
#include "includes.h"

//  VAL SELECTOR

val_selector::val_selector(double initialValue, QString label, double min, double max, double precision):value(initialValue), unitIndex(0), val(value), index(unitIndex){
    init0(label, min, max, precision);
    layout->addStretch(0);
}

val_selector::val_selector(double initialValue, QString label, double min, double max, double precision, int initialIndex, std::vector<QString> labels):
        value(initialValue), unitIndex(initialIndex), labels(labels), val(value), index(unitIndex){
    init0(label, min, max, precision);
    init1();
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
    spinbox->setSingleStep(pow(10,-precision));
    spinbox->setValue(value);
    connect(spinbox, SIGNAL(valueChanged(double)), this, SLOT(on_value_change(double)));
    connect(this, SIGNAL(_changed(double, int)), this, SLOT(setValue(double, int)));
}
void val_selector::setMaximum(double max){
    spinbox->setMaximum(max);
}
void val_selector::setMinimum(double min){
    spinbox->setMinimum(min);
}
void val_selector::setLabel(QString label){
    _label->setText(label);
}

void val_selector::init1(){
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
    unit->activeAction=unit->menu()->actions()[unitIndex];
    unit->setText(unit->activeAction->text());
}


void val_selector::on_menu_change(){
    if(unit->menu()->activeAction()==nullptr) return;
    unit->activeAction=unit->menu()->activeAction();
    unit->setText(unit->activeAction->text());
    unitIndex=0;
    for (int i=0;i!=unit->menu()->actions().size();i++) if(unit->menu()->actions()[i]==unit->activeAction) {
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

void val_selector::setValue(double nvalue, int index){
    spinbox->setValue(nvalue);
    if(index>=0 && index<labels.size()){
        unit->activeAction=unit->menu()->actions()[index];
        unit->setText(unit->activeAction->text());
    }
}
void val_selector::set(std::string valueAndUnit){
    size_t dend;
    value = std::stod(valueAndUnit,&dend);
    if(labels.empty()) Q_EMIT _changed(value, -1);
    else for (int i=0; i!=labels.size(); i++){
        if(labels[i].toStdString()==valueAndUnit.substr(dend+1)){
            Q_EMIT _changed(value, i);
            return;
        }
    }
}
std::string val_selector::get(){
    if(labels.empty()) return util::toString(value.load());
    else return util::toString(value.load()," ",labels[unitIndex].toStdString());
}


//  SIMPLE SELECTOR

smp_selector::smp_selector(QString label, int initialIndex, std::vector<QString> labels):  _index(initialIndex), labels(labels){
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
    for(auto& ln : labels){
        QAction* action=new QAction;
        action->setText(ln);
        _sBtn->menu()->addAction(action);
    }
    connect(_sBtn->menu(), SIGNAL(aboutToHide()), this, SLOT(on_menu_change()));
    connect(this, SIGNAL(_changed(int)), this, SLOT(setIndex(int)));

    if (_index>=labels.size()) _index=0;
    _sBtn->activeAction=_sBtn->menu()->actions()[_index];
    _sBtn->setText(_sBtn->activeAction->text());
}

void smp_selector::addWidget(QWidget* widget){layout->insertWidget(layout->count()-1, widget);}

void smp_selector::on_menu_change(){
    if(_sBtn->menu()->activeAction()==nullptr) return;
    _sBtn->activeAction=_sBtn->menu()->activeAction();
    _sBtn->setText(_sBtn->activeAction->text());
    _index=0;
    for (int i=0;i!=_sBtn->menu()->actions().size();i++) if(_sBtn->menu()->actions()[i]==_sBtn->activeAction) {
        _index=i;
        break;
    }
    Q_EMIT changed();
    Q_EMIT changed(_index);
}
void smp_selector::setIndex(int index){
    if(index>=_sBtn->menu()->actions().size()) return;
    _sBtn->menu()->setActiveAction(_sBtn->menu()->actions()[index]);
    on_menu_change();
}
void smp_selector::set(std::string label){
    for (int i=0; i!=labels.size(); i++){
        if(labels[i].toStdString()==label){
            Q_EMIT _changed(i);
            break;
        }
    }
}
std::string smp_selector::get(){
    return getLabel(_index);
}
std::string smp_selector::getLabel(int index){
    if(index<0 || index>=labels.size()) return "n/a";
    return labels[index].toStdString();
}

// TAB WIDGET DISPLAY SELECTOR


twd_selector::twd_selector(std::string menu, std::string init, bool twidSetMargin, bool addStretch, bool addShown): showSel(menu!="" || init!=""){
    layout=new QVBoxLayout;
    layout->setMargin(0);
    this->setLayout(layout);

    connect(this, SIGNAL(_changed(int)), this, SLOT(setIndex(int)));
    if(addShown) showBtn=new QPushButton("< show >");
    if(showSel){
        select=new QScrollToolButton();
        select->setAutoRaise(true);
        select->setToolButtonStyle(Qt::ToolButtonTextOnly);
        select->setPopupMode(QToolButton::InstantPopup);
        select->setMenu(new QMenu);
        connect(select->menu(), SIGNAL(aboutToHide()), this, SLOT(on_menu_change()));
        if(menu!="" && addShown) layout->insertWidget(layout->count(), new twid(new QLabel(QString::fromStdString(menu)) ,select, showBtn, twidSetMargin));
        else if(menu!="") layout->insertWidget(layout->count(), new twid(new QLabel(QString::fromStdString(menu)) ,select, twidSetMargin));
        else       layout->insertWidget(layout->count(), select);
        select->setText(QString::fromStdString(init));
    }
    if(addShown){
        connect(showBtn, SIGNAL(released()), this, SLOT(onClicked()));
        showBtn->setFlat(true);
        layout->setSpacing(0);
        wI=new QWidget;
        layouts=new QVBoxLayout;
        wI->setLayout(layouts);
        layouts->setMargin(0);
        layout->addWidget(wI);
        wI->setVisible(false);
    }else layouts=layout;
    if(addStretch) layouts->addStretch(0);
    insertOfs=(addStretch)?1:0;
}
void twd_selector::addWidget(QWidget* widget, QString label){
    widgets.push_back(widget);
    if(!label.isEmpty()) labels.push_back(label.toStdString());
    else labels.push_back(util::toString(widgets.size()-1));
    if(showSel){
        widget->hide();
        QAction* action=new QAction;
        action->setText(label);
        select->menu()->addAction(action);
    }
    layouts->insertWidget(layouts->count()-insertOfs, widget);
}
void twd_selector::setIndex(int index){
    if(index>=select->menu()->actions().size()) return;
    select->menu()->setActiveAction(select->menu()->actions()[index]);
    on_menu_change();
}
void twd_selector::on_menu_change(){
    if(select->menu()->activeAction()==nullptr) return;
    select->activeAction=select->menu()->activeAction();
    select->setText(select->activeAction->text());
    for (int i=0;i!=select->menu()->actions().size();i++) if(select->menu()->actions()[i]==select->activeAction) {
        if(active_index!=-1){
            widgets[active_index]->hide();
        }
        active_index=i;
        widgets[active_index]->show();
        Q_EMIT changed(active_index);
        return;
    }
}
void twd_selector::onClicked(){
    wI->setVisible(!wI->isVisible());
    showBtn->setText((wI->isVisible())?"< hide >":"< show >");
}
void twd_selector::set(std::string label){
    for (int i=0; i!=labels.size(); i++){
        if(labels[i]==label){
            Q_EMIT _changed(i);
            return;
        }
    }
}
std::string twd_selector::get(){
    return labels[active_index];
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

// CHECKBOX thread safe with get()/set() (compatible with rtoml)

checkbox_gs::checkbox_gs(bool initialState, QString label) : value(initialState){
    this->setChecked(value);
    this->setText(label);
    connect(this, SIGNAL(toggled(bool)), this, SLOT(on_toggled(bool)));
    connect(this, SIGNAL(_changed(bool)), this, SLOT(setChecked(bool)));
}
void checkbox_gs::on_toggled(bool state){
    value=state;
    Q_EMIT changed(value);
    Q_EMIT changed();
}
void checkbox_gs::setValue(bool nvalue){
    value=nvalue;
    Q_EMIT _changed(value);                 // should be thread safe
    Q_EMIT changed(value);
    Q_EMIT changed();
}
void checkbox_gs::set(bool nvalue){setValue(nvalue);}
bool checkbox_gs::get(){return val;}

// LINEEDIT thread safe with get()/set() (compatible with rtoml)

lineedit_gs::lineedit_gs(std::string initialState){
    this->setText(QString::fromStdString(initialState));
    connect(this, SIGNAL(textEdited(const QString &)), this, SLOT(on_textChanged(const QString &)));
}
void lineedit_gs::set(std::string nvalue){
    {std::lock_guard<std::mutex>lock(smx);
    this->setText(QString::fromStdString(nvalue));}
    Q_EMIT changed(nvalue);
    Q_EMIT changed();
}
std::string lineedit_gs::get(){
    std::lock_guard<std::mutex>lock(smx);
    return this->text().toStdString();
}
void lineedit_gs::on_textChanged(const QString &text){
    Q_EMIT changed(text.toStdString());
    Q_EMIT changed();
}


// BUTTON string thread safe with get()/set() (compatible with rtoml)

btnlabel_gs::btnlabel_gs(std::string initialState, bool flat){
    this->setText(QString::fromStdString(initialState));
    this->setFlat(flat);
    this->setStyleSheet("Text-align:left");
}
void btnlabel_gs::set(std::string nvalue){
    std::lock_guard<std::mutex>lock(smx);
    this->setText(QString::fromStdString(nvalue));
    Q_EMIT changed(nvalue);
    Q_EMIT changed();
}
std::string btnlabel_gs::get(){
    std::lock_guard<std::mutex>lock(smx);
    return this->text().toStdString();
}
// Adding scroll to QToolButton

void QScrollToolButton::wheelEvent(QWheelEvent *event){
    int index=-1;
    for(int i=0;i!=this->menu()->actions().size();i++){
        if(this->menu()->actions()[i]==activeAction)
        {index=i; break;}
    }
    index-=event->delta()/120;
    if(index>=0 && index<this->menu()->actions().size()){
        activeAction=this->menu()->actions()[index];
        this->menu()->setActiveAction(activeAction);
        Q_EMIT this->menu()->aboutToHide();
    }
}

// MOVE DIAL


moveDial::moveDial(){
    layout=new QHBoxLayout;
    lw=new QWidget;
    layoutv=new QVBoxLayout;
    move=new QPushButton;
    move->setMaximumSize(50,50);
    move->setMinimumSize(50,50);
    move->setText("Move");
    dial=new QDial;
    dial->setMaximumSize(50,50);
    dial->setMinimumSize(50,50);
    dial->setRange(0,359);
    dial->setWrapping(true);
    dial->setValue(270);
    connect(dial, SIGNAL(valueChanged(int)), this, SLOT(onDialChanged(int)));
    dis=new QDoubleSpinBox;
    dis->setRange(0,1000000);
    dis->setDecimals(3);
    dis->setValue(1);
    dis->setPrefix("[Distance] "); dis->setSuffix(" um");
    ang=new QDoubleSpinBox;
    ang->setRange(0,360);
    ang->setDecimals(3);
    ang->setValue(0);
    ang->setPrefix("[Angle] "); ang->setSuffix(" deg");
    connect(ang, SIGNAL(valueChanged(double)), this, SLOT(onAngChanged(double)));
    this->setLayout(layout);
    lw->setLayout(layoutv);
    layout->addWidget(move);
    layout->addWidget(lw);
    layout->addWidget(dial);
    layoutv->addWidget(dis);
    layoutv->addWidget(ang);
    layout->setMargin(0);
    layout->addStretch(0);
    layoutv->setMargin(0);
    connect(move, SIGNAL(released()), this, SLOT(onMove()));
}
void moveDial::onMove(){
    Q_EMIT doMove(dis->value()*cos(ang->value()/180*M_PI), -dis->value()*sin(ang->value()/180*M_PI));
}
void moveDial::onDialChanged(int val){
    int res=val-270;
    if(res<0) res+=360;
    ang->setValue(res);
}
void moveDial::onAngChanged(double val){
    int res=round(val+270);
    if(res>=360) res-=360;
    dial->setValue(res);
}

// HORIZONTAL LAYOUT WIDGET FOR CONVENIENCE

twid::twid(bool setmargin, bool setstretch){
    layout=new QHBoxLayout;
    this->setLayout(layout);
    if(setmargin) layout->setMargin(0);
    if(setstretch) layout->addStretch(0);
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Maximum);
}
void twid::addWidget(QWidget* widget, bool front){
    layout->insertWidget(front?0:(layout->count()-1), widget);
}
twid::twid(QWidget* widget, bool setmargin, bool setstretch): twid(setmargin, setstretch){
    addWidget(widget, true);
}

// VERTICAL LAYOUT WIDGET FOR CONVENIENCE

vtwid::vtwid(bool setmargin, bool setstretch){
    layout=new QVBoxLayout;
    this->setLayout(layout);
    if(setmargin) layout->setMargin(0);
    if(setstretch) layout->addStretch(0);
    setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);
}
void vtwid::addWidget(QWidget* widget, bool front){
    layout->insertWidget(front?0:(layout->count()-1), widget);
}
vtwid::vtwid(QWidget* widget, bool setmargin, bool setstretch): vtwid(setmargin, setstretch){
    addWidget(widget, true);
}

// HIDDEN CONTAINER

void hidCon::hhidCon(){
    layout=new QVBoxLayout;
    this->setLayout(layout);
    showBtn=new QPushButton("< show >");
    connect(showBtn, SIGNAL(released()), this, SLOT(onClicked()));
    showBtn->setFlat(true);
    mainTwid->addWidget(showBtn);
    layout->addWidget(mainTwid);
    wI=new QWidget;
    layoutI=new QVBoxLayout;
    wI->setLayout(layoutI);
    layout->setMargin(0);
    layoutI->setMargin(0);
    layout->addWidget(wI);
    wI->setVisible(false);
}
void hidCon::addWidget(QWidget* widget){
    layoutI->addWidget(widget);
}
void hidCon::onClicked(){
    wI->setVisible(!wI->isVisible());
    showBtn->setText((wI->isVisible())?"< hide >":"< show >");
}

// MORE HIDDEN CONTAINER

void mhidCon::mhhidCon(){
    layout=new QVBoxLayout;
    this->setLayout(layout);
    showBtn=new QPushButton(">");
    connect(showBtn, SIGNAL(released()), this, SLOT(onClicked()));
    showBtn->setFlat(true);
    mainTwid->addWidget(showBtn, true);
    layout->addWidget(mainTwid);
    wI=new QWidget;
    layoutI=new QVBoxLayout;
    wI->setLayout(layoutI);
    layout->setMargin(0);
    layoutI->setMargin(0);
    layout->addWidget(wI);
    wI->setVisible(false);
    for(auto& wid: mainTwid->findChildren<QWidget*>()) {if(wid!=showBtn) wid->setVisible(false);}
}
void mhidCon::addWidget(QWidget* widget){
    layoutI->addWidget(widget);
}
void mhidCon::onClicked(){
    wI->setVisible(!wI->isVisible());
    showBtn->setText((wI->isVisible())?"<":">");
    for(auto& wid: mainTwid->findChildren<QWidget*>()) if(wid!=showBtn) wid->setVisible(wI->isVisible());
}

// GUI HOVER QPushButton

void HQPushButton::enterEvent(QEvent *event){
    Q_EMIT changed(true);
}
void HQPushButton::leaveEvent(QEvent *event){
    Q_EMIT changed(false);
}
