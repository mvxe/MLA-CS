#include "tab_monitor.h"
#include "mainwindow.h"
#include "DEV/RPTY/rpty.h"

#include <viennacl/scalar.hpp>
#include <viennacl/vector.hpp>
#include <viennacl/matrix.hpp>
#include <viennacl/fft.hpp>
#include <complex>

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

        WFaxisX = new QValueAxis();
        WFaxisX->setTitleText("Time (ms)"); WFaxisX->setMinorTickCount(10); WFaxisX->setTickCount(10);
        WFaxisY = new QValueAxis();
        WFaxisY->setTitleText("Values");
        WFgraph->addAxis(WFaxisX, Qt::AlignBottom);
        WFgraph->addAxis(WFaxisY, Qt::AlignLeft);
        WFgraph->addSeries(WFchAseries);
        WFgraph->addSeries(WFchBseries);
        WFchAseries->attachAxis(WFaxisX);
        WFchBseries->attachAxis(WFaxisX);
        WFchAseries->attachAxis(WFaxisY);
        WFchBseries->attachAxis(WFaxisY);

        FTgraph=new QChart;
        FTgraphView=new QChartView(FTgraph);
        graphLayout->addWidget(FTgraphView);
        FTgraphView->setRenderHint(QPainter::Antialiasing);
        FTgraph->setTitle("FFT");
        FTchAseries=new QLineSeries();
        FTchAseries->setName("CHA");
        FTchBseries=new QLineSeries();
        FTchBseries->setName("CHB");

        FTaxisX = new QValueAxis();
        FTaxisX->setTitleText("Frequency (kHz)");   FTaxisX->setMinorTickCount(10); FTaxisX->setTickCount(10);
        FTaxisY = new QLogValueAxis();
        FTaxisY->setBase(10.0);
        FTaxisY->setTitleText("Values");
        FTaxisY->setLabelFormat("%g");
        FTaxisY->setRange(1,1e8);
        FTgraph->addAxis(FTaxisX, Qt::AlignBottom);
        FTgraph->addAxis(FTaxisY, Qt::AlignLeft);
        FTgraph->addSeries(FTchAseries);
        FTgraph->addSeries(FTchBseries);
        FTchAseries->attachAxis(FTaxisX);
        FTchBseries->attachAxis(FTaxisX);
        FTchAseries->attachAxis(FTaxisY);
        FTchBseries->attachAxis(FTaxisY);

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
    int min=9999,max=-9999;
    double scale=baseSamplFreq*(1<<selectedavg)*1e3; //in ms
    if(tab_is_open){// || save_waveform || save_fft || calc_spec){

        //printf("its open or some cb is on\n");
        if(go.pRPTY->connected){
            //printf("RPTY is connected\n");

            printf("A2F_RSMax for queue %d is %d\n",RPTY_A2F_queue,go.pRPTY->getNum(RPTY::A2F_RSMax,RPTY_A2F_queue));
            printf("A2F_RSCur for queue %d is %d\n",RPTY_A2F_queue,go.pRPTY->getNum(RPTY::A2F_RSCur,RPTY_A2F_queue));
            printf("A2F_lostN for queue %d is %d\n",RPTY_A2F_queue,go.pRPTY->getNum(RPTY::A2F_lostN,RPTY_A2F_queue));
            printf("F2A_RSMax for queue %d is %d\n",RPTY_F2A_queue,go.pRPTY->getNum(RPTY::F2A_RSMax,RPTY_F2A_queue));
            printf("F2A_RSCur for queue %d is %d\n",RPTY_F2A_queue,go.pRPTY->getNum(RPTY::F2A_RSCur,RPTY_F2A_queue));
            printf("F2A_lostN for queue %d is %d\n",RPTY_F2A_queue,go.pRPTY->getNum(RPTY::F2A_lostN,RPTY_F2A_queue));

            uint32_t acksize=go.pRPTY->getNum(RPTY::F2A_RSMax,RPTY_F2A_queue)+1;
            uint32_t toread=go.pRPTY->getNum(RPTY::F2A_RSCur,RPTY_F2A_queue);

            if(toread>=acksize){
                std::vector<uint32_t> read;
                read.reserve(toread);
                go.pRPTY->F2A_read(RPTY_F2A_queue,read.data(),toread);
                WFgraph->removeSeries(WFchAseries);
                WFgraph->removeSeries(WFchBseries);
                QList<QPointF> points;
                points.reserve(toread);
                for(int i=0; i!=toread; i++){
                    points.push_back(QPointF(i*scale,AQF::getChMSB(read[i])));
                    if(AQF::getChMSB(read[i])>max) max=AQF::getChMSB(read[i]);
                    else if(AQF::getChMSB(read[i])<min) min=AQF::getChMSB(read[i]);
                }
                WFchAseries->clear();
                WFchAseries->append(points);
                points.clear();
                for(int i=0; i!=toread; i++){
                    points.push_back(QPointF(i*scale,AQF::getChLSB(read[i])));
                    if(AQF::getChLSB(read[i])>max) max=AQF::getChLSB(read[i]);
                    else if(AQF::getChLSB(read[i])<min) min=AQF::getChLSB(read[i]);
                }
                WFchBseries->clear();
                WFchBseries->append(points);
                WFgraph->addSeries(WFchAseries);
                WFgraph->addSeries(WFchBseries);
                WFaxisX->setRange(0,toread*scale);
                WFaxisY->setRange(min,max);
                WFchAseries->attachAxis(WFaxisX);
                WFchBseries->attachAxis(WFaxisX);
                WFchAseries->attachAxis(WFaxisY);
                WFchBseries->attachAxis(WFaxisY);

                printf("toread=%u\n",toread);
                for(int j=0, otoread=toread;j!=100;j++){
                    toread=1<<j;
                    if(toread>otoread) break;
                }
                toread=toread>>1;
                printf("FFTead=%u\n",toread);

                viennacl::vector<float> A(toread);
                for(int i=0; i!=toread; i++) A[i]=AQF::getChMSB(read[i]);
                viennacl::vector<float> B(2*toread);
                viennacl::vector<float> C(2*toread);
                viennacl::linalg::real_to_complex(A, B, A.size());
                viennacl::fft(B, C);
                FTgraph->removeSeries(FTchAseries);
                FTgraph->removeSeries(FTchBseries);
                points.clear();
                for(int i=0; i!=toread/2; i++) {
                    std::complex<float> cn(C(2*i),C(2*i+1));
                    points.push_back(QPointF(i/scale/toread,std::abs(cn)));
                }
                FTchAseries->clear();
                FTchAseries->append(points);
                points.clear();
                for(int i=0; i!=toread; i++) A[i]=AQF::getChLSB(read[i]);
                viennacl::linalg::real_to_complex(A, B, A.size());
                viennacl::fft(B, C);
                for(int i=0; i!=toread/2; i++) {
                    std::complex<float> cn(C(2*i),C(2*i+1));
                    points.push_back(QPointF(i/scale/toread,std::abs(cn)));
                }
                FTchBseries->clear();
                FTchBseries->append(points);
                FTgraph->addSeries(FTchAseries);
                FTgraph->addSeries(FTchBseries);
                FTaxisX->setRange(0,1/scale/2);
                FTchAseries->attachAxis(FTaxisX);
                FTchBseries->attachAxis(FTaxisX);
                FTchAseries->attachAxis(FTaxisY);
                FTchBseries->attachAxis(FTaxisY);
                //FTgraph->createDefaultAxes();

            }
            else firstRead=false;

            if(go.pRPTY->getNum(RPTY::A2F_RSCur,RPTY_A2F_queue)==0){
                std::vector<uint32_t> commands;
                commands.push_back(CQF::W4TRIG_INTR());
                commands.push_back(CQF::ACK(1<<RPTY_F2A_queue, selectedavg, selectedchannels, true));
                commands.push_back(CQF::WAIT(acksize*(1<<selectedavg)));
                commands.push_back(CQF::ACK(1<<RPTY_F2A_queue, selectedavg, selectedchannels, false));
                go.pRPTY->A2F_write(RPTY_A2F_queue,commands.data(),commands.size());
            }
            if(!trig_on_laser_pulse) go.pRPTY->trig(1<<RPTY_A2F_queue);
        }
    }
}
