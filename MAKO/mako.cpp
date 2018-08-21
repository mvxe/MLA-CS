#include "mako.h"


camobj::camobj(MAKO *cobj, mxvar<std::string> &ID, mxvar<bool> &connected) : cobj(cobj),connected(connected), ID(ID), lost_frames_MAKO_VMB(0), VMBframes(N_FRAMES_MAKO_VMB){
    imgs_iter=0;
}

void camobj::start(){
    VMBo = AVT::VmbAPI::IFrameObserverPtr(new FrameObserver(cam.ptr));
    VmbInt64_t nPLS; // Payload size value
    cam.ptr->GetFeatureByName("PayloadSize",fet);
    fet->GetValue(nPLS);

    VmbInt64_t ret;
    cam.ptr->GetFeatureByName("Width",fet);
    fet->GetValue(ret);
    Xsize=ret;
    cam.ptr->GetFeatureByName("Height",fet);
    fet->GetValue(ret);
    Ysize=ret;
    cam.ptr->GetFeatureByName("PixelFormat",fet);
    fet->GetValue(ret);
    format_enum=ret;

    std::cerr<<"Xsize="<<Xsize<<"\n";
    std::cerr<<"Ysize="<<Ysize<<"\n";
    std::cerr<<"format="<<format_enum<<"\n";
    double ex;

    cam.ptr->GetFeatureByName("ExposureTime",fet);
    fet->SetValue(100.);
    cam.ptr->GetFeatureByName("ExposureTime",fet);
    fet->GetValue(ex);
    std::cerr<<"exposure(us)="<<ex<<"\n";

    for (int i=0; i!=VMBframes.size();i++){
        VMBframes[i].reset(new AVT::VmbAPI::Frame(nPLS));
        VMBframes[i]->RegisterObserver(VMBo);
        cam.ptr->AnnounceFrame(VMBframes[i]);
    }
    if (cam.ptr->StartCapture()!=VmbErrorSuccess) {end(); return;}
    for (int i=0; i!=VMBframes.size();i++) cam.ptr->QueueFrame(VMBframes[i]);
    cam.ptr->GetFeatureByName("AcquisitionStart",fet);      //TODO add acquisition control on demand
    fet->RunCommand();
}
void camobj::end(){
    cam.ptr->GetFeatureByName("AcquisitionStop",fet);       //TODO add acquisition control on demand
    fet->RunCommand ();
    cam.ptr->EndCapture();
    cam.ptr->FlushQueue();
    cam.ptr->RevokeAllFrames();
    for (int i=0; i!=VMBframes.size();i++){
        VMBframes[i]->UnregisterObserver();
        VMBframes[i].reset();
    }
    VMBo.reset();
}
void camobj::con_cam(bool ch){
    if (cam.ptr!=nullptr){
        if (ch && (cam.ID == ID.get())) return;
        sw.MVM_ignore.set(true);
        end();
        cam.ptr->Close();
        cam.ptr.reset();
    }
    if (ID.get()!="none"){
        bool found = false;
        for (int i=0;i!=cobj->cams.size();i++){
            if (ID.get()==cobj->cams[i].ID){
                cam = cobj->cams[i];
                found = true;
                break;
            }
        }
        if (found){
            sw.MVM_ignore.set(true);
            if (cam.ptr->Open(VmbAccessModeFull) == VmbErrorSuccess){
                connected.set(true);
                start();
                return;
            }else{
                sw.MVM_ignore.set(false);
                cam.ptr.reset();
            }
        }
    }
    connected.set(false);
}

/*########### MAKO ###########*/

MAKO::MAKO() : vsys( AVT::VmbAPI::VimbaSystem::GetInstance()), iuScope(this, sw.iuScopeID, sw.iuScope_connected){             //init new cameras here!
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
    bool ch=true;
    for (;;){
        if (sw.MAKO_list.get()) {
            list_cams();
            ch = false;
            sw.MAKO_list.set(false);
        }
        if (sw.MAKO_reco.get()){
            iuScope.con_cam(ch);   //add other cameras here too
            ch = true;
            sw.MAKO_reco.set(false);
        }

        std::this_thread::sleep_for (std::chrono::milliseconds(1000));

        if(sw.MAKO_end.get()){
            //TODO cleanup
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
        std::cerr<<"Vimba error getting cameras: "<<errc<<".\n";
        exit (EXIT_FAILURE);
    }
}
