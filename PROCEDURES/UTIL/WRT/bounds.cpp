#include "bounds.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgBoundsGUI::pgBoundsGUI(){
    layout=new QVBoxLayout;
    layout->setMargin(0);
    this->setLayout(layout);
    selector=new twd_selector("Select active boundary:", "none", false, false);
    OOBLabel=new QLabel(); OOBLabel->setMaximumHeight(20); OOBLabel->setMaximumWidth(20); OOBLabel->setScaledContents(true);
    layout->addWidget(selector);
    layout->addWidget(new twid(new QLabel("In bounds:"),OOBLabel, false));
    layout->addStretch(0);

    circRad=new QWidget;
    circPts=new QWidget;
    rectDim=new QWidget;
    rectPts=new QWidget;
    layCircRad=new QVBoxLayout;
    layCircPts=new QVBoxLayout;
    layRectDim=new QVBoxLayout;
    layRectPts=new QVBoxLayout;
    circRad->setLayout(layCircRad);
    circPts->setLayout(layCircPts);
    rectDim->setLayout(layRectDim);
    rectPts->setLayout(layRectPts);
    selector->addWidget(circRad, "Circle: center+radius");
    selector->addWidget(circPts, "Circle: 3 points-clearence");
    selector->addWidget(rectDim, "Rectangle: center+width+height");
    selector->addWidget(rectPts, "Rectangle: 4 points-clearence");
    selector->addWidget(new QWidget, "none");

    setCircCenter=new QPushButton("Set");
    connect(setCircCenter, SIGNAL(released()), this, SLOT(onSetCircCenter()));
    setCircCenterTxt=new QLabel(QString::fromStdString(util::toString("Coords: ",circCenter[0],", ",circCenter[1])));
    selCircRadius=new val_selector(1,"pgBoundsGUI_selCircRadius", "Radius:",0,999999,3,0,{"um"});
    layCircRad->addWidget(new twid(setCircCenter, setCircCenterTxt));
    layCircRad->addWidget(selCircRadius);


    setCircEdge=new QPushButton("Set");
    connect(setCircEdge, SIGNAL(released()), this, SLOT(onSetCircEdge()));
    setCircEdgeTxt=new QLabel(QString::fromStdString(util::toString(getStatCirc())));
    layCircPts->addWidget(new twid(setCircEdge, setCircEdgeTxt));
    circClearance=new val_selector(0,"pgBoundsGUI_circClearance", "Clearance:",-999999,999999,3,0,{"um"});
    layCircPts->addWidget(circClearance);


    timer = new QTimer(this);
    timer->setInterval(timer_delay);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
}

void pgBoundsGUI::onSetCircCenter(){
    if(!go.pXPS->connected) return;
    for(int i=0;i!=2;i++) circCenter[i]=go.pXPS->getPos(XPS::mgroup_XYZF).pos[i];
    setCircCenterTxt->setText(QString::fromStdString(util::toString("Corrds: ",circCenter[0],", ",circCenter[1])));
}
void pgBoundsGUI::onSetCircEdge(){
    if(!go.pXPS->connected) return;
    for(int i=0;i!=2;i++) circEdge[cIter][i]=go.pXPS->getPos(XPS::mgroup_XYZF).pos[i];
    cIter++; if(cIter>2) cIter=0;
    setCircEdgeTxt->setText(QString::fromStdString(util::toString(getStatCirc())));
}
std::string pgBoundsGUI::getStatCirc(){
    std::string ret;
    ret+="Coords: ";
    for(int i=0;i!=3;i++) ret+=util::toString("(",circEdge[i][0],",",circEdge[i][1],")",i==0?"\n":" ");
    return ret;
}

bool pgBoundsGUI::isWithinBounds(double x, double y){
    switch(selector->index){
        case 0: return (sqrt(pow(x-circCenter[0],2)+pow(y-circCenter[1],2))<selCircRadius->val/1000);
        case 1: double xx,yy,rad;
                calcCenRad(xx, yy, rad);
                return (sqrt(pow(x-xx,2)+pow(y-yy,2))<(rad-circClearance->val/1000));
        case 2: return true;
        case 3: return true;
        default: return true;
    }
}


void pgBoundsGUI::calcCenRad(double &x, double &y, double &rad){
    if (((circEdge[0][0]==circEdge[1][0]) && (circEdge[2][0]==circEdge[1][0]))||((circEdge[0][1]==circEdge[1][1]) && (circEdge[2][1]==circEdge[1][1]))){
        rad=0; x=0; y=0;
    }
    double ma=(circEdge[1][1]-circEdge[0][1])/(circEdge[1][0]-circEdge[0][0]);
    double mb=(circEdge[2][1]-circEdge[1][1])/(circEdge[2][0]-circEdge[1][0]);
    if (circEdge[0][0]==circEdge[1][0]){
        x=(mb*(circEdge[2][1]-circEdge[0][1])+circEdge[1][0]-circEdge[2][0])/2;
        y=(circEdge[1][1]+circEdge[0][1])/2;
    }
    else if (circEdge[2][0]==circEdge[1][0]){
        x=(ma*(circEdge[0][1]-circEdge[2][1])+circEdge[1][0]+circEdge[0][0])/2;
        y=(circEdge[1][1]+circEdge[2][1])/2;
    }
    else{
        x=( ma*mb*(circEdge[0][1]-circEdge[2][1]) + mb*(circEdge[1][0]+circEdge[0][0]) - ma*(circEdge[1][0]+circEdge[2][0]) )/(2*(mb-ma));
        y=( mb*(circEdge[1][1]+circEdge[2][1]) - ma*(circEdge[1][1]+circEdge[0][1]) - circEdge[0][0] + circEdge[2][0] )/(2*(mb-ma));
    }
    rad=sqrt(pow(x-circEdge[0][0],2)+pow(y-circEdge[0][1],2));
}




void pgBoundsGUI::update(){
    double cur[2];
    for(int i=0;i!=2;i++) cur[i]=go.pXPS->getPos(XPS::mgroup_XYZF).pos[i];
    if(isWithinBounds(cur[0],cur[1])) OOBLabel->setPixmap(QPixmap(":/dialog-ok.svg"));
    else OOBLabel->setPixmap(QPixmap(":/gtk-no.svg"));
}


