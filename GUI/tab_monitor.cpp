#include "tab_monitor.h"
#include "mainwindow.h"
#include "DEV/RPTY/rpty.h"

tab_monitor::tab_monitor(QWidget *parent){
    layout=new QHBoxLayout;
    parent->setLayout(layout);

    graphWidget=new QWidget;
    graphLayout=new QVBoxLayout;
    graphWidget->setLayout(graphLayout);
    layout->addWidget(graphWidget);

        WFgraph=new QChart;
        WFgraphView=new QChartView(WFgraph);
        graphLayout->addWidget(WFgraphView);
        WFgraphView->setRenderHint(QPainter::Antialiasing);
        WFgraph->setTitle("Waveform");
        WFchAseries=new QLineSeries();
        WFchAseries->setName("CHA");
        WFchBseries=new QLineSeries();
        WFchBseries->setName("CHB");
        //WFgraph->setAnimationOptions(QChart::AllAnimations);
        WFgraph->addSeries(WFchAseries);
        WFgraph->addSeries(WFchBseries);
        WFgraph->createDefaultAxes();
        WFgraph->axisY()->setRange(-300,-100);
        WFgraph->axisX()->setRange(0,1000);


        FTgraph=new QChart;
        FTgraphView=new QChartView(FTgraph);
        graphLayout->addWidget(FTgraphView);
        FTgraphView->setRenderHint(QPainter::Antialiasing);
        FTgraph->setTitle("FFT");

        SGgraph=new QChart;
        SGgraphView=new QChartView(SGgraph);
        graphLayout->addWidget(SGgraphView);
        SGgraphView->hide();



    buttonWidget=new QWidget;
    buttonLayout=new QVBoxLayout;
    buttonWidget->setLayout(buttonLayout);
    layout->addWidget(buttonWidget);

        channelSelect=new QToolButton;
        buttonLayout->addWidget(channelSelect);
        channelSelect->setAutoRaise(true);
        channelSelect->setToolButtonStyle(Qt::ToolButtonTextOnly);
        channelSelect->setPopupMode(QToolButton::InstantPopup);
        channelSelect->setText("Select Channels");
        channelSelect->setMenu(new QMenu);
        for(int i=0;i!=BN_channel;i++){
            _sTB_channel[i].action=new QAction;
            _sTB_channel[i].action->setText(_sTB_channel[i].text.c_str());
            channelSelect->menu()->addAction(_sTB_channel[i].action);
        }
        connect(channelSelect, SIGNAL(triggered(QAction *)), this, SLOT(on_channel_select(QAction *)));

        averagingSelect=new QToolButton;
        buttonLayout->addWidget(averagingSelect);
        averagingSelect->setAutoRaise(true);
        averagingSelect->setToolButtonStyle(Qt::ToolButtonTextOnly);
        averagingSelect->setPopupMode(QToolButton::InstantPopup);
        averagingSelect->setText("Select Averaging (Acq. Frequency)");
        averagingSelect->setMenu(new QMenu);
        for(int i=0;i<=maxavg;i++){
            _avg_actionList[i]=new QAction;
            _avg_actionList[i]->setText(util::toString("Averaging ",(long)1<<i," samples (",1./baseSamplFreq/((long)1<<i),"Hz)").c_str());
            averagingSelect->menu()->addAction(_avg_actionList[i]);
        }
        connect(averagingSelect, SIGNAL(triggered(QAction *)), this, SLOT(on_averaging_select(QAction *)));

        QHBoxLayout* SBlayout=new QHBoxLayout;
        QWidget* SBwidget=new QWidget;
        SBwidget->setLayout(SBlayout);
        buttonLayout->addWidget(SBwidget);
            QLabel* SBtxt0=new QLabel;
            SBtxt0->setText("Acquisition frequency: ");
            SBlayout->addWidget(SBtxt0);
            frequencySpinbox=new QDoubleSpinBox;
            frequencySpinbox->setValue(acqfreq);
            frequencySpinbox->setDecimals(6);
            frequencySpinbox->setRange(1e-6,1e6);
            SBlayout->addWidget(frequencySpinbox);
            connect(frequencySpinbox, SIGNAL(editingFinished()), this, SLOT(on_freqValueChanged()));
            QLabel* SBtxt1=new QLabel;
            SBtxt1->setText("Hz");
            SBlayout->addWidget(SBtxt1);


        CBtrig=new QCheckBox;
        buttonLayout->addWidget(CBtrig);
        CBtrig->setText("Trigger on laser pulse");
        connect(CBtrig, SIGNAL(stateChanged(int)), this, SLOT(on_CBtrig_stateChanged(int)));

        QLabel* infotxt=new QLabel;
        infotxt->setText("NOTE: The following options will keep acquisition\nrunning in background!");
        buttonLayout->addWidget(infotxt);

        CBsaveWF=new QCheckBox;
        buttonLayout->addWidget(CBsaveWF);
        CBsaveWF->setText("Save Waveform");
        CBsaveWF->setDisabled(true);
        connect(CBsaveWF, SIGNAL(stateChanged(int)), this, SLOT(on_CBsaveWF_stateChanged(int)));

        CBsaveFT=new QCheckBox;
        buttonLayout->addWidget(CBsaveFT);
        CBsaveFT->setText("Save FFT");
        CBsaveFT->setDisabled(true);
        connect(CBsaveFT, SIGNAL(stateChanged(int)), this, SLOT(on_CBsaveFT_stateChanged(int)));

        CBspectrogram=new QCheckBox;
        buttonLayout->addWidget(CBspectrogram);
        CBspectrogram->setText("Show spectrogram");
        CBspectrogram->setDisabled(true);
        connect(CBspectrogram, SIGNAL(stateChanged(int)), this, SLOT(on_CBspectrogram_stateChanged(int)));

        buttonLayout->addStretch(0);

}


