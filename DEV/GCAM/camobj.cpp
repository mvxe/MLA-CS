#include "DEV/GCAM/gcam.h"

camobj::camobj(std::string strID) : camobj_config(strID), selected_ID(&mkmx,strID,&go.config.save,strID){}
GCAM *camobj::cobj;

void camobj::new_frame_ready (ArvStream *stream, camobj* camm)
{
    ArvBuffer *buffer;
    buffer = arv_stream_try_pop_buffer (stream);
    if(buffer != NULL){
        camm->Xsize=arv_buffer_get_image_width(buffer);
        camm->Ysize=arv_buffer_get_image_height(buffer);
        cv::Mat* freeMat = camm->FQsPCcam.getAFreeMatPtr();
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
        if(dsize!=freeMat->total()*freeMat->elemSize()) std::cerr<<"ERROR: expected frame size does not match returned size ("<<dsize<<" vs "<<freeMat->size<<"), see camobj::new_frame_ready\n";

        camm->FullBuffers.push_back(buffer);

        if (imgfor::ocv_type_get(arv_buffer_get_image_pixel_format(buffer)).ccc!=(cv::ColorConversionCodes)-1) {std::cerr<<"converted color\n"; cvtColor(*freeMat, *freeMat, imgfor::ocv_type_get(arv_buffer_get_image_pixel_format(buffer)).ccc);}   //color conversion if img is not monochrome or bgr
        camm->FQsPCcam.enqueueMat(freeMat, arv_buffer_get_timestamp(buffer));
        camm->pushFrameIntoStream();
    }
    else std::cerr<<"WARNING: this shouldnt happen, see camobj::new_frame_ready\n";
}
void camobj::start(){

        /* TODO: remove these two: they are a workaround in the Aravis library */
    int32_t valx=1024;
    arv_device_write_memory (dev,0x000000000002002c,4,&valx,NULL);  //SI_maximum_trailer_size
    arv_device_write_memory (dev,0x0000000000020018,4,&valx,NULL);  //SI_maximum_leader_size
        /*                                                                     */

    payload=arv_camera_get_payload(cam);
    arv_camera_set_acquisition_mode(cam, ARV_ACQUISITION_MODE_CONTINUOUS);
    arv_camera_get_sensor_size(cam,&Xsize,&Ysize);
    arv_camera_set_region(cam,0,0,Xsize,Ysize);
    format=arv_camera_get_pixel_format_as_string(cam);

    std::cerr<<"payload="<<payload<<"\n";
    std::cerr<<"Xsize="<<Xsize<<"\n";
    std::cerr<<"Ysize="<<Ysize<<"\n";
    std::cerr<<"format="<<format<<"\n";

    arv_camera_set_exposure_time(cam,expo.get());
    expo.set(arv_camera_get_exposure_time(cam));
    std::cerr<<"exposure(us)="<<expo.get()<<"\n";

    double fmin,fmax;
    arv_camera_get_frame_rate_bounds (cam, &fmin, &fmax);
    arv_camera_set_frame_rate(cam, fmax);
    ackFPS=fmax;
    FQsPCcam.setCamFPS(ackFPS);
    std::cerr<<"ackFPS="<<ackFPS<<"\n";

    for (int i=0;i!=FRAMEBUFFER_INITIAL_SIZE;i++) pushFrameIntoStream();
}
void camobj::pushFrameIntoStream(){
    if(!FreeBuffers.empty()){
        arv_stream_push_buffer(str, FreeBuffers.front());
        FreeBuffers.pop();
    }
    else{
        ArvBuffer *buffer = arv_buffer_new(payload, NULL);
        arv_stream_push_buffer(str, buffer);
        FullBuffers.push_back(buffer);
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
    if(checkID && !cobj->MVM_list){
        if(selected_ID.get()!=ID){      //we selected a different camera or try to connect to the same one if not connected
            if (ID!="none") end();
            if (selected_ID.get()!="none") con_cam();  //connects to the new cam selected_ID
            else ID="none";
        }
        checkID=false;
    }

    if(_connected){
            /*work part start*/
        if(FQsPCcam.isThereInterest()){
            if(!ackstatus){
                arv_camera_start_acquisition(cam);
                ackstatus=true;
            }
        }else{
            if(ackstatus){
                arv_camera_stop_acquisition(cam);
                ackstatus=false;
            }
        }
        /*work part end*/
    }

}
void camobj::end(){
    _connected=false;
    control_lost=false;
    if(str!=NULL) arv_stream_set_emit_signals (str, FALSE);
    if(cam!=NULL) arv_camera_stop_acquisition(cam);
    if(str!=NULL) g_clear_object(&str);
    if(cam!=NULL) g_clear_object(&cam);
    if(dev!=NULL) dev=NULL;   //no unref here because unrefing cam seams to unref dev too, but it doesnt set it to NULL
    ID="none";
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
    cam=arv_camera_new(ID.c_str());
    if(cam==NULL)
        {end(); return;}
    dev=arv_camera_get_device(cam);
    str=arv_camera_create_stream(cam, NULL, NULL);
    if(dev==NULL || str==NULL)
        {end(); return;}
    g_signal_connect(dev, "control-lost", G_CALLBACK(control_lost_cb), this);
    g_signal_connect(str, "new-buffer", G_CALLBACK(new_frame_ready), this);
    arv_stream_set_emit_signals (str, TRUE);
    start();
    _connected=true;
}



ArvDeviceStatus camobj::set(std::string atr, bool nvar){
    std::lock_guard<std::mutex>lock(mtx);
    arv_device_set_boolean_feature_value(dev, atr.c_str(), nvar);
    return arv_device_get_status(dev);
}
ArvDeviceStatus camobj::set(std::string atr, long int nvar){
    std::lock_guard<std::mutex>lock(mtx);
    arv_device_set_integer_feature_value(dev, atr.c_str(), nvar);
    return arv_device_get_status(dev);
}
ArvDeviceStatus camobj::set(std::string atr, double nvar){
    std::lock_guard<std::mutex>lock(mtx);
    arv_device_set_float_feature_value(dev, atr.c_str(), nvar);
    return arv_device_get_status(dev);
}
ArvDeviceStatus camobj::set(std::string atr, std::string nvar){
    std::lock_guard<std::mutex>lock(mtx);
    arv_device_set_string_feature_value(dev, atr.c_str(), nvar.c_str());
    return arv_device_get_status(dev);
}
bool camobj::get_bool(std::string atr){
    std::lock_guard<std::mutex>lock(mtx);
    return arv_device_get_boolean_feature_value(dev, atr.c_str());
}
long int camobj::get_lint(std::string atr){
    std::lock_guard<std::mutex>lock(mtx);
    return arv_device_get_integer_feature_value(dev, atr.c_str());
}
double camobj::get_dbl(std::string atr){
    std::lock_guard<std::mutex>lock(mtx);
    return arv_device_get_float_feature_value(dev, atr.c_str());
}
std::string camobj::get_str(std::string atr){
    std::lock_guard<std::mutex>lock(mtx);
    return arv_device_get_string_feature_value(dev, atr.c_str());
}
inline ArvDeviceStatus camobj::run(std::string atr){
    std::lock_guard<std::mutex>lock(mtx);
    arv_device_execute_command(dev, atr.c_str());
    return arv_device_get_status(dev);
}
