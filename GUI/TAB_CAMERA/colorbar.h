#ifndef COLORBAR_H
#define COLORBAR_H
#include <QLabel>
#include "opencv2/opencv.hpp"
class smp_selector;

class colorBar: public QLabel{
    Q_OBJECT
public:
    colorBar(smp_selector* cm_sel);
    void show(double min, double max, int vsize);
private:
    smp_selector* cm_sel;
};

#endif // COLORBAR_H
