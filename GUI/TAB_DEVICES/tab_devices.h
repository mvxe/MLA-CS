#ifndef CONFIG_TAB_DEVICES_H
#define CONFIG_TAB_DEVICES_H
#include "GUI/gui_includes.h"

class tab_devices : public QWidget{
    Q_OBJECT
public:
    tab_devices(QWidget* parent);
    void addWidget(QWidget* widget);
private:
    QVBoxLayout* layout;

private Q_SLOTS:

};




#endif // CONFIG_TAB_DEVICES_H
