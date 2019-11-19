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
    selector->addWidget(new QWidget, "none");
    selector->addWidget(circRad, "Circle: center+radius");
    selector->addWidget(circPts, "Circle: 3 points-clearence");
    selector->addWidget(rectDim, "Rectangle: center+width+height");
    selector->addWidget(rectPts, "Rectangle: 4 points-clearence");
    selector->setIndex(index);

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
    circClearance=new val_selector(0,"pgBoundsGUI_circClearance", "Clearance:",-999999,999999,3,0,{"um"}); circClearance->setToolTip("A positive clearance makes the active area smaller.");
    layCircPts->addWidget(circClearance);

    setRectCenter=new QPushButton("Set");
    connect(setRectCenter, SIGNAL(released()), this, SLOT(onSetRectCenter()));
    setRectCenterTxt=new QLabel(QString::fromStdString(util::toString("Coords: ",rectCenter[0],", ",rectCenter[1])));
    selRectWidth=new val_selector(1,"pgBoundsGUI_selRectWidth", "Width:",0,999999,3,0,{"um"});
    selRectHeight=new val_selector(1,"pgBoundsGUI_selRectHeight", "Height:",0,999999,3,0,{"um"});
    layRectDim->addWidget(new twid(setRectCenter, setRectCenterTxt));
    layRectDim->addWidget(selRectWidth);
    layRectDim->addWidget(selRectHeight);

    setRectEdge=new QPushButton("Set");
    connect(setRectEdge, SIGNAL(released()), this, SLOT(onSetRectEdge()));
    setRectEdgeTxt=new QLabel(QString::fromStdString(util::toString(getStatRect())));
    layRectPts->addWidget(new twid(setRectEdge, new QString("Coords: ")));
    layRectPts->addWidget(setRectEdgeTxt);
    rectClearance=new val_selector(0,"pgBoundsGUI_rectClearance", "Clearance:",-999999,999999,3,0,{"um"}); rectClearance->setToolTip("A positive clearance makes the active area smaller.");
    layRectPts->addWidget(rectClearance);

    timer = new QTimer(this);
    timer->setInterval(timer_delay);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
}
pgBoundsGUI::~pgBoundsGUI(){
    index=selector->index;
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
void pgBoundsGUI::onSetRectCenter(){
    if(!go.pXPS->connected) return;
    for(int i=0;i!=2;i++) rectCenter[i]=go.pXPS->getPos(XPS::mgroup_XYZF).pos[i];
    setRectCenterTxt->setText(QString::fromStdString(util::toString("Corrds: ",rectCenter[0],", ",rectCenter[1])));
}
void pgBoundsGUI::onSetRectEdge(){
    if(!go.pXPS->connected) return;
    for(int i=0;i!=2;i++) rectEdge[rIter][i]=go.pXPS->getPos(XPS::mgroup_XYZF).pos[i];
    rIter++; if(rIter>3) rIter=0;
    setRectEdgeTxt->setText(QString::fromStdString(util::toString(getStatRect())));
}
std::string pgBoundsGUI::getStatCirc(){
    std::string ret;
    ret+="Coords: ";
    for(int i=0;i!=3;i++) ret+=util::toString("(",circEdge[i][0],",",circEdge[i][1],")",i==0?"\n":" ");
    return ret;
}
std::string pgBoundsGUI::getStatRect(){
    std::string ret;
    for(int i=0;i!=4;i++) ret+=util::toString("(",rectEdge[i][0],",",rectEdge[i][1],")",i==1?"\n":" ");
    return ret;
}

bool pgBoundsGUI::isWithinBounds(double x, double y){
    switch(selector->index){
        case 1: return (sqrt(pow(x-circCenter[0],2)+pow(y-circCenter[1],2))<selCircRadius->val/1000);
        case 2: double xx,yy,rad;
                calcCenRad(xx, yy, rad);
                return (sqrt(pow(x-xx,2)+pow(y-yy,2))<(rad-circClearance->val/1000));
        case 3: return ((abs(x-rectCenter[0])<selRectWidth->val/1000)&&(abs(y-rectCenter[1])<selRectHeight->val/1000));
        case 4: return isWithinRect(x,y);
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

bool pgBoundsGUI::isWithinRect(double x, double y){
    for(int i=0;i!=4;i++){  //do for all 4 combinations / boundaries
        int itr[4];
        for(int j=0;j!=4;j++) itr[j]=(i+j)%4;
        double a,b;
        a=(rectEdge[itr[0]][1]-rectEdge[itr[1]][1])/(rectEdge[itr[0]][0]-rectEdge[itr[1]][0]);
        b=rectEdge[itr[0]][1]-a*rectEdge[itr[0]][0];
        double dis;
        bool dir[3];
        getLineDisDir(a, b, rectEdge[itr[2]][0], rectEdge[itr[2]][1], &dir[1]);
        getLineDisDir(a, b, rectEdge[itr[3]][0], rectEdge[itr[3]][1], &dir[2]);
        if(dir[1]!=dir[2]) return false;    //convex
        getLineDisDir(a, b, x, y, &dir[0], &dis);
        if((dir[0]==dir[1]?1:(-1))*dis < rectClearance->val/1000) return false;
    }
    return true;
}
void pgBoundsGUI::getLineDisDir(double a, double b, double x, double y, bool* dir, double* dis){
    double c=y+x/a;
    double X,Y;
    X=(c-b)/(a+1/a);
    Y=a*X+b;
    *dir=(x!=X)?(x>X):(y>Y);
    if(dis!=nullptr) *dis=sqrt(pow(x-X,2)+pow(y-Y,2));
}



void pgBoundsGUI::update(){
    double cur[2];
    for(int i=0;i!=2;i++) cur[i]=go.pXPS->getPos(XPS::mgroup_XYZF).pos[i];
    if(isWithinBounds(cur[0],cur[1])) OOBLabel->setPixmap(QPixmap(":/dialog-ok.svg"));
    else OOBLabel->setPixmap(QPixmap(":/gtk-no.svg"));
}


