#include "position_report.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgPosRepGUI::pgPosRepGUI(pgMoveGUI* pgMGUI): pgMGUI(pgMGUI){
    this->setText("Position: NC");
    timer = new QTimer(this);
    timer->setInterval(timer_delay);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
}

void pgPosRepGUI::update(){
    if(go.pRPTY->connected){
        double pos[6];
        if(go.pRPTY->mux.try_lock()){
            pos[0]=go.pRPTY->getMotionSetting("X",CTRL::mst_position);
            pos[1]=go.pRPTY->getMotionSetting("Y",CTRL::mst_position);
            pos[2]=go.pRPTY->getMotionSetting("Z",CTRL::mst_position);
            pgMGUI->getPos(&pos[3],&pos[4],&pos[5]);
            go.pRPTY->mux.unlock();
            if(pos[0]!=old[0] || pos[1]!=old[1] || pos[2]!=old[2]){
                for(int i=0;i!=3;i++) old[i]=pos[i];
                this->setText(QString::fromStdString(util::toString("Stages (raw):\n  X=", std::fixed, std::setprecision(6), pos[0], "mm , Y=", pos[1], "mm\n  Z=", pos[2], "mm\n",
                    "Corrected:\n  X=", std::fixed, std::setprecision(6), pos[1], "mm , Y=", pos[2], "mm\n  Z=", pos[3], "mm"
                )));
                Q_EMIT changed(old[0], old[1], old[2]);
            }
        }
    }else this->setText(QString::fromStdString(util::toString("Position: NC")));
}

