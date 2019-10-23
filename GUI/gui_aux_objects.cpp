#include "gui_aux_objects.h"
#include "gui_includes.h"
#include "includes.h"

//  VAL SELECTOR

val_selector::val_selector(double initialValue, std::string varSaveName, QString label, double min, double max):
        valueSave(value, initialValue, &go.gui_config.save,varSaveName), unitIndexSave(unitIndex, 0, nullptr, " "), _changed(nullptr){
    init0(label, min, max);
    layout->addStretch(0);
}

val_selector::val_selector(double initialValue, std::string varSaveName, QString label, double min, double max, int initialIndex, std::vector<QString> labels):
        valueSave(value, initialValue, &go.gui_config.save,varSaveName), unitIndexSave(unitIndex, initialIndex, &go.gui_config.save,util::toString(varSaveName,"_index")), _changed(nullptr){
    init0(label, min, max);
    init1(labels);
    layout->addStretch(0);
}

val_selector::val_selector(double initialValue, std::string varSaveName, QString label, double min, double max, std::atomic<bool>* changed):
        valueSave(value, initialValue, &go.gui_config.save,varSaveName), unitIndexSave(unitIndex, 0, nullptr, " "), _changed(changed){
    init0(label, min, max);
    layout->addStretch(0);
}

val_selector::val_selector(double initialValue, std::string varSaveName, QString label, double min, double max, int initialIndex, std::vector<QString> labels, std::atomic<bool>* changed):
        valueSave(value, initialValue, &go.gui_config.save,varSaveName), unitIndexSave(unitIndex, initialIndex, &go.gui_config.save,util::toString(varSaveName,"_index")), _changed(changed){
    init0(label, min, max);
    init1(labels);
    layout->addStretch(0);
}

void val_selector::init0(QString label, double min, double max){
    _label = new QLabel(label);
    layout = new QHBoxLayout();
    spinbox = new QDoubleSpinBox();

    this->setLayout(layout);
    layout->addWidget(_label);
    layout->addWidget(spinbox);
    layout->setMargin(0);

    spinbox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    spinbox->setRange(min,max);
    spinbox->setValue(value);
    connect(spinbox, SIGNAL(valueChanged(double)), this, SLOT(on_value_change(double)));
}

void val_selector::init1(std::vector<QString> labels){
    unit = new QToolButton();
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
    for (int i=0;i!=unit->menu()->actions().size();i++) if(unit->menu()->actions()[i]==unit->menu()->activeAction()) {
        unitIndex=i;
        if(_changed!=nullptr) *_changed=true;
        return;
    }
    unitIndex=0;
    if(_changed!=nullptr) *_changed=true;
}

void val_selector::on_value_change(double nvalue){
    value=nvalue;
    if(_changed!=nullptr) *_changed=true;
}


//  SIMPLE SELECTOR

smp_selector::smp_selector(QString label, int initialIndex, std::vector<QString> labels):
    indexSave(_index, 0, nullptr, " "), _changed(nullptr){
    init(label, labels);
}
smp_selector::smp_selector(QString label, int initialIndex, std::vector<QString> labels, std::atomic<bool>* changed):
    indexSave(_index, 0, nullptr, " "), _changed(changed){
    init(label, labels);
}
smp_selector::smp_selector(std::string varSaveName, QString label, int initialIndex, std::vector<QString> labels):
    indexSave(_index, initialIndex, &go.gui_config.save,varSaveName), _changed(nullptr){
    init(label, labels);
}
smp_selector::smp_selector(std::string varSaveName, QString label, int initialIndex, std::vector<QString> labels, std::atomic<bool>* changed):
    indexSave(_index, initialIndex, &go.gui_config.save,varSaveName), _changed(changed){
    init(label, labels);
}

void smp_selector::init(QString label, std::vector<QString> labels){
    _label = new QLabel(label);
    layout = new QHBoxLayout();
    _sBtn = new QToolButton();

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

void smp_selector::on_menu_change(){
    if(_sBtn->menu()->activeAction()==nullptr) return;
    _sBtn->setText(_sBtn->menu()->activeAction()->text());
    for (int i=0;i!=_sBtn->menu()->actions().size();i++) if(_sBtn->menu()->actions()[i]==_sBtn->menu()->activeAction()) {
        _index=i;
        if(_changed!=nullptr) *_changed=true;
        return;
    }
    _index=0;
    if(_changed!=nullptr) *_changed=true;
}
