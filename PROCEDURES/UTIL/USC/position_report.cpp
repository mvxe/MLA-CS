#include "position_report.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgPosRepGUI::pgPosRepGUI(){
    this->setText("Position: NC");
    timer = new QTimer(this);
    timer->setInterval(timer_delay);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
}

void pgPosRepGUI::update(){
    if(go.pXPS->connected){ if(go.pXPS->isQueueEmpty()){
        XPS::raxis ret = go.pXPS->getPos(XPS::mgroup_XYZF);
        if(ret.pos[0]!=old[0] || ret.pos[1]!=old[1] || ret.pos[2]!=old[2] || ret.pos[3]!=old[3]){
            for(int i=0;i!=4;i++) old[i]=ret.pos[i];
            this->setText(QString::fromStdString(util::toString("X=", std::fixed, std::setprecision(6), ret.pos[0], "mm , Y=", ret.pos[1], "mm\nZ=", ret.pos[2], "mm , F=", ret.pos[3], "mm")));
            Q_EMIT changed(old[0], old[1], old[2], old[3]);
        }
    }}
    else this->setText(QString::fromStdString(util::toString("Position: NC")));
}

