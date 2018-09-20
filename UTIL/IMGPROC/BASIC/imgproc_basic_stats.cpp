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
    for (int i=0;i!=mat->rows;i++)
       for (int j=0;j!=mat->cols;j++)
           total+=pow(avg-mat->at<unsigned char>(i,j),2);
    return static_cast<double>(total/mat->rows/mat->cols);
}
