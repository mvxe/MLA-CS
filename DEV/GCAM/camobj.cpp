#include "DEV/GCAM/gcam.h"
#include "GUI/gui_aux_objects.h"

camobj::camobj(std::string strID) : camobj_config(strID), selected_ID(&mkmx,strID){
    conf["selected_ID"]=selected_ID;
    conf["cameraTimeout"]=cameraTimeout;
    conf["cameraTimeout"].comments().push_back(" Camera timeout in ms");
}
GCAM *camobj::cobj;
camobj::~camobj(){
    while(!FullBuffers.empty()){
        g_clear_object(&FullBuffers.front());
        FullBuffers.pop_front();
    }
}

void camobj::new_frame_ready (ArvStream *stream, camobj* camm)
{
    ArvBuffer *buffer;
    buffer = arv_stream_try_pop_buffer (stream);
    if(buffer != NULL){
        camm->Xsize=arv_buffer_get_image_width(buffer);
        camm->Ysize=arv_buffer_get_image_height(buffer);
        cv::Mat* freeMat = camm->FQsPCcam.getAFreeMatPtr();
        if(imgfor::ocv_type_get(arv_buffer_get_image_pixel_format(buffer)).ocv_type==-1) std::cerr<<"ERROR: Image type not recognized! Cannot convert to OpenCV format.\n";     //TODO: this cannot really handle cameras with BPP!=8,16,24...
        if(freeMat==nullptr){
            freeMat=new cv::Mat(camm->Ysize, camm->Xsize, imgfor::ocv_type_get(arv_buffer_get_image_pixel_format(buffer)).ocv_type);
        }
        else{
            camm->requeueFrame(freeMat);
            if(freeMat->size().width!=camm->Xsize || freeMat->size().height!=camm->Ysize){
                std::cerr<<"WARNING: frame resized from "<<freeMat->size().width<<", "<<freeMat->size().height<<" to "<<camm->Xsize<<", "<<camm->Ysize<<"\n";
                freeMat->release(); //just deletes the header, not the data
                freeMat=new cv::Mat(camm->Ysize, camm->Xsize, imgfor::ocv_type_get(arv_buffer_get_image_pixel_format(buffer)).ocv_type);
            }
        }
        size_t dsize;
        freeMat->data=(uchar*)(arv_buffer_get_data(buffer, &dsize));
        if(dsize!=freeMat->total()*freeMat->elemSize()) std::cerr<<"ERROR: expected frame size does not match returned size ("<<dsize<<" vs "<<freeMat->size()<<"), see camobj::new_frame_ready\n";

        camm->FullBuffers.push_back(buffer);
        //std::cerr<<"ERROR: BPP: "<<ARV_PIXEL_FORMAT_BIT_PER_PIXEL(arv_buffer_get_image_pixel_format(buffer))<<" format:"<<arv_buffer_get_image_pixel_format(buffer)<<"\n";
        if (imgfor::ocv_type_get(arv_buffer_get_image_pixel_format(buffer)).ccc!=(cv::ColorConversionCodes)-1) {std::cerr<<"converted color\n"; cvtColor(*freeMat, *freeMat, imgfor::ocv_type_get(arv_buffer_get_image_pixel_format(buffer)).ccc);}   //color conversion if img is not monochrome or bgr
        camm->FQsPCcam.enqueueMat(freeMat, arv_buffer_get_timestamp(buffer));
        camm->pushFrameIntoStream();

        camm->resetTimeout();
    }
    else std::cerr<<"WARNING: this shouldnt happen, see camobj::new_frame_ready\n";
}
void camobj::start(){
    arv_camera_set_acquisition_mode(cam, ARV_ACQUISITION_MODE_CONTINUOUS, NULL);
    arv_camera_get_sensor_size(cam,&Xsize,&Ysize, NULL);
    arv_camera_set_region(cam,0,0,Xsize,Ysize, NULL);
    payload=arv_camera_get_payload(cam, NULL);
    format=arv_camera_get_pixel_format_as_string(cam, NULL);
    set_trigger("none");

    std::cerr<<"payload="<<payload<<"\n";
    std::cerr<<"Xsize="<<Xsize<<"\n";
    std::cerr<<"Ysize="<<Ysize<<"\n";
    std::cerr<<"format="<<format<<"\n";

    arv_camera_set_exposure_time(cam,expo.get(), NULL);
    expo.set(arv_camera_get_exposure_time(cam, NULL));
    std::cerr<<"exposure(us)="<<expo.get()<<"\n";

    double fmin,fmax;
    arv_camera_set_boolean(cam, "AcquisitionFrameRateEnable", true, NULL);  // these settings were made to work with Basler cameras, and may be problematic for other genicam cameras
    get_frame_rate_bounds(&fmin, &fmax);
    ackFPS=0;
    arv_camera_set_float(cam, "AcquisitionFrameRate", ackFPS, NULL);
    arv_camera_start_acquisition(cam, NULL);

    std::cerr<<"maxFPS="<<fmax<<"\n";
    std::cerr<<"minFPS="<<fmin<<"\n";

    for (int i=0;i!=FRAMEBUFFER_INITIAL_SIZE;i++) pushFrameIntoStream();
    resetTimeout();
}
void camobj::pushFrameIntoStream(){
    if(!FreeBuffers.empty()){
        arv_stream_push_buffer(str, FreeBuffers.front());
        FreeBuffers.pop();
    }
    else{
        ArvBuffer *buffer = arv_buffer_new(payload, NULL);
        arv_stream_push_buffer(str, buffer);
    }
}
void camobj::requeueFrame(cv::Mat* MatPtr){
    for(int i=0;i!=FullBuffers.size();i++){
        if(arv_buffer_get_data(FullBuffers[i], NULL)==MatPtr->data){
            FreeBuffers.push(FullBuffers[i]);
            FullBuffers.erase(FullBuffers.begin()+i);
            return;
        }
    }
    std::cerr<<"WARNING: this shouldnt happen, see camobj::requeueFrame\n";
}

