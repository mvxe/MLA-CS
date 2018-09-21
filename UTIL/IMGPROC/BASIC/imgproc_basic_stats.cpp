#include "imgproc_basic_stats.h"
#include "opencv2/opencv.hpp"

double imgproc_basic_stats::get_avg_value(const cv::Mat *mat){
    long double total=0;
    for (int i=0;i!=mat->rows;i++)
       for (int j=0;j!=mat->cols;j++)
           total+=mat->at<unsigned char>(i,j);
    return static_cast<double>(total/mat->rows/mat->cols);
}

double imgproc_basic_stats::get_contrast(const cv::Mat* mat, double avg){
    long double total=0;
    for (int i=0;i!=mat->rows-1;i++)
       for (int j=0;j!=mat->cols-1;j++){
           total+=pow(mat->at<unsigned char>(i,j)-mat->at<unsigned char>(i+1,j),4);
           total+=pow(mat->at<unsigned char>(i,j)-mat->at<unsigned char>(i,j+1),4);
           total+=pow(mat->at<unsigned char>(i,j)-mat->at<unsigned char>(i+1,j+1),4);
       }
    return static_cast<double>(total/mat->rows/mat->cols);
}
