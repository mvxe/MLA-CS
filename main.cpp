#include "globals.h"
#include "opencv2/opencv.hpp"
#include "opencv2/core/ocl.hpp"

#include "UTIL/pipe.h"
#include <iostream>

int main(int argc, char *argv[]){
    if (!cv::ocl::haveOpenCL())
    {
        std::cout << "OpenCL is not available..." << std::endl;
        //return 0;
    }

    cv::ocl::Context context;
    if (!context.create(cv::ocl::Device::TYPE_GPU))
    {
        std::cout << "Failed creating the OpenCL context..." << std::endl;
        //return 0;
    } else{
        std::cout << context.ndevices() << " GPU devices are detected." << std::endl; //This bit provides an overview of the OpenCL devices you have in your computer
        for (int i = 0; i < context.ndevices(); i++)
        {
            cv::ocl::Device device = context.device(i);
            std::cout << "name:              " << device.name() << std::endl;
            std::cout << "available:         " << device.available() << std::endl;
            std::cout << "imageSupport:      " << device.imageSupport() << std::endl;
            std::cout << "OpenCL_C_Version:  " << device.OpenCL_C_Version() << std::endl;
            std::cout << "clock:              " << device.maxClockFrequency() << std::endl;
            std::cout << "maxComputeUnits:              " << device.maxComputeUnits() << std::endl;
             std::cout << "maxWorkGroupSize:              " << device.maxWorkGroupSize() << std::endl;
            std::cout << std::endl;
        }
        cv::ocl::Device(context.device(0));
    }

    go.startup(argc, argv);
}
