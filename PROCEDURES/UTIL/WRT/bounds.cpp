#include "bounds.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgBoundsGUI::pgBoundsGUI(pgMoveGUI* pgMGUI, pgBeamAnalysis* pgBeAn): pgMGUI(pgMGUI), pgBeAn(pgBeAn){
    conf["saveCircCenterX"]=circCenter[0];
    conf["saveCircCenterY"]=circCenter[1];
    conf["saveCircEdgeX0"]=circEdge[0][0];
    conf["saveCircEdgeX1"]=circEdge[1][0];
    conf["saveCircEdgeX2"]=circEdge[2][0];
    conf["saveCircEdgeY0"]=circEdge[0][1];
    conf["saveCircEdgeY1"]=circEdge[1][1];
    conf["saveCircEdgeY2"]=circEdge[2][1];
    conf["saveRectCenterX"]=rectCenter[0];
    conf["saveRectCenterY"]=rectCenter[1];
    conf["saveRectEdgeX0"]=rectEdge[0][0];
    conf["saveRectEdgeX1"]=rectEdge[1][0];
    conf["saveRectEdgeX2"]=rectEdge[2][0];
    conf["saveRectEdgeX3"]=rectEdge[3][0];
    conf["saveRectEdgeY0"]=rectEdge[0][1];
    conf["saveRectEdgeY1"]=rectEdge[1][1];
    conf["saveRectEdgeY2"]=rectEdge[2][1];
    conf["saveRectEdgeY3"]=rectEdge[3][1];
    conf["saveIndex"]=index;

    layout=new QVBoxLayout;
    layout->setMargin(0);
    this->setLayout(layout);
    selector=new twd_selector("Select active boundary:", "none", false, false, true);
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
    selCircRadius=new val_selector(1, "Radius:",0,999999,3,0,{"um"});
    conf["selCircRadius"]=selCircRadius;
    layCircRad->addWidget(new twid(setCircCenter, setCircCenterTxt));
    layCircRad->addWidget(selCircRadius);


    setCircEdge=new QPushButton("Set");
    connect(setCircEdge, SIGNAL(released()), this, SLOT(onSetCircEdge()));
    setCircEdgeTxt=new QLabel(QString::fromStdString(util::toString(getStatCirc())));
    layCircPts->addWidget(new twid(setCircEdge, setCircEdgeTxt));
    circClearance=new val_selector(0, "Clearance:",-999999,999999,3,0,{"um"}); circClearance->setToolTip("A positive clearance makes the active area smaller.");
    conf["circClearance"]=circClearance;
    layCircPts->addWidget(circClearance);

    setRectCenter=new QPushButton("Set");
    connect(setRectCenter, SIGNAL(released()), this, SLOT(onSetRectCenter()));
    setRectCenterTxt=new QLabel(QString::fromStdString(util::toString("Coords: ",rectCenter[0],", ",rectCenter[1])));
    selRectWidth=new val_selector(1, "Width:",0,999999,3,0,{"um"});
    conf["selRectWidth"]=selRectWidth;
    selRectHeight=new val_selector(1, "Height:",0,999999,3,0,{"um"});
    conf["selRectHeight"]=selRectHeight;
    layRectDim->addWidget(new twid(setRectCenter, setRectCenterTxt));
    layRectDim->addWidget(selRectWidth);
    layRectDim->addWidget(selRectHeight);

    setRectEdge=new QPushButton("Set");
    connect(setRectEdge, SIGNAL(released()), this, SLOT(onSetRectEdge()));
    setRectEdgeTxt=new QLabel(QString::fromStdString(util::toString(getStatRect())));
    layRectPts->addWidget(new twid(setRectEdge, new QString("Coords: ")));
    layRectPts->addWidget(setRectEdgeTxt);
    rectClearance=new val_selector(0, "Clearance:",-999999,999999,3,0,{"um"}); rectClearance->setToolTip("A positive clearance makes the active area smaller.");
    conf["rectClearance"]=rectClearance;
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
    XPS::raxis tmp=go.pXPS->getPos(XPS::mgroup_XYZF);
    circCenter[0]=tmp.pos[0]-pgBeAn->writeBeamCenterOfsX*pgMGUI->getNmPPx()/1000000;  //-correct for noncentered write beam
    circCenter[1]=tmp.pos[1]-pgBeAn->writeBeamCenterOfsY*pgMGUI->getNmPPx()/1000000;
    setCircCenterTxt->setText(QString::fromStdString(util::toString("Corrds: ",circCenter[0],", ",circCenter[1])));
}
void pgBoundsGUI::onSetCircEdge(){
    if(!go.pXPS->connected) return;
    XPS::raxis tmp=go.pXPS->getPos(XPS::mgroup_XYZF);
    circEdge[cIter][0]=tmp.pos[0]-pgBeAn->writeBeamCenterOfsX*pgMGUI->getNmPPx()/1000000;
    circEdge[cIter][1]=tmp.pos[1]-pgBeAn->writeBeamCenterOfsY*pgMGUI->getNmPPx()/1000000;
    cIter++; if(cIter>2) cIter=0;
    setCircEdgeTxt->setText(QString::fromStdString(util::toString(getStatCirc())));
}
void pgBoundsGUI::onSetRectCenter(){
    if(!go.pXPS->connected) return;
    XPS::raxis tmp=go.pXPS->getPos(XPS::mgroup_XYZF);
    rectCenter[0]=tmp.pos[0]-pgBeAn->writeBeamCenterOfsX*pgMGUI->getNmPPx()/1000000;
    rectCenter[1]=tmp.pos[1]-pgBeAn->writeBeamCenterOfsY*pgMGUI->getNmPPx()/1000000;
    setRectCenterTxt->setText(QString::fromStdString(util::toString("Corrds: ",rectCenter[0],", ",rectCenter[1])));
}
void pgBoundsGUI::onSetRectEdge(){
    if(!go.pXPS->connected) return;
    XPS::raxis tmp=go.pXPS->getPos(XPS::mgroup_XYZF);
    rectEdge[rIter][0]=tmp.pos[0]-pgBeAn->writeBeamCenterOfsX*pgMGUI->getNmPPx()/1000000;
    rectEdge[rIter][1]=tmp.pos[1]-pgBeAn->writeBeamCenterOfsY*pgMGUI->getNmPPx()/1000000;
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
    x-=pgBeAn->writeBeamCenterOfsX*pgMGUI->getNmPPx()/1000000;  //correct for noncentered write beam
    y-=pgBeAn->writeBeamCenterOfsY*pgMGUI->getNmPPx()/1000000;
    switch(selector->index){
        case 1: return (sqrt(pow(x-circCenter[0],2)+pow(y-circCenter[1],2))<selCircRadius->val/1000);
        case 2: double xx,yy,rad;
                calcCenRad(xx, yy, rad);
                return (sqrt(pow(x-xx,2)+pow(y-yy,2))<(rad-circClearance->val/1000));
        case 3: return ((abs(x-rectCenter[0])<selRectWidth->val/2000)&&(abs(y-rectCenter[1])<selRectHeight->val/2000));
        case 4: return isWithinRect(x,y);
        default: return true;
    }
}


