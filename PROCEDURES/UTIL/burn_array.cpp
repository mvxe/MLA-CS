#include "burn_array.h"
#include "includes.h"

pBurnArray::pBurnArray(double espacing, double eexp_fst, double eexp_lst, int gridX, int gridY): spacing(espacing), exp_fst(eexp_fst), exp_lst(eexp_lst), gridX(gridX), gridY(gridY){
    spacing/=1000;
    exp_fst/=1000;
    exp_lst/=1000;
}
pBurnArray::~pBurnArray(){
}
void pBurnArray::run(){
    const double SMT=0.1;

    go.pXPS->setGPIO(XPS::iuScopeLED,true);
    go.pXPS->setGPIO(XPS::writingLaser,false);

    //XPS checks events every one servo cycle == 100us

    po = go.pXPS->createNewPVTobj(XPS::mgroup_XYZ, "pBurnArray.txt");

    for (int j=0;j!=gridY;j++){
        for (int i=0;i!=gridX;i++){
            po->add(SMT, spacing, 0, 0, 0, 0, 0);
            po->addAction(XPS::writingLaser,true);  //TODO fix bug: if addAction is the first command it may not do anything (sometimes)
            po->add(((exp_fst+(exp_lst-exp_fst)*(i+j*(gridX-1))/gridY/gridX)), 0, 0, 0, 0, 0, 0);
            po->addAction(XPS::writingLaser,false);
            po->add(((exp_fst+(exp_lst-exp_fst)*(i+j*(gridX-1))/gridY/gridX)), 0, 0, 0, 0, 0, 0);

            if(go.pXPS->verifyPVTobj(po).retval!=0) {std::cout<<"retval was"<<go.pXPS->verifyPVTobj(po).retstr<<"\n";return;}
            go.pXPS->execPVTobj(po, &ret);
            ret.block_till_done();
            po->clear();
        }

        po->add(SMT*gridX, -gridX*spacing, 0, spacing, 0, 0, 0);
        if(go.pXPS->verifyPVTobj(po).retval!=0) return;
        go.pXPS->execPVTobj(po, &ret);
        ret.block_till_done();
        po->clear();
    }

    go.pXPS->setGPIO(XPS::iuScopeLED,true);
    go.pXPS->setGPIO(XPS::writingLaser,false);

    done=true;
    end=true;
}

