#include "GUI/TAB_DEVICES/tab_devices.h"
#include "GUI/mainwindow.h"
#include "includes.h"

tab_devices::tab_devices(QWidget* parent){
    layout=new QVBoxLayout;
    parent->setLayout(layout);

    addDev=new QToolButton;
    addDev->setText("Add device");
    connect(addDev, SIGNAL(released()), this, SLOT(on_add_item_clicked()));
    addDev->setIcon(QPixmap(":/edit-add.svg"));
    addDev->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addDev->setAutoRaise(true);
    layout->addWidget(addDev);

    add_item(new _dev_layout(true));
    add_item(new _dev_layout(false));
    layout->addStretch(0);
}
void tab_devices::add_item(_dev_layout *item){
    layout->insertWidget(0,item->widget);
}

void tab_devices::on_add_item_clicked(){
    std::cerr<<"TODO:implement: add device\n";     //TODO implement
}


/*********** device base ***********/


_dev_layout::_dev_layout(bool set){
    widget=new QWidget;
    layout=new QHBoxLayout;
    layout->setMargin(0);   //for some reason it isnt 0 by default
    layout->addStretch(0);
    widget->setLayout(layout);

    TB_device=new QToolButton;
    TB_device->setAutoRaise(true);
    TB_device->setToolButtonStyle(Qt::ToolButtonTextOnly);
    TB_device->setPopupMode(QToolButton::InstantPopup);
    TB_device->setText("Device Name");
    TB_device->setMenu(new QMenu);
    connect(TB_device->menu(), SIGNAL(aboutToShow()), this, SLOT(on_TB_device_clicked()));
    for(int i=0;i!=BN_dev;i++){
        QAction* action=new QAction;
        action->setText(_sTB_device[i].text.c_str());
        action->setIcon(QPixmap(_sTB_device[i].icon.c_str()));
        TB_device->menu()->addAction(action);
        connect(action, SIGNAL(triggered()), this, _sTB_device[i].method);
    }

    TB_status=new QToolButton;
    TB_status->setAutoRaise(true);
    TB_status->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    TB_status->setPopupMode(QToolButton::InstantPopup);
    TB_status->setText("Offline");
    TB_status->setIconSize(QSize(24,24));
    TB_status->setIcon(QPixmap(":/emblem-nowrite.svg"));
    TB_status->setMenu(new QMenu);
    connect(TB_status->menu(), SIGNAL(aboutToShow()), this, SLOT(on_TB_status_clicked()));
    for(int i=0;i!=BN_sta;i++){
        QAction* action=new QAction;
        action->setText(_sTB_status[i].text.c_str());
        action->setIcon(QPixmap(_sTB_status[i].icon.c_str()));
        TB_status->menu()->addAction(action);
        connect(action, SIGNAL(triggered()), this, _sTB_status[i].method);
    }

    QLabel* LB_stat=new QLabel;
    LB_stat->setText("::");

    layout->insertWidget(layout->count()-1,TB_device);
    layout->insertWidget(layout->count()-1,LB_stat);
    layout->insertWidget(layout->count()-1,TB_status);

    if(set){
        QLabel* LB_set=new QLabel;
        LB_set->setText("::");
        layout->insertWidget(layout->count()-1,LB_set);
    }
}

void _dev_layout::on_TB_device_clicked(){
    //TODO implement (remove)
}
void _dev_layout::on_device_remove(){
    std::cerr<<"TODO:implement: removed\n";     //TODO implement
}
void _dev_layout::on_device_rename(){
    std::cerr<<"TODO:implement: renamed\n";     //TODO implement
}

void _dev_layout::on_TB_status_clicked(){
    std::cerr<<"TODO:implement: status clicked\n";  //TODO implement
}
void _dev_layout::on_status_disconnect(){
    std::cerr<<"TODO:implement: disconnecting\n";     //TODO implement
}
void _dev_layout::on_status_recconect(){
    std::cerr<<"TODO:implement: reconnecting\n";     //TODO implement
}
void _dev_layout::on_status_disable(){
    std::cerr<<"TODO:implement: disabling\n";     //TODO implement
}
