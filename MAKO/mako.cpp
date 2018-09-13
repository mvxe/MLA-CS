#include "mako.h"
#include "globals.h"

MAKO::MAKO() : vsys( AVT::VmbAPI::VimbaSystem::GetInstance()){
    sw.MAKO_cam_desc.set(new std::vector<_dcams>());
    VmbErrorType errc = vsys.Startup();
    if (errc!=VmbErrorSuccess){
        std::cerr<<"ERROR: Vimba system startup error: "<<errc<<".\n";
        go.quit();
    }
    errc = vsys.RegisterCameraListObserver(AVT::VmbAPI::ICameraListObserverPtr(new CamObserver()));
    if (errc!=VmbErrorSuccess){
        std::cerr<<"ERROR: Vimba system event error: "<<errc<<".\n";
        go.quit();
    }
}
MAKO::~MAKO(){
    if (vsys.Shutdown()!=VmbErrorSuccess){
        std::cerr<<"ERROR: Vimba system shutdown error.\n";
        go.quit();
    }
}

void MAKO::run(){    //this is the MAKO thread loop
    bool ch=true;
    for (;;){
        if (sw.MAKO_list.get()) {
            list_cams();
            ch = false;
            sw.MAKO_list.set(false);
        }
        if (sw.MAKO_reco.get()){
            iuScope.con_cam(ch);                                                                                                //add other cameras here too
            ch = true;
            sw.MAKO_reco.set(false);
        }

        iuScope.work();                                                                                                         //add other cameras here too
        std::this_thread::sleep_for (std::chrono::milliseconds(1));                                                             //TODO fix this delay with timers

        if(sw.MAKO_end.get()){
            if (iuScope.cam.ptr!=nullptr) iuScope.end();                                                                        //add other cameras here too
            std::cout<<"MAKO thread exited.\n";
            sw.MAKO_end.set(false);
            return;
        }
    }
}

void MAKO::list_cams(){
    cams.clear();
    VmbErrorType errc = vsys.GetCameras(cameras);
    if (errc == VmbErrorSuccess){
        sw.MAKO_cam_desc.get()->clear();
        for (int i=0;i!=cameras.size();i++){
            cams.emplace_back();
            cams.back().ptr = cameras[i];
            cams.back().ptr->GetID(cams.back().ID);
            std::string temp;
            sw.MAKO_cam_desc.get()->emplace_back();
            cams.back().ptr->GetID(temp); sw.MAKO_cam_desc.get()->back().ID=temp;
            cams.back().ptr->GetName(temp); sw.MAKO_cam_desc.get()->back().description+="Name: "+temp;
            cams.back().ptr->GetModel(temp); sw.MAKO_cam_desc.get()->back().description+="\nModel: "+temp;
            cams.back().ptr->GetSerialNumber(temp); sw.MAKO_cam_desc.get()->back().description+="\nSerialNumber: "+temp;
            cams.back().ptr->GetInterfaceID(temp); sw.MAKO_cam_desc.get()->back().description+="\nInterfaceID: "+temp;
        }
        sw.MAKO_cam_desc.get()->emplace_back();
        sw.MAKO_cam_desc.get()->back().ID = "none";
        sw.MAKO_cam_desc.get()->back().description = "";
    }
    else {
        std::cerr<<"ERROR: Vimba error getting cameras: "<<errc<<".\n";
        go.quit();
    }
}
