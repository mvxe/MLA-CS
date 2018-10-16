#include "get_depth_map.h"
#include "includes.h"
#include <opencv2/phase_unwrapping.hpp>
pGetDepthMap::pGetDepthMap(double range, double offset, double speed, unsigned char threshold): range(range), offset(offset), speed(speed), threshold(threshold), addOfs(speed){
}
pGetDepthMap::~pGetDepthMap(){
}
void pGetDepthMap::run(){
    if(!go.pMAKO->iuScope->connected || !go.pXPS->connected) return;
    framequeue=go.pMAKO->iuScope->FQsPCcam.getNewFQ();

    go.pXPS->setGPIO(XPS::iuScopeLED,false);

    po = go.pXPS->createNewPVTobj(XPS::mgroup_XYZ, "pGetDepthMap.txt");
    po->add(1,          0,0,0,0,-range/2-addOfs+offset  ,0);
    po->add(1,          0,0,0,0,addOfs                  ,speed);                    //same as po->addAction("GPIO3.DO.DOSet",1,0,0,0);
    po->addAction(XPS::iuScopeLED,true);
    po->add(range/speed,0,0,0,0,range                   ,speed);
    po->addAction(XPS::iuScopeLED,false);                                           //same as po->addAction("GPIO3.DO.DOSet",1,1,0,0);
    po->add(1,          0,0,0,0,addOfs                  ,0);
    po->add(1,          0,0,0,0,-range/2-addOfs-offset  ,0);
    if(go.pXPS->verifyPVTobj(po).retval!=0) return;                                 //this will block and exec after MoveAbsolute is done
    go.pXPS->execPVTobj(po, &ret);                                                  //we dont block here, gotta start processing frames

    framequeue->setUserFps(99999);                      //start acquisiton, set max fps

    while (!ret.check_if_done()) while (NMat+1<framequeue->getFullNumber()){       //in this block we discard frames below threshold, NMat is the final number of frames
        mat=framequeue->getUserMat(NMat);
        if(mat!=nullptr){
            if(mat->rows!=1024 || mat->cols!=1280) {std::cerr<<mat->rows<<" "<<mat->cols<<" bad rows or cols, Nmat="<<NMat<<" ,Full="<<framequeue->getFullNumber()<<"\n";std::this_thread::sleep_for (std::chrono::milliseconds(100));} //TODO remove this, this was for debugging purposes
            else if(cv::mean(*mat)[0]<threshold){
                framequeue->freeUserMat(NMat);
            }
            else NMat++;
        }
        else std::this_thread::sleep_for (std::chrono::milliseconds(10));
    }
    NMat++;

    framequeue->setUserFps(0);                          //end acquisition

    if(NMat<=20){   //just in case samething went wrong
        std::cout<<"shiet"<<NMat<<"\n";
        goto cleanup;
    }

    for (int i=0;i!=3;i++){
        framequeue->freeUserMat(NMat);      //the threshold!=normal LED level so we remove the edge frames
        framequeue->freeUserMat(0);
        NMat-=2;
    }

    //single();
    multiple();

cleanup:
    go.pXPS->setGPIO(XPS::iuScopeLED,true);
    go.pMAKO->iuScope->FQsPCcam.deleteFQ(framequeue);   //cleanup
    done=true;
    end=true;
    std::cout<<"toal mats: "<<go.pMAKO->iuScope->FQsPCcam.getFullNumber()<<"+"<<go.pMAKO->iuScope->FQsPCcam.getFreeNumber()<<" Nmat:"<<NMat<<"\n";
}

