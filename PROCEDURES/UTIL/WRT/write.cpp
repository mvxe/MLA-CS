#include "write.h"
#include "GUI/gui_includes.h"
#include "includes.h"
#include "GUI/tab_monitor.h"    //for debug purposes

pgWrite::pgWrite(){
    gui_activation=new QWidget;
    gui_settings=new QWidget;

    alayout=new QVBoxLayout;
    slayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);
    gui_settings->setLayout(slayout);

    pulse=new QPushButton("Pulse");
    connect(pulse, SIGNAL(released()), this, SLOT(onPulse()));
    pulseInt=new val_selector(1000, "pgWrite_pulseInt", "Int:", 1, 8192, 0);
    pulseDur=new val_selector(1, "pgWrite_pulseDur", "Dur", 0.001, 1000, 3, 0, {"ms"});
    alayout->addWidget(new twid(pulse, pulseInt, pulseDur));

}

void pgWrite::onPulse(){
    if(!go.pRPTY->connected) return;
    std::vector<uint32_t> commands;
    commands.push_back(CQF::W4TRIG_INTR());
    commands.push_back(CQF::TRIG_OTHER(1<<tab_monitor::RPTY_A2F_queue));
    commands.push_back(CQF::SG_SAMPLE(CQF::O0td, pulseInt->val, 0));
    commands.push_back(CQF::WAIT(pulseDur->val/8e-6-1));
    commands.push_back(CQF::SG_SAMPLE(CQF::O0td, 0, 0));
    go.pRPTY->A2F_write(0,commands.data(),commands.size());
    go.pRPTY->trig(1<<0);
}
