#ifndef GCAM_H
#define GCAM_H

#include "DEV/GCAM/camobj.h"
#include "DEV/GCAM/arvwrap.h"
#include "DEV/GCAM/frame_queues.h"
#include <vector>
class QMenu;
class QToolButton;

class GCAM : public gcam_config, public protooth{
    friend class camobj;
public:
    GCAM();
    ~GCAM();
    rtoml::vsr conf;

    std::atomic<bool> MVM_list{true};
    void update();          // call once after conf is loaded to update GUI
private:
    void run();
    void list_cams();
};

class QGCAM: public QObject{
    Q_OBJECT
public:
    QGCAM(GCAM* parent): parent(parent){}
    GCAM* parent;
    camobj* CamObj;
    QMenu* menu;
    QToolButton* camSelect;
private Q_SLOTS:
    void cam_select_show();
    void cam_triggered(QAction *arg1);
};

#endif // GCAM_H

