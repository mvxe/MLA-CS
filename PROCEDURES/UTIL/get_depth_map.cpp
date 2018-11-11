#include "get_depth_map.h"
#include "includes.h"
#include <opencv2/phase_unwrapping.hpp>
#include <chrono>

pGetDepthMap::pGetDepthMap(double range, double offset, double speed, unsigned char threshold, std::string filename): range(range), offset(offset), speed(speed), threshold(threshold), addOfs(speed), filename(filename){
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
    cv::UMat Umat2D;
    cv::UMat Ufft2D;

    int NMat_opt=cv::getOptimalDFTSize(NMat);
    std::cout<<"Mats= "<<NMat<<", optimal="<<NMat_opt<<"\n";

    cv::Mat mat2D(NMat_opt, mat->cols, CV_32F, cv::Scalar::all(0));
    cv::Mat fft2D;
    cv::Mat mat2DDepth(mat->rows, mat->cols, CV_32F, cv::Scalar::all(0));


    std::chrono::time_point<std::chrono::system_clock> A;
    std::chrono::time_point<std::chrono::system_clock> B;
//    std::chrono::time_point<std::chrono::system_clock> C;
//    std::chrono::time_point<std::chrono::system_clock> D;


    int maxLoc=0;


    A=std::chrono::system_clock::now();
    for(int k=0;k!=mat->rows;k++){
//        A=std::chrono::system_clock::now();
        for(int i=0;i!=NMat_opt;i++){
            if(i<NMat) mat=framequeue->getUserMat(i);
            mat->row(k).copyTo(mat2D.row(i));
        }
        cv::transpose(mat2D,mat2D);
//        B=std::chrono::system_clock::now();

        Umat2D=mat2D.getUMat(cv::ACCESS_READ);
        cv::dft(Umat2D, Ufft2D, cv::DFT_COMPLEX_OUTPUT+cv::DFT_ROWS);
            //fft2D=Ufft2D.getMat(cv::ACCESS_READ);     //this wont work as its asynchronous
        Ufft2D.copyTo(fft2D);
//        cv::dft(mat2D, fft2D, cv::DFT_COMPLEX_OUTPUT+cv::DFT_ROWS);
//        C=std::chrono::system_clock::now();



        if(end) return;
        std::cerr<<"done with dft"<<k<<"\n";

        if (maxLoc==0){  //we only do it for first col, no need for more
            float max=0;
            for(int i=NMat_opt/50;i!=NMat_opt/4;i++){
                if(std::abs(fft2D.at<std::complex<float>>(mat->rows/2, i)) > max){
                    max=std::abs(fft2D.at<std::complex<float>>(mat->rows/2, i));
                    maxLoc=i;
                }
            }
            std::cerr<<"max="<<maxLoc<<"\n";
        }

        for(int j=0;j!=mat->cols;j++){
            mat2DDepth.at<float>(k,j)=std::arg(fft2D.at<std::complex<float>>(j, maxLoc));
        }
        mat2D=mat2D.reshape(0,mat2D.cols);    //we dont use transpose here, because we overwrite the data later anyway, and reshape just changes the header
//        D=std::chrono::system_clock::now();
//        std::cout<<"copy mat "<<std::chrono::duration_cast<std::chrono::microseconds>(B - A).count()<<" microseconds\n";
//        std::cout<<"calc dft "<<std::chrono::duration_cast<std::chrono::microseconds>(C - B).count()<<" microseconds\n";
//        std::cout<<"copy dft "<<std::chrono::duration_cast<std::chrono::microseconds>(D - C).count()<<" microseconds\n";

    }

    cv::Mat wrapped;
    mat2DDepth.convertTo(wrapped, CV_16U, (1<<16)/M_PI/2, (1<<16)/2);
    imwrite(filename, wrapped);

    cv::Mat unwrapped;
    cv::phase_unwrapping::HistogramPhaseUnwrapping::Params params;
    //mat2DDepth.convertTo(wrapped, CV_32FC1, 0.5/M_PI);
    mat2DDepth.convertTo(wrapped, CV_32FC1);
    params.width = wrapped.cols;
    params.height = wrapped.rows;
    cv::Ptr<cv::phase_unwrapping::HistogramPhaseUnwrapping> phaseUnwrapping = cv::phase_unwrapping::HistogramPhaseUnwrapping::create(params);
    phaseUnwrapping->unwrapPhaseMap(wrapped, unwrapped);
    unwrapped.convertTo(unwrapped, CV_16U, (1<<16)/M_PI/2, (1<<16)/2);

    B=std::chrono::system_clock::now();
    std::cout<<"total time "<<std::chrono::duration_cast<std::chrono::milliseconds>(B - A).count()<<" milliseconds\n";

    std::size_t found = filename.find(".");
    if (found!=std::string::npos)
        filename.insert(found,"-unwrapped");
    else filename+="-unwrapped";
    imwrite(filename, unwrapped );

  std::ofstream ofile;
  ofile.open ("MEASUREMENT2.dat",std::ofstream::trunc);
    for(int i=0;i!=NMat_opt;i++){
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
