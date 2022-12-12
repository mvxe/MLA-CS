#include "GUI/TAB_DEVICES/tab_devices.h"

tab_devices::tab_devices(QWidget* parent){
    layout=new QVBoxLayout;
    parent->setLayout(layout);

    layout->addStretch(0);
}

void tab_devices::addWidget(QWidget* widget){
    layout->insertWidget(layout->count()-1, widget);
}
