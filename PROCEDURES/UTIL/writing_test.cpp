#include "writing_test.h"
#include "includes.h"

pWritingTest::pWritingTest(){
}
pWritingTest::~pWritingTest(){
}
void pWritingTest::run(){
    const double TotDim=0.200;    //200um
    const double SMT=0.1;
    const double speed=1;

    go.pXPS->setGPIO(XPS::iuScopeLED,true);
    go.pXPS->setGPIO(XPS::writingLaser,false);

    po = go.pXPS->createNewPVTobj(XPS::mgroup_XYZ, "pWritingTest.txt");


    po->add(SMT,                -0.6*TotDim,  0    ,   0.5*TotDim     , 0     ,0,0);
    po->add(SMT,                 0.1*TotDim,  speed,   0              , 0     ,0,0);
    po->addAction(XPS::writingLaser,true);
    po->add(1.0*TotDim/speed,    1.0*TotDim,  speed,   0              , 0     ,0,0);
    po->addAction(XPS::writingLaser,false);
    po->add(SMT,                 0.1*TotDim,  0    ,   0              , 0     ,0,0);
    po->add(SMT,                -0.1*TotDim,  0    ,   0.1*TotDim     , 0     ,0,0);
    po->add(SMT,                 0         ,  0    ,  -0.1*TotDim     ,-speed ,0,0);
    po->addAction(XPS::writingLaser,true);
    po->add(1.0*TotDim/speed,    0         ,  0    ,  -1.0*TotDim     ,-speed ,0,0);
    po->addAction(XPS::writingLaser,false);
    po->add(SMT,                 0         ,  0    ,  -0.1*TotDim     , 0     ,0,0);
    po->add(SMT,                 0.1*TotDim,  0    ,   0.1*TotDim     , 0     ,0,0);
    po->add(SMT,                -0.1*TotDim, -speed,   0              , 0     ,0,0);
    po->addAction(XPS::writingLaser,true);
    po->add(1.0*TotDim/speed,   -1.0*TotDim, -speed,   0              , 0     ,0,0);
    po->addAction(XPS::writingLaser,false);
    po->add(SMT,                -0.1*TotDim,  0    ,   0              , 0     ,0,0);
    po->add(SMT,                 0.1*TotDim,  0    ,  -0.1*TotDim     , 0     ,0,0);
    po->add(SMT,                 0         ,  0    ,   0.1*TotDim     , speed ,0,0);
    po->addAction(XPS::writingLaser,true);
    po->add(1.0*TotDim/speed,    0         ,  0    ,   1.0*TotDim     , speed ,0,0);
    po->addAction(XPS::writingLaser,false);
    po->add(SMT,                 0         ,  0    ,   0.1*TotDim     , 0     ,0,0);
    po->add(SMT,                 0.5*TotDim,  0    ,  -0.6*TotDim     , 0     ,0,0);

    //WARNING: THE MAX EVENT NUMBER OF XPS IS ABOUT 32
    po->addAction(XPS::iuScopeLED,true);
    po->addAction(XPS::writingLaser,false);

    if(go.pXPS->verifyPVTobj(po).retval!=0) return;
    go.pXPS->execPVTobj(po, &ret);


    //ret.block_till_done();


    //go.pXPS->setGPIO(XPS::iuScopeLED,true);
    //go.pXPS->setGPIO(XPS::writingLaser,false);
    done=true;
    end=true;
}

