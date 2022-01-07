#include "DEV/GCAM/gcam.h"
#include "globals.h"

GCAM::GCAM(){
    camobj::cobj=this;
    for(int i=0;i!=_CAM_NUM;i++){
        _c[i].ptr=new camobj(_c[i].cname);
        conf[_c[i].cname]=_c[i].ptr->conf;
    }
    //arv_g_type_init();
}
GCAM::~GCAM(){
    for(int i=0;i!=_CAM_NUM;i++) delete _c[i].ptr;
    arv_shutdown();
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
