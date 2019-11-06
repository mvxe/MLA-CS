#include "colorbar.h"
#include "GUI/gui_includes.h"

colorBar::colorBar(smp_selector* cm_sel): cm_sel(cm_sel){
    this->setMouseTracking(false);
    this->setScaledContents(false);
    this->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}

void colorBar::show(double min, double max, int vsize){
    cv::Mat mat(vsize, 50, CV_8U, cv::Scalar(0));
    cv::Mat amat(vsize, 1, CV_8U);
    for(int i=0;i!=vsize;i++) amat.at<uchar>(i)=i*255/vsize;
    cv::applyColorMap(amat, amat, OCV_CM::ids[cm_sel->index]);
    cv::repeat(amat, 1, 50, mat);

    double range=max-min;

    cv::putText(mat,"- 0 nm", cv::Point(0,0), cv::FONT_HERSHEY_DUPLEX, 0.3, cv::Vec3b(255,255,255)-amat.at<cv::Vec3b>(0), 1, cv::LINE_AA);
    cv::putText(mat,util::toString("- ",range/2," nm"), cv::Point(0,(vsize-1)/2), cv::FONT_HERSHEY_DUPLEX, 0.3, cv::Vec3b(255,255,255)-amat.at<cv::Vec3b>((vsize-1)/2), 1, cv::LINE_AA);
    cv::putText(mat,util::toString("- ",range," nm"), cv::Point(0,vsize-1), cv::FONT_HERSHEY_DUPLEX, 0.3, cv::Vec3b(255,255,255)-amat.at<cv::Vec3b>(vsize-1), 1, cv::LINE_AA);

    QImage qimg(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
    std::move(qimg).rgbSwapped();
    this->setPixmap(QPixmap::fromImage(qimg));
    this->QLabel::show();
}