void pGetDepthMap::multiple(){
    mat=framequeue->getUserMat(0);
    cv::Mat mat2D(mat->rows, NMat, CV_32F, cv::Scalar::all(0));
    cv::Mat fft2D;
    cv::Mat mat2DDepth(mat->rows, mat->cols, CV_32F, cv::Scalar::all(0));
    for(int k=0;k!=mat->cols;k++){
        for(int i=0;i!=NMat;i++){
            mat=framequeue->getUserMat(i);
            for(int j=0;j!=mat->rows;j++)
                mat2D.at<float>(j,i)=mat->at<unsigned char>(j,k); //TODO perhaps this can be optimized by opencv functions
        }
        cv::dft(mat2D, fft2D, cv::DFT_COMPLEX_OUTPUT+cv::DFT_ROWS);
        if(end) return;
        std::cerr<<"done with dft"<<k<<"\n";
        for(int j=0;j!=mat->rows;j++){
            float max=0;
            int maxLoc=0;
            for(int i=NMat/50;i!=NMat/4;i++){
                if(std::abs(fft2D.at<std::complex<float>>(j, i)) > max){
                    max=std::abs(fft2D.at<std::complex<float>>(j, i));
                    maxLoc=i;
                }
            }
            mat2DDepth.at<float>(j,k)=std::arg(fft2D.at<std::complex<float>>(j, maxLoc));
        }
    }

    cv::Mat wrapped;
    mat2DDepth.convertTo(wrapped, CV_16U, (1<<16)/M_PI/2, (1<<16)/2);
    imwrite( "depthImage-wrapped.png", wrapped );

    cv::Mat unwrapped;
    cv::phase_unwrapping::HistogramPhaseUnwrapping::Params params;
    //mat2DDepth.convertTo(wrapped, CV_32FC1, 0.5/M_PI);
    mat2DDepth.convertTo(wrapped, CV_32FC1);
    params.width = wrapped.cols;
    params.height = wrapped.rows;
    cv::Ptr<cv::phase_unwrapping::HistogramPhaseUnwrapping> phaseUnwrapping = cv::phase_unwrapping::HistogramPhaseUnwrapping::create(params);
    phaseUnwrapping->unwrapPhaseMap(wrapped, unwrapped);
    unwrapped.convertTo(unwrapped, CV_16U, (1<<16)/M_PI/2, (1<<16)/2);
    imwrite( "depthImage-unwrapped.png", unwrapped );

  std::ofstream ofile;
  ofile.open ("MEASUREMENT2.dat",std::ofstream::trunc);
    for(int i=0;i!=NMat;i++){
        ofile<<i<<" "<<mat2D.at<float>(500, i)<<" "<<std::abs(fft2D.at<std::complex<float>>(500, i))<<" "<<std::arg(fft2D.at<std::complex<float>>(500, i))<<"\n";
    }
  ofile.close();

}

void pGetDepthMap::single(){
    mat=framequeue->getUserMat(0);
    cv::Mat mat1D(1, NMat, CV_32F, cv::Scalar::all(0));
    cv::Mat fft1D;
    for(int i=0;i!=NMat;i++){
        mat=framequeue->getUserMat(i);
        mat1D.at<float>(i)=mat->at<unsigned char>(500,500);
    }
    cv::dft(mat1D, fft1D, cv::DFT_COMPLEX_OUTPUT);

  std::ofstream ofile;
  ofile.open ("MEASUREMENT.dat",std::ofstream::trunc);
    for(int i=0;i!=NMat;i++){
        ofile<<i<<" "<<mat1D.at<float>(i)<<" "<<std::abs(fft1D.at<std::complex<float>>(i))<<" "<<std::arg(fft1D.at<std::complex<float>>(i))<<"\n";
    }
  ofile.close();
}

/*To run on GPU: change
 * cv::Mat --> cv::UMat (all arguments to functions)
 * cv::dft(mat1D, fft1D, cv::DFT_COMPLEX_OUTPUT);
 * mat1D.at<float>(i)  --> mat1D.getMat(cv::ACCESS_READ).at<float>(i)
 *
 * also mat copying (not useful since weve different types
 *  mat->col(k).copyTo(mat2D.col(i));
 *  mat2D.convertTo(mat2D, CV_32F);
 */
