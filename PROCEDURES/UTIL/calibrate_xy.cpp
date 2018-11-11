#include "calibrate_xy.h"
#include "includes.h"
#include <opencv2/phase_unwrapping.hpp>
#include <chrono>

pCalibrateXY::pCalibrateXY(double testDis): testDis(testDis){
}
pCalibrateXY::~pCalibrateXY(){
}

void pCalibrateXY::run(){
    if(!go.pMAKO->iuScope->connected || !go.pXPS->connected) return;
    framequeue=go.pMAKO->iuScope->FQsPCcam.getNewFQ();

    go.pXPS->setGPIO(XPS::iuScopeLED,true);
    cv::Mat matA, matB, matC;

    framequeue->setUserFps(999);
    mat=nullptr;
    while(mat==nullptr) {mat=framequeue->getUserMat(); std::this_thread::sleep_for (std::chrono::milliseconds(1));}
    mat->copyTo(matA);
    framequeue->freeUserMat();
    framequeue->setUserFps(0);

    go.pXPS->MoveRelative(XPS::mgroup_XYZ, testDis,0,0);        //TODO implement return value like in exec command, so that we can wait till it completes
    std::this_thread::sleep_for (std::chrono::seconds(1));          //should be enough

    framequeue->setUserFps(999);
    mat=nullptr;
    while(mat==nullptr) {mat=framequeue->getUserMat(); std::this_thread::sleep_for (std::chrono::milliseconds(1));}
    mat->copyTo(matB);
    framequeue->freeUserMat();
    framequeue->setUserFps(0);

    go.pXPS->MoveRelative(XPS::mgroup_XYZ, 0,testDis,0);
    std::this_thread::sleep_for (std::chrono::seconds(1));

    framequeue->setUserFps(999);
    mat=nullptr;
    while(mat==nullptr) {mat=framequeue->getUserMat(); std::this_thread::sleep_for (std::chrono::milliseconds(1));}
    mat->copyTo(matC);
    framequeue->freeUserMat();
    framequeue->setUserFps(0);

    //now we process images:

    cv::Ptr<cv::Feature2D> orb = cv::ORB::create(MAX_FEATURES);
    std::vector<cv::KeyPoint> keypoints_A, keypoints_B, keypoints_C;
    cv::Mat descriptors_A, descriptors_B, descriptors_C;

    orb->detectAndCompute(matA, cv::Mat(), keypoints_A, descriptors_A );
    orb->detectAndCompute(matB, cv::Mat(), keypoints_B, descriptors_B );
    orb->detectAndCompute(matC, cv::Mat(), keypoints_C, descriptors_C );


    std::vector<cv::DMatch> matchesAB, matchesBC;
    cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create("BruteForce-Hamming");
    matcher->match(descriptors_A, descriptors_B, matchesAB, cv::Mat());
    matcher->match(descriptors_B, descriptors_C, matchesBC, cv::Mat());

    std::sort(matchesAB.begin(), matchesAB.end());
    std::sort(matchesBC.begin(), matchesBC.end());

    cv::Mat imMatchesAB, imMatchesBC;
    cv::drawMatches(matA, keypoints_A, matB, keypoints_B, matchesAB, imMatchesAB);
    cv::drawMatches(matB, keypoints_B, matC, keypoints_C, matchesBC, imMatchesBC);
    imwrite("imMatchesAB.jpg", imMatchesAB);
    imwrite("imMatchesBC.jpg", imMatchesBC);

    std::vector<cv::Point2f> pointsA_AB, pointsB_AB, pointsB_BC, pointsC_BC;

    for(size_t i=0; i!=matchesAB.size(); i++){
        pointsA_AB.push_back(keypoints_A[matchesAB[i].queryIdx].pt);
        pointsB_AB.push_back(keypoints_B[matchesAB[i].trainIdx].pt);
    }
    for(size_t i=0; i!=matchesBC.size(); i++){
        pointsB_BC.push_back(keypoints_B[matchesBC[i].queryIdx].pt);
        pointsC_BC.push_back(keypoints_C[matchesBC[i].trainIdx].pt);
    }

    cv::Mat AB_trans=cv::findHomography(pointsA_AB, pointsB_AB, cv::RANSAC);
    cv::Mat BC_trans=cv::findHomography(pointsB_BC, pointsC_BC, cv::RANSAC);

    std::cout<<"Matrix AB:\n";
    for(int i=0; i!= AB_trans.rows; i++){
        for(int j=0; j!= AB_trans.cols; j++) std::cout<<AB_trans.at<double>(i,j)<<", ";
        std::cout<<"\n";
    }
    std::cout<<"Matrix BC:\n";
    for(int i=0; i!= BC_trans.rows; i++){
        for(int j=0; j!= BC_trans.cols; j++) std::cout<<BC_trans.at<double>(i,j)<<", ";
        std::cout<<"\n";
    }

    go.pMAKO->iuScope->FQsPCcam.deleteFQ(framequeue);   //cleanup
    done=true;
    end=true;
}