void camobj::work(){
    if(control_lost) end();
    else if(checkID && !cobj->MVM_list){
        if(selected_ID.get()!=ID){      //we selected a different camera or try to connect to the same one if not connected
            if (ID!="none") end();
            if (selected_ID.get()!="none") con_cam();  //connects to the new cam selected_ID
            else ID="none";
        }
        checkID=false;
    }else if(_connected){
        /*work part start*/
        double maxReqFPS=FQsPCcam.isThereInterest();
        if(maxReqFPS!=ackFPS){
            ackFPS=maxReqFPS;
            arv_camera_set_float(cam, "AcquisitionFrameRate", ackFPS, NULL);
            actualAckFPS=arv_camera_get_float(cam, "ResultingFrameRate", NULL);
            FQsPCcam.setActualFps(actualAckFPS);
            if(ackFPS!=0){
                std::cerr<<"Set new camera FPS: "<<ackFPS<<"\n";
                std::cerr<<"Actual resulting camera FPS: "<<actualAckFPS<<"\n";
            }else{
                std::cerr<<"No more interest, stopping camera ack.\n";
            }
            resetTimeout();
        }

        if(actualAckFPS!=0 && trigMode==false){     // check for camera timeout
            unsigned delayms;
            {std::lock_guard<std::mutex>lock(lastFrameTimeMux);
            std::chrono::time_point<std::chrono::system_clock> now=std::chrono::system_clock::now();
            delayms=std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime).count();}
            if(delayms>cameraTimeout){
                arv_camera_execute_command(cam,"DeviceReset",NULL);
                std::cerr<<"Camera timeout! Camera has been reset.\n";
            }
        }
        /*work part end*/
    }
    else if(FQsPCcam.isThereInterest()){
        checkID=true;
    }
}
void camobj::end(){
    _connected=false;
    if(str!=NULL) arv_stream_set_emit_signals (str, FALSE);
    if(cam!=NULL) arv_camera_stop_acquisition(cam, NULL);
    if(str!=NULL) g_clear_object(&str);
    if(cam!=NULL) g_clear_object(&cam);
    if(dev!=NULL) dev=NULL;   //no unref here because unrefing cam seams to unref dev too, but it doesnt set it to NULL
    ID="none";
    while(!FreeBuffers.empty()){
        g_clear_object(&FreeBuffers.front());
        FreeBuffers.pop();
    }
    if(control_lost) cobj->MVM_list=true;
    control_lost=false;
    GUI_icon->setPix(pixmaps::px_offline);
}

