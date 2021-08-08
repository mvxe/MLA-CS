#ifndef TAB_MONITOR_H
#define TAB_MONITOR_H
#include "GUI/gui_includes.h"
#include "DEV/RPTY/fpga_const.h"
class MainWindow;



class tab_monitor: public QWidget{
    Q_OBJECT
public:
    tab_monitor(QWidget* parent);
    ~tab_monitor();
    void init_timer();
    rtoml::vsr conf{"app_monitor.toml"};                //configuration map
    QTimer *timer;

    QHBoxLayout* layout;

    QWidget* graphWidget;
    QVBoxLayout* graphLayout;

    val_selector* pulseInt;
    val_selector* pulseDur;
    QWidget* buttonWidget;
        QChartView* WFgraphView;
        QChartView* FTgraphView;
        QChartView* SGgraphView;
        QChart* WFgraph;
        QChart* FTgraph;
        QChart* SGgraph;
        QLineSeries *WFchAseries;
        QLineSeries *WFchBseries;
        QLineSeries *FTchAseries;
        QLineSeries *FTchBseries;
        QValueAxis *WFaxisX;
        QValueAxis *WFaxisY;
        QValueAxis *FTaxisX;
        QLogValueAxis *FTaxisY;

    QVBoxLayout* buttonLayout;
        QToolButton* channelSelect;
        QToolButton* averagingSelect;
        QCheckBox* CBfastack;
        QCheckBox* CBsaveWF;
        QCheckBox* CBsaveFT;
        QCheckBox* CBspectrogram;
        QCheckBox* CBtrig;
        QCheckBox* CBlpauto;
        QPushButton* rstView;

    MainWindow* pmw;

    double WFLastRange=0;
    double FTLastRange=0;

    struct _sTB_btns{
        std::string text;
        std::string icon;
        QAction* action;
        CQF::ACK_CHANNELS channels;
    };
    static const int BN_channel=6;
    _sTB_btns _sTB_channel[BN_channel]{
        {"ADC-A && ADC-B","",nullptr,CQF::fADC_A__fADC_B},
        {"ADC-A && PID-0","",nullptr,CQF::fADC_A__PIDO_0},
        {"ADC-A && PID-1","",nullptr,CQF::fADC_A__PIDO_1},
        {"ADC-B && PID-0","",nullptr,CQF::fADC_B__PIDO_0},
        {"ADC-B && PID-1","",nullptr,CQF::fADC_B__PIDO_1},
        {"PID-0 && PID-1","",nullptr,CQF::PIDO_0__PIDO_1}
    };
    constexpr static unsigned work_call_time=1000;    //work_fun is called periodically via timer every this many milliseconds
    constexpr static unsigned work_call_time_fast=16.667;
    bool using_fast=false;
    constexpr static unsigned maxavg=31;
    QAction* _avg_actionList[maxavg+1];
    constexpr static double baseSamplFreq=8e-9;

    bool rdy_ch=false;
    bool rdy_avg=false;
    void try_unblock();
    bool rdy_ack=false;

    unsigned selectedavg=0;
    CQF::ACK_CHANNELS selectedchannels;
    bool trig_on_laser_pulse=false;
    bool laser_pulse_auto=false;
    constexpr static unsigned RPTY_A2F_queue=3;
    constexpr static unsigned RPTY_F2A_queue=0;

    bool save_waveform=false;
    bool save_fft=false;
    bool calc_spec=false;
    bool tab_is_open=false;

private Q_SLOTS:
    void on_channel_select(QAction *action);
    void on_averaging_select(QAction *action);
    void on_CBsaveWF_stateChanged(int state);
    void on_CBsaveFT_stateChanged(int state);
    void on_CBspectrogram_stateChanged(int state);
    void on_CBtrig_stateChanged(int state);
    void on_CBlpauto_stateChanged(int state);
    void on_rstView_released();
    void on_CBfastack_stateChanged(int state);

public Q_SLOTS:
    void work_fun();                                  //this is called periodically via timer every work_call_time milliseconds
public:

};

#endif // TAB_MONITOR_H
