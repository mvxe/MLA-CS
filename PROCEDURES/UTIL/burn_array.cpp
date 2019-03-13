#include "burn_array.h"
#include "includes.h"

pBurnArray::pBurnArray(double espacing, double eexp_fst, double eexp_lst, int gridX, int gridY, bool vac): spacing(espacing), exp_fst(eexp_fst), exp_lst(eexp_lst), gridX(gridX), gridY(gridY), vac(vac),filename(""),useLines(false){
    spacing/=1000;
    exp_fst/=1000;
    exp_lst/=1000;
}
pBurnArray::pBurnArray(std::string filename, bool useLines):spacing(0), exp_fst(0), exp_lst(0), gridX(0), gridY(0), vac(0),filename(filename),useLines(useLines){}
pBurnArray::~pBurnArray(){
}
void pBurnArray::run(){
    const double SMT=0.1;

    go.pXPS->setGPIO(XPS::iuScopeLED,true);
    go.pXPS->setGPIO(XPS::writingLaser,false);

    //XPS checks events every one servo cycle == 100us

    po = go.pXPS->createNewPVTobj(XPS::mgroup_XYZF, "pBurnArray.txt");

    if(filename.empty())
        for (int j=0;j!=gridY;j++){
            for (int i=0;i!=gridX;i++){
                if(end) break;
                po->add(SMT, spacing, 0, 0, 0, 0, 0,0,0);
                if(!(vac && (j%2) && (i%2))) po->addAction(XPS::writingLaser,true);  //TODO fix bug: if addAction is the first command it may not do anything (sometimes)
                if(!vac) po->add(((exp_fst+(exp_lst-exp_fst)*(i+j*(gridX-1))/gridY/gridX)), 0, 0, 0, 0, 0, 0,0,0);
                else if((!(j%2)) && (!(i%2))) po->add(exp_fst, 0, 0, 0, 0, 0, 0,0,0);
                else  po->add(exp_lst, 0, 0, 0, 0, 0, 0,0,0);
                po->addAction(XPS::writingLaser,false);
                if(!vac) po->add(((exp_fst+(exp_lst-exp_fst)*(i+j*(gridX-1))/gridY/gridX)), 0, 0, 0, 0, 0, 0,0,0);
                else if((!(j%2)) && (!(i%2))) po->add(exp_fst, 0, 0, 0, 0, 0, 0,0,0);
                else  po->add(exp_lst, 0, 0, 0, 0, 0, 0,0,0);
                if(go.pXPS->verifyPVTobj(po).retval!=0) {std::cout<<"retval was"<<go.pXPS->verifyPVTobj(po).retstr<<"\n";return;}
                go.pXPS->execPVTobj(po, &ret);
                ret.block_till_done();
                po->clear();
            }
            if(end) break;
            po->add(SMT*gridX, -gridX*spacing, 0, spacing, 0, 0, 0,0,0);
            if(go.pXPS->verifyPVTobj(po).retval!=0) return;
            go.pXPS->execPVTobj(po, &ret);
            ret.block_till_done();
            po->clear();
        }
    else{
        std::string line;
        std::ifstream datafile(filename);
        double X,Y,T;
        double oX=0, oY=0;
        double dis;

        std::string::size_type sza,szb;
        if (datafile.is_open()){

            if(!useLines){
                while(getline(datafile,line)){
                    if(end) break;
                    if(line.size()<2 || line.find("#")!=std::string::npos);
                    else{
                        X=std::stod(line,&sza)/1000;
                        Y=std::stod(line.substr(sza),&szb)/1000;
                        T=std::stod(line.substr(sza+szb))/1000;
                        dis=sqrt(pow(X-oX,2)+pow(Y-oY,2));
                        po->add(dis*SMT*100, X-oX, 0, Y-oY, 0, 0, 0,0,0);
                        if(T>=0.0001){
                            po->addAction(XPS::writingLaser,true);
                            po->add(T, 0, 0, 0, 0, 0, 0,0,0);
                            po->addAction(XPS::writingLaser,false);
                        }
                        if(go.pXPS->verifyPVTobj(po).retval!=0) {std::cout<<"retval was"<<go.pXPS->verifyPVTobj(po).retstr<<"\n";return;}
                        go.pXPS->execPVTobj(po, &ret);
                        std::cout<<X<<" "<<Y<<" "<<T<<"\n";
                        ret.block_till_done();
                        po->clear();
                        oX=X; oY=Y;
                    }
                }
            }else{
                bool execL=false;
                double fDis=0;
                const double minT=0.01;
                while(getline(datafile,line)){
                    if(end) break;
                    if(line.size()<2 || line.find("#")!=std::string::npos);
                    else{
                        X=std::stod(line,&sza)/1000;
                        Y=std::stod(line.substr(sza),&szb)/1000;
                        T=std::stod(line.substr(sza+szb))/1000;
                        dis=sqrt(pow(X-oX,2)+pow(Y-oY,2));
                        if((dis>fDis/10)){
                            if(execL==true){
                                execL=false;
                                po->addAction(XPS::writingLaser,false);
                                if(go.pXPS->verifyPVTobj(po).retval!=0) {std::cout<<"retval was"<<go.pXPS->verifyPVTobj(po).retstr<<"\n";return;}
                                go.pXPS->execPVTobj(po, &ret);
                                std::cout<<X<<" "<<Y<<" "<<T<<"\n";
                                ret.block_till_done();
                                po->clear();
                            }
                            fDis=dis;
                            po->add(dis*SMT*100, X-oX, 0, Y-oY, 0, 0, 0,0,0);
                        }
                        else{
                            if(T<minT) T=minT;
                            if(execL==false){
                                po->addAction(XPS::writingLaser,true);
                                execL=true;
                            }
                            po->add(T, X-oX, (X-oX)/T, 0, 0, 0, 0,0,0);
                        }

                        oX=X; oY=Y;
                    }
                }
                po->addAction(XPS::writingLaser,false);
                if(go.pXPS->verifyPVTobj(po).retval!=0) {std::cout<<"retval was"<<go.pXPS->verifyPVTobj(po).retstr<<"\n";return;}
                go.pXPS->execPVTobj(po, &ret);
                std::cout<<X<<" "<<Y<<" "<<T<<"\n";
                ret.block_till_done();
                po->clear();
            }

            datafile.close();
        }
    }

    go.pXPS->setGPIO(XPS::iuScopeLED,true);
    go.pXPS->setGPIO(XPS::writingLaser,false);

    done=true;
    end=true;
}