void tab_monitor::on_channel_select(QAction *action){
    for(int i=0;i!=BN_channel;i++) if(action==_sTB_channel[i].action){
        selectedchannels=_sTB_channel[i].channels;
        break;
    }
    printf ("%s\n", action->text().toStdString().c_str());
    channelSelect->setText(action->text());
    rdy_ch=true; try_unblock();
}
void tab_monitor::on_averaging_select(QAction *action){
    for(int i=0;i<=maxavg;i++) if(action==_avg_actionList[i]){
        selectedavg=i;
        break;
    }
    averagingSelect->setText(action->text());
    printf ("avging: %d\n", selectedavg);
    rdy_avg=true; try_unblock();
}

void tab_monitor::on_CBtrig_stateChanged(int state){
    trig_on_laser_pulse=(state==Qt::Checked);
}
void tab_monitor::on_CBsaveWF_stateChanged(int state){
    if(state==Qt::Checked){
        //TODO save filename
    }
    save_waveform=(state==Qt::Checked);
}
void tab_monitor::on_CBsaveFT_stateChanged(int state){
    if(state==Qt::Checked){
        //TODO save filename
    }
    save_fft=(state==Qt::Checked);
}
void tab_monitor::on_CBspectrogram_stateChanged(int state){
    if(state==Qt::Checked){

    }
    calc_spec=(state==Qt::Checked);
}

void tab_monitor::on_freqValueChanged(){
    acqfreq=frequencySpinbox->value();
}


void tab_monitor::try_unblock(){
    if(!rdy_ack && rdy_ch && rdy_avg){
        rdy_ack=true;
        CBsaveWF->setDisabled(false);
        CBsaveFT->setDisabled(false);
        CBspectrogram->setDisabled(false);
        init_timer();
    }
}

void tab_monitor::init_timer(){     //once the tab is visited, the timer interrupt stays on, TODO fix if it becomes a problem
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(work_fun()));
    timer->start(work_call_time);
}

void tab_monitor::work_fun(){
    //printf("lol monitor\n");
    if(tab_is_open || save_waveform || save_fft || calc_spec){

        //printf("its open or some cb is on\n");
        if(go.pRPTY->connected){
            //printf("RPTY is connected\n");

                printf("A2F_RSMax for queue %d is %d\n",0,go.pRPTY->getNum(RPTY::A2F_RSMax,0));
                printf("A2F_RSCur for queue %d is %d\n",0,go.pRPTY->getNum(RPTY::A2F_RSCur,0));
                printf("A2F_lostN for queue %d is %d\n",0,go.pRPTY->getNum(RPTY::A2F_lostN,0));
                printf("F2A_RSMax for queue %d is %d\n",0,go.pRPTY->getNum(RPTY::F2A_RSMax,0));
                printf("F2A_RSCur for queue %d is %d\n",0,go.pRPTY->getNum(RPTY::F2A_RSCur,0));
                printf("F2A_lostN for queue %d is %d\n",0,go.pRPTY->getNum(RPTY::F2A_lostN,0));

            uint32_t acksize=go.pRPTY->getNum(RPTY::F2A_RSMax,0)/2;
                //uint32_t acksize=500;
            //printf("acksize=%u\n",acksize);

            if(go.pRPTY->getNum(RPTY::A2F_RSCur,0)==0){
                std::vector<uint32_t> commands;
                commands.push_back(CQF::W4TRIG_INTR());
                commands.push_back(CQF::ACK(1<<0, selectedavg, selectedchannels, true));
                commands.push_back(CQF::WAIT(acksize*(1<<selectedavg)));
                commands.push_back(CQF::ACK(1<<0, selectedavg, selectedchannels, false));
                go.pRPTY->A2F_write(0,commands.data(),commands.size());
            }
            go.pRPTY->trig(1<<0);

            if(!firstRead){
                uint32_t toread=go.pRPTY->getNum(RPTY::F2A_RSCur,0);
                std::vector<uint32_t> read;
                read.reserve(toread);
                go.pRPTY->F2A_read(0,read.data(),toread);
//                WFgraph->removeSeries(WFchAseries);
//                WFgraph->removeSeries(WFchBseries);

                QList<QPointF> points;
                points.reserve(1000);
                for(int i=0; i!=1000; i++) points.push_back(QPointF(i,AQF::getChMSB(read[i])));
                WFchAseries->clear();
                WFchAseries->append(points);
                points.clear();
                for(int i=0; i!=1000; i++) points.push_back(QPointF(i,AQF::getChLSB(read[i])));
                WFchBseries->clear();
                WFchBseries->append(points);
//                WFgraph->addSeries(WFchAseries);
//                WFgraph->addSeries(WFchBseries);
            }
            else firstRead=false;
        }
    }
}
