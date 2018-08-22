#ifndef VMBWRAP_H
#define VMBWRAP_H

#include <VimbaCPP/Include/VimbaCPP.h>
#include "includes.h"


namespace wfun{                 //to make the damn vimba api features simpler to use
    template <typename T>
    VmbErrorType set(AVT::VmbAPI::CameraPtr ptr, char* atr, T nvar){
        AVT::VmbAPI::FeaturePtr fet;
        ptr->GetFeatureByName(atr,fet);     //see page 28 of C++ Vimba manual for possible features
        return fet->SetValue(nvar);
    }
    template <typename T>
    T get(AVT::VmbAPI::CameraPtr ptr, char* atr){
        AVT::VmbAPI::FeaturePtr fet;
        T ret;
        ptr->GetFeatureByName(atr,fet);
        fet->GetValue(ret);
        return ret;
    }
    inline VmbErrorType run(AVT::VmbAPI::CameraPtr ptr, char* atr){
        AVT::VmbAPI::FeaturePtr fet;
        ptr->GetFeatureByName(atr,fet);
        return fet->RunCommand();
    }
}


struct format_conv{            //to convert vimba formats to standard opencv mat
    VmbPixelFormatType vmb_type;
    int ocv_type;
    std::string vmb_name;
    std::string ocv_name;
};





#endif // VMBWRAP_H
