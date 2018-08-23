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


static struct img_form{            //to convert vimba formats to standard opencv mat, these are by hand and may be incorrect, I commented out formats are not something I could be bothered to implement
    VmbPixelFormatType vmb_type;        //VimbaImageTransform/VmbCommonTypes.h
    int ocv_type;
    std::string vmb_name;
    cv::ColorConversionCodes ccc;   //opencv uses the bgr scheme, so others have to be converted to this, if the code is -1, do not convert  (https://docs.opencv.org/3.1.0/d7/d1b/group__imgproc__misc.html)
} format_conv[] = {
{VmbPixelFormatMono8,CV_8U,"VmbPixelFormatMono8",(cv::ColorConversionCodes)-1},  // Monochrome, 8 bits (PFNC:Mono8)
{VmbPixelFormatMono10,CV_16U,"VmbPixelFormatMono10",(cv::ColorConversionCodes)-1},  // Monochrome, 10 bits in 16 bits (PFNC:Mono10)
{VmbPixelFormatMono10p,CV_16U,"VmbPixelFormatMono10p",(cv::ColorConversionCodes)-1},  // Monochrome, 10 bits in 16 bits (PFNC:Mono10p)
{VmbPixelFormatMono12,CV_16U,"VmbPixelFormatMono12",(cv::ColorConversionCodes)-1},  // Monochrome, 12 bits in 16 bits (PFNC:Mono12)
//{VmbPixelFormatMono12Packed,,"VmbPixelFormatMono12Packed",},  // Monochrome, 2x12 bits in 24 bits (GEV:Mono12Packed)
//{VmbPixelFormatMono12p,,"VmbPixelFormatMono12p",},  // Monochrome, 2x12 bits in 24 bits (PFNC:MonoPacked)
{VmbPixelFormatMono14,CV_16U,"VmbPixelFormatMono14",(cv::ColorConversionCodes)-1},  // Monochrome, 14 bits in 16 bits (PFNC:Mono14)
{VmbPixelFormatMono16,CV_16U,"VmbPixelFormatMono16",(cv::ColorConversionCodes)-1},  // Monochrome, 16 bits (PFNC:Mono16)
{VmbPixelFormatBayerGR8,CV_8UC2,"VmbPixelFormatBayerGR8",cv::COLOR_BayerGR2BGR},  // Bayer-color, 8 bits, starting with GR line (PFNC:BayerGR8)
{VmbPixelFormatBayerRG8,CV_8UC2,"VmbPixelFormatBayerRG8",cv::COLOR_BayerRG2BGR},  // Bayer-color, 8 bits, starting with RG line (PFNC:BayerRG8)
{VmbPixelFormatBayerGB8,CV_8UC2,"VmbPixelFormatBayerGB8",cv::COLOR_BayerGB2BGR},  // Bayer-color, 8 bits, starting with GB line (PFNC:BayerGB8)
{VmbPixelFormatBayerBG8,CV_8UC2,"VmbPixelFormatBayerBG8",cv::COLOR_BayerBG2BGR},  // Bayer-color, 8 bits, starting with BG line (PFNC:BayerBG8)
{VmbPixelFormatBayerGR10,CV_16UC2,"VmbPixelFormatBayerGR10",cv::COLOR_BayerGR2BGR},  // Bayer-color, 10 bits in 16 bits, starting with GR line (PFNC:BayerGR10)
{VmbPixelFormatBayerRG10,CV_16UC2,"VmbPixelFormatBayerRG10",cv::COLOR_BayerRG2BGR},  // Bayer-color, 10 bits in 16 bits, starting with RG line (PFNC:BayerRG10)
{VmbPixelFormatBayerGB10,CV_16UC2,"VmbPixelFormatBayerGB10",cv::COLOR_BayerGB2BGR},  // Bayer-color, 10 bits in 16 bits, starting with GB line (PFNC:BayerGB10)
{VmbPixelFormatBayerBG10,CV_16UC2,"VmbPixelFormatBayerBG10",cv::COLOR_BayerBG2BGR},  // Bayer-color, 10 bits in 16 bits, starting with BG line (PFNC:BayerBG10)
{VmbPixelFormatBayerGR12,CV_16UC2,"VmbPixelFormatBayerGR12",cv::COLOR_BayerGR2BGR},  // Bayer-color, 12 bits in 16 bits, starting with GR line (PFNC:BayerGR12)
{VmbPixelFormatBayerRG12,CV_16UC2,"VmbPixelFormatBayerRG12",cv::COLOR_BayerRG2BGR},  // Bayer-color, 12 bits in 16 bits, starting with RG line (PFNC:BayerRG12)
{VmbPixelFormatBayerGB12,CV_16UC2,"VmbPixelFormatBayerGB12",cv::COLOR_BayerGB2BGR},  // Bayer-color, 12 bits in 16 bits, starting with GB line (PFNC:BayerGB12)
{VmbPixelFormatBayerBG12,CV_16UC2,"VmbPixelFormatBayerBG12",cv::COLOR_BayerBG2BGR},  // Bayer-color, 12 bits in 16 bits, starting with BG line (PFNC:BayerBG12)
//VmbPixelFormatBayerGR12Packed,,"VmbPixelFormatBayerGR12Packed",},  // Bayer-color, 2x12 bits in 24 bits, starting with GR line (GEV:BayerGR12Packed)
//{VmbPixelFormatBayerRG12Packed,,"VmbPixelFormatBayerRG12Packed",},  // Bayer-color, 2x12 bits in 24 bits, starting with RG line (GEV:BayerRG12Packed)
//{VmbPixelFormatBayerGB12Packed,,"VmbPixelFormatBayerGB12Packed",},  // Bayer-color, 2x12 bits in 24 bits, starting with GB line (GEV:BayerGB12Packed)
//{VmbPixelFormatBayerBG12Packed,,"VmbPixelFormatBayerBG12Packed",},  // Bayer-color, 2x12 bits in 24 bits, starting with BG line (GEV:BayerBG12Packed)
//{VmbPixelFormatBayerGR10p,1,"VmbPixelFormatBayerGR10p",},  // Bayer-color, 12 bits continuous packed, starting with GR line (PFNC:BayerGR10p)
//{VmbPixelFormatBayerRG10p,1,"VmbPixelFormatBayerRG10p",},  // Bayer-color, 12 bits continuous packed, starting with RG line (PFNC:BayerRG10p)
//{VmbPixelFormatBayerGB10p,1,"VmbPixelFormatBayerGB10p",},  // Bayer-color, 12 bits continuous packed, starting with GB line (PFNC:BayerGB10p)
//{VmbPixelFormatBayerBG10p,1,"VmbPixelFormatBayerBG10p",},  // Bayer-color, 12 bits continuous packed, starting with BG line (PFNC:BayerBG10p)
//{VmbPixelFormatBayerGR12p,1,"VmbPixelFormatBayerGR12p",},  // Bayer-color, 12 bits continuous packed, starting with GR line (PFNC:BayerGR12p)
//{VmbPixelFormatBayerRG12p,1,"VmbPixelFormatBayerRG12p",},  // Bayer-color, 12 bits continuous packed, starting with RG line (PFNC:BayerRG12p)
//{VmbPixelFormatBayerGB12p,1,"VmbPixelFormatBayerGB12p",},  // Bayer-color, 12 bits continuous packed, starting with GB line (PFNC:BayerGB12p)
//{VmbPixelFormatBayerBG12p,1,"VmbPixelFormatBayerBG12p",},  // Bayer-color, 12 bits continuous packed, starting with BG line (PFNC:BayerBG12p)
{VmbPixelFormatBayerGR16,CV_16UC2,"VmbPixelFormatBayerGR16",cv::COLOR_BayerGR2BGR},  // Bayer-color, 16 bits, starting with GR line (PFNC:BayerGR16)
{VmbPixelFormatBayerRG16,CV_16UC2,"VmbPixelFormatBayerRG16",cv::COLOR_BayerRG2BGR},  // Bayer-color, 16 bits, starting with RG line (PFNC:BayerRG16)
{VmbPixelFormatBayerGB16,CV_16UC2,"VmbPixelFormatBayerGB16",cv::COLOR_BayerGB2BGR},  // Bayer-color, 16 bits, starting with GB line (PFNC:BayerGB16)
{VmbPixelFormatBayerBG16,CV_16UC2,"VmbPixelFormatBayerBG16",cv::COLOR_BayerBG2BGR},  // Bayer-color, 16 bits, starting with BG line (PFNC:BayerBG16)
{VmbPixelFormatRgb8,CV_8UC3,"VmbPixelFormatRgb8",cv::COLOR_RGB2BGR},  // RGB, 8 bits x 3 (PFNC:RGB8)
{VmbPixelFormatBgr8,CV_8UC3,"VmbPixelFormatBgr8",(cv::ColorConversionCodes)-1},  // BGR, 8 bits x 3 (PFNC:BGR8)
//{VmbPixelFormatArgb8,CV_8UC4,"VmbPixelFormatArgb8",},  // ARGB, 8 bits x 4 (PFNC:RGBa8)
{VmbPixelFormatRgba8,CV_8UC4,"VmbPixelFormatRgba8",cv::COLOR_RGBA2BGRA},   // RGBA, 8 bits x 4, legacy name
{VmbPixelFormatBgra8,CV_8UC4,"VmbPixelFormatBgra8",(cv::ColorConversionCodes)-1},  // BGRA, 8 bits x 4 (PFNC:BGRa8)
{VmbPixelFormatRgb12,CV_16UC3,"VmbPixelFormatRgb12",cv::COLOR_RGB2BGR},  // RGB, 12 bits in 16 bits x 3 (PFNC:RGB12)
{VmbPixelFormatRgb16,CV_16UC3,"VmbPixelFormatRgb16",cv::COLOR_RGB2BGR}  // RGB, 16 bits x 3 (PFNC:RGB16)
//{VmbPixelFormatYuv411,,"VmbPixelFormatYuv411",},  // YUV 411 with 8 bits (GEV:YUV411Packed)
//{VmbPixelFormatYuv422,,"VmbPixelFormatYuv422",},  // YUV 422 with 8 bits (GEV:YUV422Packed)
//{VmbPixelFormatYuv444,,"VmbPixelFormatYuv444",},  // YUV 444 with 8 bits (GEV:YUV444Packed)
//{VmbPixelFormatYCbCr411_8_CbYYCrYY,1,"VmbPixelFormatYCbCr411_8_CbYYCrYY",},  // Y´CbCr 411 with 8 bits (PFNC:YCbCr411_8_CbYYCrYY) - identical to VmbPixelFormatYuv411
//{VmbPixelFormatYCbCr422_8_CbYCrY,1,"VmbPixelFormatYCbCr422_8_CbYCrY",},  // Y´CbCr 422 with 8 bits (PFNC:YCbCr422_8_CbYCrY) - identical to VmbPixelFormatYuv422
//{VmbPixelFormatYCbCr8_CbYCr,1,"VmbPixelFormatYCbCr8_CbYCr",} // Y´CbCr 444 with 8 bits (PFNC:YCbCr8_CbYCr) - identic
};


#endif // VMBWRAP_H
