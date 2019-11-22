#include "deptheval.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgDepthEval::pgDepthEval(varShareClient<pgScanGUI::scanRes>* src, varShareClient<pgScanGUI::scanRes>* dst): src(src){
    dst=result.getClient();
    layout=new QVBoxLayout;
    this->setLayout(layout);
}
