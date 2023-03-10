#include "DEV/GCAM/gcam.h"
#include "GUI/gui_aux_objects.h"
#include "globals.h"
#include "qmenu.h"

GCAM::GCAM(){
    camobj::cobj=this;
    for(int i=0;i!=_CAM_NUM;i++){
        _c[i].ptr=new camobj(_c[i].cname);
        conf[_c[i].cname]=_c[i].ptr->conf;

        // add some GUI:
        _c[i].ptr->GUI_icon=new ts_label("",pixmaps::px_offline);
        _c[i].ptr->GUI_icon->setMaximumHeight(26);
        _c[i].ptr->GUI_icon->setMaximumWidth(26);
        _c[i].ptr->GUI_icon->setScaledContents(true);

        _c[i].ptr->qdo=new QGCAM(this);
        _c[i].ptr->qdo->camSelect=new QToolButton();
        _c[i].ptr->qdo->camSelect->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
        _c[i].ptr->qdo->camSelect->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
        _c[i].ptr->qdo->camSelect->setAutoRaise(true);
        _c[i].ptr->qdo->CamObj=_c[i].ptr;
        _c[i].ptr->qdo->menu = new QMenu();
        _c[i].ptr->qdo->menu->setToolTipsVisible(true);
        _c[i].ptr->qdo->camSelect->setMenu(_c[i].ptr->qdo->menu);
        QObject::connect(_c[i].ptr->qdo->menu, SIGNAL(aboutToShow()), _c[i].ptr->qdo, SLOT(cam_select_show()));
        QObject::connect(_c[i].ptr->qdo->camSelect, SIGNAL(triggered(QAction*)), _c[i].ptr->qdo, SLOT(cam_triggered(QAction*)));
        connectionGUI.push_back(new twid(_c[i].ptr->GUI_icon, new QLabel(QString::fromStdString(_c[i].cname)),_c[i].ptr->qdo->camSelect));
    }
    //arv_g_type_init();
}
GCAM::~GCAM(){
    for(int i=0;i!=_CAM_NUM;i++) delete _c[i].ptr;
    arv_shutdown();
}

void GCAM::update(){
    for(int i=0;i!=_CAM_NUM;i++)
        _c[i].ptr->qdo->camSelect->setText(QString::fromStdString("camera ID: "+_c[i].ptr->selected_ID.get()));
}

void GCAM::run(){    //this is the GCAM thread loop
    for (;;){
        if (MVM_list) {
            list_cams();
            MVM_list=false;
        }

        auto b4=std::chrono::system_clock::now();
        for(int i=0;i!=_CAM_NUM;i++)
            _c[i].ptr->work();
        if(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now()-b4).count()<1000)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if(end){
            for(int i=0;i!=_CAM_NUM;i++)
                if (_c[i].ptr->cam!=NULL)
                    _c[i].ptr->end();
            std::cout<<"GCAM thread exited.\n";
            done=true;
            return;
        }
    }
}

void GCAM::list_cams(){
    arv_update_device_list();
    int Ncams=arv_get_n_devices();
    std::lock_guard<std::mutex>lock(cam_desc_mux);
    cam_desc.clear();
    for (int i=0;i!=Ncams;i++){
        cam_desc.emplace_back();
        cam_desc.back().ID=arv_get_device_id(i);
        cam_desc.back().description+="Physical ID: " +std::string(arv_get_device_physical_id(i));
        cam_desc.back().description+="\nModel: "+std::string(arv_get_device_model(i));
        cam_desc.back().description+="\nSerial number: "+std::string(arv_get_device_serial_nbr(i));
        cam_desc.back().description+="\nVendor: "+std::string(arv_get_device_vendor(i));
        cam_desc.back().description+="\nAddress: "+std::string(arv_get_device_address(i));
        cam_desc.back().description+="\nProtocol: "+std::string(arv_get_device_protocol(i));
    }
    cam_desc.emplace_back(_dcams{"none",""});
}

void QGCAM::cam_select_show(){
    menu->clear();
    std::lock_guard<std::mutex>lock(parent->cam_desc_mux);
    for (int i=0;i!=parent->cam_desc.size();i++){
        QAction *actx = new QAction();
        actx->setText(QString::fromStdString(parent->cam_desc.at(i).ID));
        actx->setToolTip(QString::fromStdString(parent->cam_desc.at(i).description));
        menu->addAction(actx);
    }
    parent->MVM_list=true;
}

void QGCAM::cam_triggered(QAction *arg1){
    CamObj->selected_ID.set(arg1->text().toStdString());
    camSelect->setText("camera ID: "+arg1->text());
    CamObj->checkID=true;
}