void camobj::control_lost_cb (ArvDevice *ldev){
    for(int i=0;i!=cobj->_CAM_NUM;i++)
        if(cobj->_c[i].ptr->dev==ldev){
            std::cout<<"Ctrl lost of "<<cobj->_c[i].ptr->ID<<"\n";
            cobj->_c[i].ptr->control_lost=true;
            break;
        }
}
void camobj::con_cam(){
    _connected=false;
    ID=selected_ID.get();
    cam=arv_camera_new(ID.c_str(), NULL);
    if(cam==NULL)
        {end(); return;}
    dev=arv_camera_get_device(cam);
    str=arv_camera_create_stream(cam, NULL, NULL, NULL);
    if(dev==NULL || str==NULL)
        {end(); return;}
    g_signal_connect(dev, "control-lost", G_CALLBACK(control_lost_cb), this);
    g_signal_connect(str, "new-buffer", G_CALLBACK(new_frame_ready), this);
    arv_stream_set_emit_signals (str, TRUE);
    start();
    _connected=true;
    GUI_icon->setPix(pixmaps::px_online);
}



void camobj::set(std::string atr, bool nvar){
    std::lock_guard<std::mutex>lock(mtx);
    arv_device_set_boolean_feature_value(dev, atr.c_str(), nvar, NULL);
}
void camobj::set(std::string atr, long int nvar){
    std::lock_guard<std::mutex>lock(mtx);
    arv_device_set_integer_feature_value(dev, atr.c_str(), nvar, NULL);
}
void camobj::set(std::string atr, double nvar){
    std::lock_guard<std::mutex>lock(mtx);
    arv_device_set_float_feature_value(dev, atr.c_str(), nvar, NULL);
}
void camobj::set(std::string atr, std::string nvar){
    std::lock_guard<std::mutex>lock(mtx);
    arv_device_set_string_feature_value(dev, atr.c_str(), nvar.c_str(), NULL);
}
bool camobj::get_bool(std::string atr){
    std::lock_guard<std::mutex>lock(mtx);
    return arv_device_get_boolean_feature_value(dev, atr.c_str(), NULL);
}
long int camobj::get_lint(std::string atr){
    std::lock_guard<std::mutex>lock(mtx);
    return arv_device_get_integer_feature_value(dev, atr.c_str(), NULL);
}
double camobj::get_dbl(std::string atr){
    std::lock_guard<std::mutex>lock(mtx);
    return arv_device_get_float_feature_value(dev, atr.c_str(), NULL);
}
std::string camobj::get_str(std::string atr){
    std::lock_guard<std::mutex>lock(mtx);
    return arv_device_get_string_feature_value(dev, atr.c_str(), NULL);
}
void camobj::run(std::string atr){
    std::lock_guard<std::mutex>lock(mtx);
    arv_device_execute_command(dev, atr.c_str(), NULL);
}

void camobj::set_trigger(std::string trig){
    resetTimeout();
    std::lock_guard<std::mutex>lock(mtx);
    if(trig=="none") {
        arv_camera_set_string(cam, "TriggerMode", "Off", NULL);
        trigMode=false;
    }
    else {
        arv_camera_set_trigger(cam, trig.c_str(), NULL);    //Typical values for source are "Line1" or "Line2". See the camera documentation for the allowed values. Activation is set to rising edge. It can be changed by accessing the underlying device object.
        arv_camera_set_string(cam, "TriggerMode", "On", NULL);
        trigMode=true;
    }
}

void camobj::resetTimeout(){
    std::lock_guard<std::mutex>lock(lastFrameTimeMux);
    lastFrameTime=std::chrono::system_clock::now();
}

void camobj::get_frame_rate_bounds (double *min, double *max){
    std::lock_guard<std::mutex>lock(mtx);
    arv_camera_get_frame_rate_bounds(cam, min, max, NULL);
    arv_camera_set_float(cam, "AcquisitionFrameRate", *max, NULL);
    double tmpMax=arv_camera_get_float(cam, "ResultingFrameRate", NULL);
    arv_camera_set_float(cam, "AcquisitionFrameRate", ackFPS, NULL);
    if(tmpMax<*max) *max=tmpMax;
}
