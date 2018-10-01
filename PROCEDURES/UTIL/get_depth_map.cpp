#include "get_depth_map.h"
#include "includes.h"

pGetDepthMap::pGetDepthMap(double range, double speed, unsigned char threshold): range(range), speed(speed), threshold(threshold), addOfs(speed){
}
pGetDepthMap::~pGetDepthMap(){
}
void pGetDepthMap::run(){
    if(!go.pMAKO->iuScope->connected || !go.pXPS->connected) return;
    framequeue=go.pMAKO->iuScope->FQsPCcam.getNewFQ();

    go.pXPS->setGPIO(XPS::iuScopeLED,false);

    po = go.pXPS->createNewPVTobj(XPS::mgroup_XYZ, "pGetDepthMap.txt");
    po->add(1,          0,0,0,0,-range/2-addOfs  ,0);
    po->add(1,          0,0,0,0,addOfs           ,speed);                           //same as po->addAction("GPIO3.DO.DOSet",1,0,0,0);
    po->addAction(XPS::iuScopeLED,true);
    po->add(range/speed,0,0,0,0,range            ,speed);
    po->addAction(XPS::iuScopeLED,false);                                           //same as po->addAction("GPIO3.DO.DOSet",1,1,0,0);
    po->add(1,          0,0,0,0,addOfs           ,0);
    po->add(1,          0,0,0,0,-range/2-addOfs  ,0);
    if(go.pXPS->verifyPVTobj(po).retval!=0) return;                                 //this will block and exec after MoveAbsolute is done
    go.pXPS->execPVTobj(po, &ret);                                                  //we dont block here, gotta start processing frames

    framequeue->setUserFps(99999);                      //start acquisiton, set max fps

    while (!ret.check_if_done()) while (NMat+1<framequeue->getFullNumber()){       //in this block we discard frames below threshold, NMat is the final number of frames
        mat=framequeue->getUserMat(NMat);
        if(mat!=nullptr){
            if(mat->rows!=1024 || mat->cols!=1280) std::cerr<<mat->rows<<" "<<mat->cols<<" bad rows or cols\n";
            if(cv::mean(*mat)[0]<threshold){
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

    framequeue->freeUserMat(NMat);      //the threshold!=normal LED level so we remove the edge frames
    framequeue->freeUserMat(0);
    NMat-=2;

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
    int sizes[] = { NMat, mat->rows, mat->cols };
  std::cerr<<"allocating matrix\n";
    cv::Mat mat3D(3, sizes, CV_32F, cv::Scalar::all(0));
    cv::Mat fft3D;
  std::cerr<<"starting to assign matrix\n";
    for(int i=0;i!=NMat;i++){
        mat=framequeue->getUserMat(i);
        for(int j=0;j!=mat->rows;j++)
            for(int k=0;k!=mat->cols;k++)
                mat3D.at<float>(i,j,k)=mat->at<unsigned char>(j,k); //TODO perhaps this can be optimized by opencv functions
    }
  std::cerr<<"done assigning matrix\n";
    cv::dft(mat3D, fft3D, cv::DFT_COMPLEX_OUTPUT+cv::DFT_ROWS);
  std::cerr<<"done with dft\n";
  std::ofstream ofile;
  ofile.open ("MEASUREMENT2.dat",std::ofstream::trunc);
    for(int i=0;i!=NMat;i++){
        ofile<<i<<" "<<mat3D.at<float>(i,500,500)<<" "<<std::abs(fft3D.at<std::complex<float>>(i,500,500))<<" "<<std::arg(fft3D.at<std::complex<float>>(i,500,500))<<"\n";
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
