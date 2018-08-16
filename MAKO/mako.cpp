#include "mako.h"

MAKO::MAKO() : vsys( AVT::VmbAPI::VimbaSystem::GetInstance()){
    sw.MAKO_cam_desc.set(new std::vector<_dcams>());
    VmbErrorType errc = vsys.Startup();
    if (errc!=VmbErrorSuccess){
        std::cerr<<"Vimba system startup error: "<<errc<<".\n";
        exit (EXIT_FAILURE);
    }
    errc = vsys.RegisterCameraListObserver(AVT::VmbAPI::ICameraListObserverPtr(new CamObserver()));
    if (errc!=VmbErrorSuccess){
        std::cerr<<"Vimba system event error: "<<errc<<".\n";
        exit (EXIT_FAILURE);
    }
}
MAKO::~MAKO(){
    if (vsys.Shutdown()!=VmbErrorSuccess){
        std::cerr<<"Vimba system shutdown error.\n";
        exit (EXIT_FAILURE);
    }
}

void MAKO::run(){    //this is the MAKO thread loop
    for (;;){
        if(sw.MAKO_list.get()) list_cams();
        else con_cams(true);

        std::this_thread::sleep_for (std::chrono::milliseconds(1000));

        if(sw.MAKO_end.get()){
            //TODO cleanup
            std::cout<<"MAKO thread exited.\n";
            sw.MAKO_end.set(false);
            return;
        }
    }
}

void MAKO::con_cams(bool ch){
    if (iuScope.ptr!=nullptr){
        if (ch && (iuScope.ID == sw.iuScopeID.get())) return;
        sw.MVM_ignore.set(true);
        iuScope.ptr->Close();
        iuScope.ptr.reset();
    }
    if (sw.iuScopeID.get()!="none"){
        bool found = false;
        for (int i=0;i!=cams.size();i++){
            if (sw.iuScopeID.get()==cams[i].ID){
                iuScope = cams[i];
                found = true;
                break;
            }
        }
        if (found){
            sw.MVM_ignore.set(true);
            if (iuScope.ptr->Open(VmbAccessModeFull) == VmbErrorSuccess){
                sw.iuScope_connected.set(true);
                //TODO when connected to camera
                return;
            }else{
                sw.MVM_ignore.set(false);
                iuScope.ptr.reset();
            }
        }
    }
    sw.iuScope_connected.set(false);
}

void MAKO::list_cams(){
    std::cerr<<"refreshing\n";
    sw.MAKO_list.set(false);
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
        std::cerr<<"Vimba error getting cameras: "<<errc<<".\n";
        exit (EXIT_FAILURE);
    }
    con_cams(false);
}
