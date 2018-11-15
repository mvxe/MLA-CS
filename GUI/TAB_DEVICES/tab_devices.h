#ifndef _CONFIG_TAB_DEVICES_H
#define _CONFIG_TAB_DEVICES_H
#include "GUI/gui_includes.h"

class _dev_layout;

class tab_devices : public QWidget{
    Q_OBJECT
public:
    tab_devices(QWidget* parent);
private:
    //~tab_devices();   //no need for destructor, all the widgets that are children of the parent QWidget get destroyed on it's destruction
    void add_item(_dev_layout *item);

    QVBoxLayout* layout;

    QToolButton* addDev;
    std::vector<_dev_layout*> items;
private slots:
    void on_add_item_clicked();
};


/*********** device base ***********/


class _dev_layout : public QWidget{       //template for devices
    Q_OBJECT
public:
    _dev_layout(bool set=false);
    QWidget* widget;
private:
    QHBoxLayout* layout;
    std::vector<QWidget*> items;

    QToolButton* TB_device;
    QLabel* LB_stat;
    QToolButton* TB_status;
    QLabel* LB_set;

    struct _sTB_btns{
        std::string text;
        std::string icon;
        const char* method;
    };
    static const int BN_dev=2;
    _sTB_btns _sTB_device[BN_dev]{
      {"Remove",":/gtk-no.svg",SLOT(on_device_remove())},
      {"Rename",":/info.svg",SLOT(on_device_rename())}
    };
    static const int BN_sta=3;
    _sTB_btns _sTB_status[BN_sta]{
      {"Disconnect",":/player_stop.svg",SLOT(on_status_disconnect())},
      {"Reconnect",":/redo.svg",SLOT(on_status_recconect())},
      {"Disable",":/gtk-media-pause.svg",SLOT(on_status_disable())}
    };
private slots:
    void on_TB_device_clicked();
    void on_device_remove();
    void on_device_rename();

    void on_TB_status_clicked();
    void on_status_disconnect();
    void on_status_recconect();
    void on_status_disable();
};


/*********** Here we define device widget types ***********/



#endif // _CONFIG_TAB_DEVICES_H
