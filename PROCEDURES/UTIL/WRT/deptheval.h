#ifndef DEPTHEVAL_H
#define DEPTHEVAL_H

#include "PROCEDURES/procedure.h"
#include "GUI/gui_aux_objects.h"
#include "PROCEDURES/UTIL/USC/scan.h"

class pgDepthEval: public QWidget{
    Q_OBJECT
public:
    pgDepthEval(varShareClient<pgScanGUI::scanRes>* src, varShareClient<pgScanGUI::scanRes>* dst);
private:
    varShare<pgScanGUI::scanRes> result;
    varShareClient<pgScanGUI::scanRes>* src;
    QVBoxLayout* layout;
};

#endif // DEPTHEVAL_H
