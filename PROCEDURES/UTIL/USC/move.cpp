#include "move.h"
#include "GUI/gui_includes.h"
#include "includes.h"

pgMoveGUI::pgMoveGUI(){
    init_gui_activation();
    init_gui_settings();
}

void pgMoveGUI::init_gui_activation(){
    gui_activation=new QWidget;
    alayout=new QVBoxLayout;
    gui_activation->setLayout(alayout);

    xMove=new eadScrlBar("Move X: ", 200,20);
    connect(xMove->abar, SIGNAL(change(double)), this, SLOT(_onMoveX(double)));
    alayout->addWidget(xMove);
    yMove=new eadScrlBar("Move Y: ", 200,20);
    connect(yMove->abar, SIGNAL(change(double)), this, SLOT(_onMoveY(double)));
    alayout->addWidget(yMove);
    zMove=new eadScrlBar("Move Z: ", 200,20);
    connect(zMove->abar, SIGNAL(change(double)), this, SLOT(_onMoveZ(double)));
    alayout->addWidget(zMove);
    fMove=new eadScrlBar("Move F: ", 200,20);
    connect(fMove->abar, SIGNAL(change(double)), this, SLOT(_onMoveF(double)));
    alayout->addWidget(fMove);
}

void pgMoveGUI::init_gui_settings(){
    gui_settings=new QWidget;
    slayout=new QVBoxLayout;
    gui_settings->setLayout(slayout);

    xMoveScale=new val_selector(0.001, "pgMoveGUI_xMoveScale", "X move multiplier: ", 0, 100, 6);
    slayout->addWidget(xMoveScale);
    yMoveScale=new val_selector(0.001, "pgMoveGUI_yMoveScale", "Y move multiplier: ", 0, 100, 6);
    slayout->addWidget(yMoveScale);
    zMoveScale=new val_selector(0.001, "pgMoveGUI_zMoveScale", "Z move multiplier: ", 0, 100, 6);
    slayout->addWidget(zMoveScale);
    fMoveScale=new val_selector(0.001, "pgMoveGUI_fMoveScale", "F move multiplier: ", 0, 100, 6);
    slayout->addWidget(fMoveScale);
}

void pgMoveGUI::onMove(double Xmov, double Ymov, double Zmov, double Fmov){
    go.pXPS->MoveRelative(XPS::mgroup_XYZF,Xmov*xMoveScale->val/1000,Ymov*xMoveScale->val/1000,Zmov*zMoveScale->val/1000,Fmov*fMoveScale->val/1000-Zmov*zMoveScale->val/1000);
}
