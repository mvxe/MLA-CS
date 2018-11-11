#ifndef _CONFIG_TAB_CAMERA_H
#define _CONFIG_TAB_CAMERA_H
#include "GUI/gui_includes.h"

class tab_camera{
private:
    std::mutex dismx;
protected:
    cv::Mat* onDisplay;
public:
    tsbool disable{&dismx};

    double xps_x_sen, xps_y_sen, xps_z_sen;
    cc_save<double> xps_x_sen_save{xps_x_sen, 100000,&go.gui_config.save,"xps_x_sen"};
    cc_save<double> xps_y_sen_save{xps_y_sen, 100000,&go.gui_config.save,"xps_y_sen"};
    cc_save<double> xps_z_sen_save{xps_z_sen, 100000,&go.gui_config.save,"xps_z_sen"};
};


#endif // _CONFIG_TAB_CAMERA_H
