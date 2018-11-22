#ifndef VMBWRAP_H
#define VMBWRAP_H

#include <arv.h>
#include <string>
#include "opencv2/opencv.hpp"

struct img_form{                         //to convert vimba formats to standard opencv mat, these are by hand and may be incorrect, I commented out formats are not something I could be bothered to implement
    ArvPixelFormat type;         //VimbaImageTransform/VmbCommonTypes.h
    int ocv_type;
    std::string name;
    cv::ColorConversionCodes ccc;        //opencv uses the bgr scheme, so others have to be converted to this, if the code is -1, do not convert  (https://docs.opencv.org/3.1.0/d7/d1b/group__imgproc__misc.html)
};

namespace imgfor{
    const static img_form convmat[] = {
    {ARV_PIXEL_FORMAT_BAYER_BG_10,                     CV_16UC2 ,"ARV_PIXEL_FORMAT_BAYER_BG_10",cv::COLOR_BayerBG2BGR},
    {ARV_PIXEL_FORMAT_BAYER_BG_12,                     CV_16UC2 ,"ARV_PIXEL_FORMAT_BAYER_BG_12",cv::COLOR_BayerBG2BGR},
//    {ARV_PIXEL_FORMAT_BAYER_GR_12_PACKED,   ,"ARV_PIXEL_FORMAT_BAYER_GR_12_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_BAYER_GB_12_PACKED,   ,"ARV_PIXEL_FORMAT_BAYER_GB_12_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_BAYER_RG_12_PACKED,   ,"ARV_PIXEL_FORMAT_BAYER_RG_12_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_BAYER_BG_12_PACKED,   ,"ARV_PIXEL_FORMAT_BAYER_BG_12_PACKED",(cv::ColorConversionCodes)-1},
    {ARV_PIXEL_FORMAT_BAYER_BG_8,                       CV_8UC2 ,"ARV_PIXEL_FORMAT_BAYER_BG_8",cv::COLOR_BayerBG2BGR},
    {ARV_PIXEL_FORMAT_BAYER_GB_10,                     CV_16UC2 ,"ARV_PIXEL_FORMAT_BAYER_GB_10",cv::COLOR_BayerGB2BGR},
    {ARV_PIXEL_FORMAT_BAYER_GB_12,                     CV_16UC2 ,"ARV_PIXEL_FORMAT_BAYER_GB_12",cv::COLOR_BayerGB2BGR},
    {ARV_PIXEL_FORMAT_BAYER_GB_8,                       CV_8UC2 ,"ARV_PIXEL_FORMAT_BAYER_GB_8",cv::COLOR_BayerGB2BGR},
    {ARV_PIXEL_FORMAT_BAYER_GR_10,                     CV_16UC2 ,"ARV_PIXEL_FORMAT_BAYER_GR_10",cv::COLOR_BayerGR2BGR},
    {ARV_PIXEL_FORMAT_BAYER_GR_12,                     CV_16UC2 ,"ARV_PIXEL_FORMAT_BAYER_GR_12",cv::COLOR_BayerGR2BGR},
    {ARV_PIXEL_FORMAT_BAYER_GR_8,                       CV_8UC2 ,"ARV_PIXEL_FORMAT_BAYER_GR_8",cv::COLOR_BayerGR2BGR},
    {ARV_PIXEL_FORMAT_BAYER_RG_10,                     CV_16UC2 ,"ARV_PIXEL_FORMAT_BAYER_RG_10",cv::COLOR_BayerRG2BGR},
    {ARV_PIXEL_FORMAT_BAYER_RG_12,                     CV_16UC2 ,"ARV_PIXEL_FORMAT_BAYER_RG_12",cv::COLOR_BayerRG2BGR},
    {ARV_PIXEL_FORMAT_BAYER_RG_8,                       CV_8UC2 ,"ARV_PIXEL_FORMAT_BAYER_RG_8",cv::COLOR_BayerRG2BGR},
    {ARV_PIXEL_FORMAT_BAYER_BG_16,                     CV_16UC2 ,"ARV_PIXEL_FORMAT_BAYER_BG_16",cv::COLOR_BayerBG2BGR},
    {ARV_PIXEL_FORMAT_BAYER_GB_16,                     CV_16UC2 ,"ARV_PIXEL_FORMAT_BAYER_GB_16",cv::COLOR_BayerGB2BGR},
    {ARV_PIXEL_FORMAT_BAYER_GR_16,                     CV_16UC2 ,"ARV_PIXEL_FORMAT_BAYER_GR_16",cv::COLOR_BayerGR2BGR},
    {ARV_PIXEL_FORMAT_BAYER_RG_16,                     CV_16UC2 ,"ARV_PIXEL_FORMAT_BAYER_RG_16",cv::COLOR_BayerRG2BGR},
//    {ARV_PIXEL_FORMAT_BGRA_8_PACKED,   ,"ARV_PIXEL_FORMAT_BGRA_8_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_BGR_10_PACKED,   ,"ARV_PIXEL_FORMAT_BGR_10_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_BGR_12_PACKED,   ,"ARV_PIXEL_FORMAT_BGR_12_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_BGR_8_PACKED,   ,"ARV_PIXEL_FORMAT_BGR_8_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_CUSTOM_BAYER_BG_12_PACKED,   ,"ARV_PIXEL_FORMAT_CUSTOM_BAYER_BG_12_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_CUSTOM_BAYER_BG_16, CV_8U ,"ARV_PIXEL_FORMAT_CUSTOM_BAYER_BG_16",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_CUSTOM_BAYER_GB_12_PACKED,   ,"ARV_PIXEL_FORMAT_CUSTOM_BAYER_GB_12_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_CUSTOM_BAYER_GB_16, CV_8U ,"ARV_PIXEL_FORMAT_CUSTOM_BAYER_GB_16",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_CUSTOM_BAYER_GR_12_PACKED,   ,"ARV_PIXEL_FORMAT_CUSTOM_BAYER_GR_12_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_CUSTOM_BAYER_GR_16, CV_8U ,"ARV_PIXEL_FORMAT_CUSTOM_BAYER_GR_16",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_CUSTOM_BAYER_RG_12_PACKED,   ,"ARV_PIXEL_FORMAT_CUSTOM_BAYER_RG_12_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_CUSTOM_BAYER_RG_16, CV_8U ,"ARV_PIXEL_FORMAT_CUSTOM_BAYER_RG_16",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_CUSTOM_YUV_422_YUYV_PACKED,   ,"ARV_PIXEL_FORMAT_CUSTOM_YUV_422_YUYV_PACKED",(cv::ColorConversionCodes)-1},
    {ARV_PIXEL_FORMAT_MONO_10,                           CV_16U ,"ARV_PIXEL_FORMAT_MONO_10",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_MONO_10_PACKED,   ,"ARV_PIXEL_FORMAT_MONO_10_PACKED",(cv::ColorConversionCodes)-1},
    {ARV_PIXEL_FORMAT_MONO_12,                           CV_16U ,"ARV_PIXEL_FORMAT_MONO_12",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_MONO_12_PACKED,   ,"ARV_PIXEL_FORMAT_MONO_12_PACKED",(cv::ColorConversionCodes)-1},
    {ARV_PIXEL_FORMAT_MONO_14,                           CV_16U ,"ARV_PIXEL_FORMAT_MONO_14",(cv::ColorConversionCodes)-1},
    {ARV_PIXEL_FORMAT_MONO_16,                           CV_16U ,"ARV_PIXEL_FORMAT_MONO_16",(cv::ColorConversionCodes)-1},
    {ARV_PIXEL_FORMAT_MONO_8,                             CV_8U ,"ARV_PIXEL_FORMAT_MONO_8",(cv::ColorConversionCodes)-1},
    {ARV_PIXEL_FORMAT_MONO_8_SIGNED,                      CV_8S ,"ARV_PIXEL_FORMAT_MONO_8_SIGNED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_RGBA_8_PACKED,   ,"ARV_PIXEL_FORMAT_RGBA_8_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_RGB_10_PACKED,   ,"ARV_PIXEL_FORMAT_RGB_10_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_RGB_10_PLANAR,   ,"ARV_PIXEL_FORMAT_RGB_10_PLANAR",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_RGB_12_PACKED,   ,"ARV_PIXEL_FORMAT_RGB_12_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_RGB_12_PLANAR,   ,"ARV_PIXEL_FORMAT_RGB_12_PLANAR",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_RGB_16_PLANAR,   ,"ARV_PIXEL_FORMAT_RGB_16_PLANAR",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_RGB_8_PACKED,    ,"ARV_PIXEL_FORMAT_RGB_8_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_RGB_8_PLANAR,   ,"ARV_PIXEL_FORMAT_RGB_8_PLANAR",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_YUV_411_PACKED,   ,"ARV_PIXEL_FORMAT_YUV_411_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_YUV_422_PACKED,   ,"ARV_PIXEL_FORMAT_YUV_422_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_YUV_422_YUYV_PACKED,   ,"ARV_PIXEL_FORMAT_YUV_422_YUYV_PACKED",(cv::ColorConversionCodes)-1},
//    {ARV_PIXEL_FORMAT_YUV_444_PACKED,   ,"ARV_PIXEL_FORMAT_YUV_444_PACKED",(cv::ColorConversionCodes)-1}
    };
    const size_t sizev = std::extent<decltype(convmat)>::value;
    inline img_form ocv_type_get(ArvPixelFormat type){
        for (int i=0;i!=sizev;i++) if(convmat[i].type==type) return convmat[i];
        return {type,-1,"none",(cv::ColorConversionCodes)-1};
    }
}



#endif // VMBWRAP_H
