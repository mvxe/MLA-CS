#ifndef IMGPROC_BASIC_STATS_H
#define IMGPROC_BASIC_STATS_H

namespace cv {
    class Mat;
}


namespace imgproc_basic_stats {
    double get_avg_value(const cv::Mat* mat);
}

#endif // IMGPROC_BASIC_STATS_H
