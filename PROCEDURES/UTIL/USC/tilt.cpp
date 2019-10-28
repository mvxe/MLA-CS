#include "tilt.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgTiltGUI::pgTiltGUI(){
    timer = new QTimer(this);
    timer->setInterval(work_call_time);
    connect(timer, SIGNAL(timeout()), this, SLOT(work_fun()));
    init_gui_activation();
    init_gui_settings();
}

void pgTiltGUI::init_gui_activation(){
    gui_activation=new QWidget;
    alayout=new QHBoxLayout;
    gui_activation->setLayout(alayout);
    QLabel* label= new QLabel("Tilt adjustment:");
    alayout->addWidget(label);
    joyBtn=new joyBttn;
    joyBtn->setMouseTracking(false);
    joyBtn->setFrameShape(QFrame::Box);
    joyBtn->setFrameShadow(QFrame::Plain);
    joyBtn->setScaledContents(false);
    joyBtn->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    joyBtn->parent=this;
    box=new cv::Mat(boxSize,boxSize,CV_8UC3);
    reDraw(boxSize/2,boxSize/2);
    alayout->addWidget(joyBtn);
    alayout->addStretch(0);
}

void pgTiltGUI::init_gui_settings(){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);
    tilt_mult=new val_selector(0, "pgTiltGUI_tilt_mult", "Tilt multiplier ", -100, 100, 0, {"au/pix/sec"});
    slayout->addWidget(tilt_mult);
    focus_autoadjX=new val_selector(0, "pgTiltGUI_focus_autoadjX", "Focus adjustment for X ", -100, 100, 0, {"um/au"});
    slayout->addWidget(focus_autoadjX);
    focus_autoadjY=new val_selector(0, "pgTiltGUI_focus_autoadjY", "Focus adjustment for Y ", -100, 100, 0, {"um/au"});
    slayout->addWidget(focus_autoadjY);
}

void pgTiltGUI::work_fun(){
    if(!go.pCNC->connected || !go.pXPS->connected) return;
    if(!inited){
        go.pCNC->execCommand("M999\n");
        std::this_thread::sleep_for (std::chrono::milliseconds(10));
        go.pCNC->execCommand("M18 S10\n"); //set stepper disable inactivity to 10 seconds
        go.pCNC->execCommand("G91\n");      //set to relative positioning
        inited=true;
    }
    double dX,dY,dZ;
    double speed;
    dX=Xmov*tilt_mult->val/1000;
    dY=Ymov*tilt_mult->val/1000;
    go.pCNC->execCommand("G1 X",dX," Y",dY," F100\n");
    dZ=(Xmov*focus_autoadjX->val+Ymov*focus_autoadjY->val)/100000;
    go.pXPS->MoveRelative(XPS::mgroup_XYZF,0,0,dZ,-dZ);
}

void joyBttn::mouseMoveEvent(QMouseEvent *event){
    parent->reDraw(event->pos().x(),event->pos().y());
}
void joyBttn::mousePressEvent(QMouseEvent *event){ mouseMoveEvent(event);}

void pgTiltGUI::reDraw0(){reDraw(boxSize/2,boxSize/2);}
void pgTiltGUI::reDraw(int x, int y){
    box->setTo(cv::Scalar(0,0,0));
    for(int i=0;i!=2;i++){
        cv::line(*box, cv::Point(0,boxSize/2+(2*i-1)*limSize), cv::Point(boxSize,boxSize/2+(2*i-1)*limSize), cv::Scalar(255,255,255));
        cv::line(*box, cv::Point(boxSize/2+(2*i-1)*limSize,0), cv::Point(boxSize/2+(2*i-1)*limSize,boxSize), cv::Scalar(255,255,255));
    }
    cv::circle(*box, cv::Point(x,y), 1, cv::Scalar(255,0,0));
    joyBtn->setPixmap(QPixmap::fromImage(QImage(box->data, box->cols, box->rows, box->step, QImage::Format_RGB888)));

    if(x<0)x=0;
    else if(x>=boxSize) x=boxSize-1;
    if(y<0)y=0;
    else if(y>=boxSize) y=boxSize-1;
    Xmov=x-boxSize/2;
    Ymov=y-boxSize/2;
    if(abs(Xmov)>limSize) Xmov+=(Xmov>0?-1:1)*limSize;
    else Xmov=0;
    if(abs(Ymov)>limSize) Ymov+=(Ymov>0?-1:1)*limSize;
    else Ymov=0;
    if(Xmov!=0 || Ymov!=0) timer->start();
    else timer->stop();
}

