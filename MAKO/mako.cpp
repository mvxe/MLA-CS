#include "mako.h"

camobj::camobj(MAKO *cobj, mxvar<std::string> &ID, mxva<bool> &connected) : cobj(cobj),connected(connected), ID(ID), lost_frames_MAKO_VMB(0), VMBframes(N_FRAMES_MAKO_VMB), ackstatus(false), FQsPCcam(){}

void camobj::start(){
    VMBo = AVT::VmbAPI::IFrameObserverPtr(new FrameObserver(cam.ptr, &FQsPCcam));

    VmbInt64_t nPLS=wfun::get<VmbInt64_t>(cam.ptr,"PayloadSize");
    Xsize=wfun::get<VmbInt64_t>(cam.ptr,"Width");
    Ysize=wfun::get<VmbInt64_t>(cam.ptr,"Height");
    format_enum=wfun::get<VmbInt64_t>(cam.ptr,"PixelFormat");

/* TODO implement this in gui later on*/
    AVT::VmbAPI::FeaturePtrVector features;
    cam.ptr->GetFeatures(features);
    std::string fname[5];
    bool rw[2];
    for (int i=0;i!=features.size();i++){
        features[i]->GetName(fname[0]);
        features[i]->GetUnit(fname[1]);
        features[i]->GetRepresentation(fname[2]);
        features[i]->GetToolTip(fname[3]);
        features[i]->GetDescription(fname[4]);
        features[i]->IsReadable(rw[0]);
        features[i]->IsWritable(rw[1]);
        //std::cerr<<"feature name: "<<fname[0]<<"\n  Unit: "<<fname[1]<<"\n  Representation: "<<fname[2]<<"\n  ToolTip: "<<fname[3]<<"\n  Description: "<<fname[4];
        //std::cerr<<"\n  R/W: "<<rw[0]<<"/"<<rw[1]<<"\n\n";
    }
/*#####################################*/

    std::cerr<<"Xsize="<<Xsize<<"\n";
    std::cerr<<"Ysize="<<Ysize<<"\n";
    std::cerr<<"format="<<format_enum<<"\n";
    //for (int i=0;i!=imgs.size();i++) imgs[i]=cv::Mat(Xsize,Ysize, CV_8UC3);                                      //we set the img sizes and formats, redo/separate this if you add res settings!


    //wfun::set<double>(cam.ptr,"ExposureTime",sw.MAKO_expo*0.1);
    wfun::set<double>(cam.ptr,"ExposureTime",1000.);
    std::cerr<<"exposure(us)="<< wfun::get<double>(cam.ptr,"ExposureTime") <<"\n";

    ackFPS=wfun::get<double>(cam.ptr,"AcquisitionFrameRate");
    FQsPCcam.setCamFPS(ackFPS);
    std::cerr<<"ackFPS="<<ackFPS<<"\n";

    for (int i=0; i!=VMBframes.size();i++){
        VMBframes[i].reset(new AVT::VmbAPI::Frame(nPLS));
        VMBframes[i]->RegisterObserver(VMBo);
        cam.ptr->AnnounceFrame(VMBframes[i]);
    }
    if (cam.ptr->StartCapture()!=VmbErrorSuccess) {end(); return;}
    for (int i=0; i!=VMBframes.size();i++) cam.ptr->QueueFrame(VMBframes[i]);
}
void camobj::work(){
    if (connected.get(true)==false){
        ackstatus=false;
        return;
    }
    bool doack=false;
    if (FQsPCcam.isThereInterest()){
        if (!ackstatus) ackstatus=(wfun::run(cam.ptr,"AcquisitionStart")==VmbErrorSuccess);
        doack=ackstatus;
    }
    if(doack==false && ackstatus==true){
        wfun::run(cam.ptr,"AcquisitionStop");
        ackstatus=false;
    }
}
void camobj::end(){
    if (ackstatus) wfun::run(cam.ptr,"AcquisitionStop");
    ackstatus=false;
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
    sw.iuScope_img=iuScope.FQsPCcam.getNewFQ();                                                                               //add image queues here!

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
            iuScope.con_cam(ch);                                                                                                //add other cameras here too
            ch = true;
            sw.MAKO_reco.set(false);
        }

        iuScope.work();
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
        std::cerr<<"Vimba error getting cameras: "<<errc<<".\n";
        exit (EXIT_FAILURE);
    }
}
