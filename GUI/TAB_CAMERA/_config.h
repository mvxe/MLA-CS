#ifndef _CONFIG_TAB_CAMERA_H
#define _CONFIG_TAB_CAMERA_H
#include "GUI/includes.h"

class tab_camera{
private:
    std::mutex dismx;
public:
    tsbool disable{&dismx};

    int xps_x_sen, xps_y_sen, xps_z_sen;
    cc_save<int> xps_x_sen_save{xps_x_sen, 100,&go.gui_config.save,"xps_x_sen"};
    cc_save<int> xps_y_sen_save{xps_y_sen, 100,&go.gui_config.save,"xps_y_sen"};
    cc_save<int> xps_z_sen_save{xps_z_sen, 100,&go.gui_config.save,"xps_z_sen"};
};


#endif // _CONFIG_TAB_CAMERA_H