void pgBoundsGUI::calcCenRad(double &x, double &y, double &rad){        // TODO: these two keep doing the same calcs every time, perhaps save them in variables instead?
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
    if(!go.pXPS->connected) return;
    if(!go.pXPS->isQueueEmpty())  return;
    double cur[2];
    XPS::raxis tmp=go.pXPS->getPos(XPS::mgroup_XYZF);
    for(int i=0;i!=2;i++) cur[i]=tmp.pos[i];
    if(isWithinBounds(cur[0],cur[1])) OOBLabel->setPixmap(QPixmap(":/dialog-ok.svg"));
    else OOBLabel->setPixmap(QPixmap(":/gtk-no.svg"));
}

void pgBoundsGUI::drawBound(cv::Mat* img, double XYnmppx, bool isMask){
    if(go.pXPS->connected) if(go.pXPS->isQueueEmpty()){
        XPS::raxis tmp=go.pXPS->getPos(XPS::mgroup_XYZF);
        for(int i=0;i!=2;i++) cur[i]=tmp.pos[i];
    }
    int ofsX=img->cols/2;
    int ofsY=img->rows/2;

    cv::Mat mask;
    if(isMask) mask=cv::Mat(img->rows,img->cols,CV_8U,cv::Scalar(0));

    double clr[2]={0,255}; int thck[2]={3,1};
    for(int i=0;i!=2;i++)
    if(selector->index==1){
        if(isMask) cv::circle(mask, {(int)((cur[0]-circCenter[0])*1000000/XYnmppx+ofsX),(int)((cur[1]-circCenter[1])*1000000/XYnmppx+ofsY)}, (int)(selCircRadius->val*1000/XYnmppx), 255, -1);
        else for(int i=0;i!=2;i++){
            cv::circle(*img, {(int)((cur[0]-circCenter[0])*1000000/XYnmppx+ofsX),(int)((cur[1]-circCenter[1])*1000000/XYnmppx+ofsY)}, (int)(selCircRadius->val*1000/XYnmppx), {clr[i]}, thck[i], cv::LINE_AA);
        }

    }else if(selector->index==2){
        double xx,yy,rad;
        calcCenRad(xx, yy, rad);
        if(isnan(xx)||isnan(yy)||isnan(rad)) break;
        if(isMask) cv::circle(mask, {(int)((cur[0]-xx)*1000000/XYnmppx+ofsX),(int)((cur[1]-yy)*1000000/XYnmppx+ofsY)}, (int)((rad-circClearance->val/1000)*1000000/XYnmppx), 255, -1);
        else for(int i=0;i!=2;i++)
            cv::circle(*img, {(int)((cur[0]-xx)*1000000/XYnmppx+ofsX),(int)((cur[1]-yy)*1000000/XYnmppx+ofsY)}, (int)((rad-circClearance->val/1000)*1000000/XYnmppx), {clr[i]}, thck[i], cv::LINE_AA);
    }else if(selector->index==3){
        if(isMask) cv::rectangle(mask, {(int)((cur[0]-rectCenter[0]-selRectWidth->val/1000/2)*1000000/XYnmppx+ofsX),(int)((cur[1]-rectCenter[1]-selRectHeight->val/1000/2)*1000000/XYnmppx+ofsY),(int)(selRectWidth->val*1000/XYnmppx),(int)(selRectHeight->val*1000/XYnmppx)}, 255, -1);
        else for(int i=0;i!=2;i++)
            cv::rectangle(*img, {(int)((cur[0]-rectCenter[0]-selRectWidth->val/1000/2)*1000000/XYnmppx+ofsX),(int)((cur[1]-rectCenter[1]-selRectHeight->val/1000/2)*1000000/XYnmppx+ofsY),(int)(selRectWidth->val*1000/XYnmppx),(int)(selRectHeight->val*1000/XYnmppx)}, {clr[i]}, thck[i], cv::LINE_AA);
    }else if(selector->index==4){
        double points[4][2]; double avgPt[2]={0,0};
        double lines[4][2];
        for(int i=0;i!=4;i++){
            int itr[4];
            for(int j=0;j!=4;j++) itr[j]=(i+j)%4;
            lines[i][0]=(rectEdge[itr[0]][1]-rectEdge[itr[1]][1])/(rectEdge[itr[0]][0]-rectEdge[itr[1]][0]);
            lines[i][1]=rectEdge[itr[0]][1]-lines[i][0]*rectEdge[itr[0]][0];
            avgPt[0]+=rectEdge[i][0];
            avgPt[1]+=rectEdge[i][1];
        }
        avgPt[0]/=4; avgPt[1]/=4;
        if(!isWithinRect(avgPt[0], avgPt[1])) break;
        bool dirs[4];
        for(int i=0;i!=4;i++){
            getLineDisDir(lines[i][0], lines[i][1], avgPt[0], avgPt[1], &dirs[i]);
            lines[i][1]+=rectClearance->val/1000/cos(atan(-lines[i][0]))*(dirs[i]?(-1):1)*(lines[i][0]<0?(-1):1);
        }
        for(int i=0;i!=4;i++){
            int itr[4];
            for(int j=0;j!=4;j++) itr[j]=(i+j)%4;
            points[i][0]=(lines[itr[1]][1]-lines[itr[0]][1])/(lines[itr[0]][0]-lines[itr[1]][0]);
            points[i][1]=points[i][0]*lines[itr[0]][0]+lines[itr[0]][1];
        }
        if(isMask){
            cv::Point cvpoints[4];
            for(int i=0;i!=4;i++) cvpoints[i]=cv::Point((int)((cur[0]-points[i][0])*1000000/XYnmppx+ofsX),(int)((cur[1]-points[i][1])*1000000/XYnmppx+ofsY));
            cv::fillConvexPoly(mask, cvpoints, 4, 255);
        }else for(int j=0;j!=2;j++)
            for(int i=0;i!=4;i++){
                int itr[4];
                for(int j=0;j!=4;j++) itr[j]=(i+j)%4;
                cv::line(*img, {(int)((cur[0]-points[itr[0]][0])*1000000/XYnmppx+ofsX),(int)((cur[1]-points[itr[0]][1])*1000000/XYnmppx+ofsY)}, {(int)((cur[0]-points[itr[1]][0])*1000000/XYnmppx+ofsX),(int)((cur[1]-points[itr[1]][1])*1000000/XYnmppx+ofsY)}, {clr[j]}, thck[j], cv::LINE_AA);
            }
    }

    if(isMask && selector->index!=0){
        cv::bitwise_not(mask, mask);
        cv::bitwise_or(*img,mask,*img);
    }
}
