#ifndef TAB_MONITOR_H
#define TAB_MONITOR_H
#include "GUI/gui_includes.h"
#include "DEV/RPTY/fpga_const.h"
class MainWindow;



class tab_monitor: public QWidget{
    Q_OBJECT
public:
    tab_monitor(QWidget* parent);
    void init_timer();
    QTimer *timer;

    QHBoxLayout* layout;

    QWidget* graphWidget;
    QVBoxLayout* graphLayout;

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
        QDoubleSpinBox* frequencySpinbox;
        QCheckBox* CBsaveWF;
        QCheckBox* CBsaveFT;
        QCheckBox* CBspectrogram;
        QCheckBox* CBtrig;

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
        constexpr static unsigned maxavg=31;
        QAction* _avg_actionList[maxavg+1];
        constexpr static double baseSamplFreq=8e-9;

        bool rdy_ch=false;
        bool rdy_avg=false;
        void try_unblock();
        bool rdy_ack=false;

        unsigned selectedavg=0;
        double acqfreq=1;           //in Hz
        CQF::ACK_CHANNELS selectedchannels;
        bool trig_on_laser_pulse=false;

        bool save_waveform=false;
        bool save_fft=false;
        bool calc_spec=false;
        bool tab_is_open=false;

        bool firstRead=true;

private Q_SLOTS:
    void on_channel_select(QAction *action);
    void on_averaging_select(QAction *action);
    void on_CBsaveWF_stateChanged(int state);
    void on_CBsaveFT_stateChanged(int state);
    void on_CBspectrogram_stateChanged(int state);
    void on_CBtrig_stateChanged(int state);
    void on_freqValueChanged();

public Q_SLOTS:
    void work_fun();                                  //this is called periodically via timer every work_call_time milliseconds
public:

};

#endif // TAB_MONITOR_H
