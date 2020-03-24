////NOTE: this is not used anymore! left as code example


//#include "calibrate_xy.h"
//#include "includes.h"
//#include <opencv2/phase_unwrapping.hpp>
//#include <chrono>

//pCalibrateXY::pCalibrateXY(double testDis, double *xps_x_sen, double *xps_y_sen): testDis(testDis), xps_x_sen(xps_x_sen), xps_y_sen(xps_y_sen){
//}
//pCalibrateXY::~pCalibrateXY(){
//}

//void pCalibrateXY::run(){
//    if(!go.pGCAM->iuScope->connected || !go.pXPS->connected) return;
//    framequeue=go.pGCAM->iuScope->FQsPCcam.getNewFQ();

//    go.pXPS->setGPIO(XPS::iuScopeLED,true);
//    cv::Mat matA, matB, matC;

//    while(framequeue->getFullNumber()) framequeue->freeUserMat();
//    framequeue->setUserFps(999);
//    mat=nullptr;
//    while(mat==nullptr) {mat=framequeue->getUserMat(); std::this_thread::sleep_for (std::chrono::milliseconds(1));}
//    mat->copyTo(matA);
//    framequeue->setUserFps(0);

//    go.pXPS->MoveRelative(XPS::mgroup_XYZF, testDis,0,0,0);        //TODO implement return value like in exec command, so that we can wait till it completes
//    std::this_thread::sleep_for (std::chrono::seconds(1));          //should be enough

//    while(framequeue->getFullNumber()) framequeue->freeUserMat();
//    framequeue->setUserFps(999);
//    mat=nullptr;
//    while(mat==nullptr) {mat=framequeue->getUserMat(); std::this_thread::sleep_for (std::chrono::milliseconds(1));}
//    mat->copyTo(matB);
//    framequeue->setUserFps(0);

//    go.pXPS->MoveRelative(XPS::mgroup_XYZF, 0,testDis,0,0);
//    std::this_thread::sleep_for (std::chrono::seconds(1));

//    while(framequeue->getFullNumber()) framequeue->freeUserMat();
//    framequeue->setUserFps(999);
//    mat=nullptr;
//    while(mat==nullptr) {mat=framequeue->getUserMat(); std::this_thread::sleep_for (std::chrono::milliseconds(1));}
//    mat->copyTo(matC);
//    framequeue->setUserFps(0);

//    go.pXPS->MoveRelative(XPS::mgroup_XYZF, -testDis,-testDis,0,0);

//    //now we process images:

//    cv::Ptr<cv::Feature2D> orb = cv::ORB::create(MAX_FEATURES);
//    std::vector<cv::KeyPoint> keypoints_A, keypoints_B, keypoints_C;
//    cv::Mat descriptors_A, descriptors_B, descriptors_C;

//    orb->detectAndCompute(matA, cv::Mat(), keypoints_A, descriptors_A );
//    orb->detectAndCompute(matB, cv::Mat(), keypoints_B, descriptors_B );
//    orb->detectAndCompute(matC, cv::Mat(), keypoints_C, descriptors_C );


//    std::vector<cv::DMatch> matchesAB, matchesBC;
//    cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create("BruteForce-Hamming(2)");
//    matcher->match(descriptors_A, descriptors_B, matchesAB, cv::Mat());
//    matcher->match(descriptors_B, descriptors_C, matchesBC, cv::Mat());

//    for(size_t i=0; i!=matchesAB.size(); i++){
//        if(matchesAB[i].distance>minDis) {matchesAB.erase(matchesAB.begin()+i); i--;}
//    }
//    for(size_t i=0; i!=matchesBC.size(); i++){
//        if(matchesBC[i].distance>minDis) {matchesBC.erase(matchesBC.begin()+i); i--;}
//    }

//    std::sort(matchesAB.begin(), matchesAB.end());
//    std::sort(matchesBC.begin(), matchesBC.end());

//    cv::Mat imMatchesAB, imMatchesBC;
//    cv::drawMatches(matA, keypoints_A, matB, keypoints_B, matchesAB, imMatchesAB);
//    cv::drawMatches(matB, keypoints_B, matC, keypoints_C, matchesBC, imMatchesBC);
//    imwrite("imMatchesAB.jpg", imMatchesAB);
//    imwrite("imMatchesBC.jpg", imMatchesBC);

//    std::vector<cv::Point2f> pointsA_AB, pointsB_AB, pointsB_BC, pointsC_BC;


//    for(size_t i=0; i!=matchesAB.size(); i++){
//        pointsA_AB.push_back(keypoints_A[matchesAB[i].queryIdx].pt);
//        pointsB_AB.push_back(keypoints_B[matchesAB[i].trainIdx].pt);
//    }
//    for(size_t i=0; i!=matchesBC.size(); i++){
//        pointsB_BC.push_back(keypoints_B[matchesBC[i].queryIdx].pt);
//        pointsC_BC.push_back(keypoints_C[matchesBC[i].trainIdx].pt);
//    }

//    cv::Mat AB_trans=cv::findHomography(pointsA_AB, pointsB_AB, cv::RANSAC);
//    cv::Mat BC_trans=cv::findHomography(pointsB_BC, pointsC_BC, cv::RANSAC);

//    std::cout<<"Matrix AB, matches="<<matchesAB.size()<<":\n";
//    for(int i=0; i!= AB_trans.rows; i++){
//        for(int j=0; j!= AB_trans.cols; j++) std::cout<<AB_trans.at<double>(i,j)<<", ";
//        std::cout<<"\n";
//    }
//    std::cout<<"Matrix BC, matches="<<matchesBC.size()<<":\n";
//    for(int i=0; i!= BC_trans.rows; i++){
//        for(int j=0; j!= BC_trans.cols; j++) std::cout<<BC_trans.at<double>(i,j)<<", ";
//        std::cout<<"\n";
//    }

//    double calibX, calibY;
//    calibX=testDis/AB_trans.at<double>(0,2);
//    calibY=testDis/BC_trans.at<double>(1,2);

//    std::cout<<"Approximate calibration (disregarding stageXY to cameraXY misalignment): calibX="<<calibX<<" mm/px, calibY="<<calibY<<" mm/px\n";
//    std::cout<<"Total imaging area size: "<<calibX*mat->cols<<"mm x "<<calibY*mat->rows<<"mm\n";

//    *xps_x_sen=calibX*100000;
//    *xps_y_sen=calibY*100000;

//    go.pGCAM->iuScope->FQsPCcam.deleteFQ(framequeue);   //cleanup
//    done=true;
//    end=true;
//}
