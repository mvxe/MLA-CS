////NOTE: this is not used anymore! left as code example

//#include "burn_array.h"
//#include "includes.h"
//#include "GUI/tab_monitor.h"    //for debug purposes

//pBurnArray::pBurnArray(double espacing, double eexp_fst, int eint_fst, double eexp_lst, int eint_lst, int gridX, int gridY, bool vac):
//        spacing(espacing), exp_fst(eexp_fst), int_fst(eint_fst), exp_lst(eexp_lst), int_lst(eint_lst), gridX(gridX), gridY(gridY), vac(vac),filename(""),useLines(false){
//    spacing/=1000;
//    exp_fst/=1000;
//    exp_lst/=1000;
//}
//pBurnArray::pBurnArray(std::string filename, bool useLines, double expo_mult, double expo_time):spacing(0), exp_fst(0), exp_lst(0), gridX(0), gridY(0), vac(0),filename(filename),useLines(useLines),expo_mult(expo_mult),expTime(expo_time/1000.){}
//pBurnArray::~pBurnArray(){
//}
//void pBurnArray::run(){
//    const double SMT=0.06;//0.04;
//    double SMTT;
//    if(!go.pRPTY->connected) return;

//    go.pXPS->setGPIO(XPS::iuScopeLED,true);
//    go.pXPS->setGPIO(XPS::writingLaser,false);

//    //XPS checks events every one servo cycle == 100us

//    po = go.pXPS->createNewPVTobj(XPS::mgroup_XYZF, "pBurnArray.txt");
//    std::vector<uint32_t> commands;
//    if(filename.empty()){
////        //this part is for laser measurement test
////        const int recQueue=0;                                               //TODO remove
////        const int cmdQueue=0;
////        uchar selectedavg=0;                                                //TODO remove
////        int domax=go.pRPTY->getNum(RPTY::A2F_RSMax,recQueue)*0.99;          //TODO remove
////        double maxpulsedur=std::max(exp_lst,exp_fst)*1.1;                      //TODO remove
////        while(domax*(8e-9)*(1<<selectedavg)<maxpulsedur) selectedavg++;        //TODO remove
////        for (int i=0;i!=gridX*gridY;i++){
////            int sp=(((double)int_fst+(int_lst-int_fst)*i/(gridX*gridY-1)));
////            double pd=((exp_fst+(exp_lst-exp_fst)*i/(gridX*gridY-1)));
////            commands.push_back(CQF::W4TRIG_INTR());
////            commands.push_back(CQF::WAIT((maxpulsedur-pd)/8e-9));
////            commands.push_back(CQF::ACK(1<<recQueue, selectedavg, CQF::fADC_A__fADC_B, true));      //TODO remove
////            commands.push_back(CQF::SG_SAMPLE(CQF::O0td, sp, 0));
////            commands.push_back(CQF::WAIT(pd/8e-9 - 1));
////            commands.push_back(CQF::SG_SAMPLE(CQF::O0td, 0, 0));
////            commands.push_back(CQF::WAIT(0.1*maxpulsedur/8e-9 - 1));
////            commands.push_back(CQF::ACK(1<<recQueue, selectedavg, CQF::fADC_A__fADC_B, false));
////            go.pRPTY->A2F_write(cmdQueue,commands.data(),commands.size());
////            go.pRPTY->trig(1<<cmdQueue);
////            commands.clear();

////            do std::this_thread::sleep_for (std::chrono::milliseconds(5000));
////            while(go.pRPTY->getNum(RPTY::A2F_RSCur,cmdQueue)!=0);

////            uint32_t toread=go.pRPTY->getNum(RPTY::F2A_RSCur,recQueue);
////            std::vector<uint32_t> read;
////            read.reserve(toread);
////            go.pRPTY->F2A_read(recQueue,read.data(),toread);
////            std::ofstream wfile(util::toString("meas4ben/",sp,".dat"));
////            for(int j=0; j!=toread; j++){
////                wfile<<j*(1<<selectedavg)*8e-9<<" "<<AQF::getChMSB(read[j])<<" "<<AQF::getChLSB(read[j])<<"\n";
////            }
////            wfile.close();
////            std::cerr<<"did i="<<i<<" sp: "<<sp<<"\n";
////        }

////        go.pXPS->setGPIO(XPS::iuScopeLED,true);
////        go.pXPS->setGPIO(XPS::writingLaser,false);
////        done=true;
////        end=true;
////        return;        //TODO remove the block up to here



//        commands.push_back(CQF::GPIO_MASK(0x40,0,0));
//        commands.push_back(CQF::GPIO_DIR (0x40,0,0));
//        commands.push_back(CQF::W4TRIG_GPIO(CQF::HIGH,false,0x40,0x00));
//        po->addAction(XPS::writingLaser,true);
//        for (int j=0;j!=gridY;j++){
//            for (int i=0;i!=gridX;i++){
//                po->add(SMT, spacing, 0, 0, 0, 0, 0,0,0);
//                commands.push_back(CQF::WAIT(SMT/8e-9));
//                if(!(vac && (j%2) && (i%2))){
//                    commands.push_back(CQF::TRIG_OTHER(1<<tab_monitor::RPTY_A2F_queue));    //for debugging purposes
//                    commands.push_back(CQF::SG_SAMPLE(CQF::O0td, (((double)int_fst+(int_lst-int_fst)*(i+j*(gridX-1))/gridY/(gridX-1))), 0));
//                    //printf("%lf\n",(((double)int_fst+(int_lst-int_fst)*(i+j*(gridX-1))/gridY/(gridX-1))));
//                    commands.push_back(CQF::WAIT(((exp_fst+(exp_lst-exp_fst)*(i+j*(gridX-1))/gridY/(gridX-1)))/8e-9 - 3));
//                    commands.push_back(CQF::SG_SAMPLE(CQF::O0td, 0, 0));
//                    po->add(((exp_fst+(exp_lst-exp_fst)*(i+j*(gridX-1))/gridY/(gridX-1))), 0, 0, 0, 0, 0, 0,0,0);
//                }
//            }
//            if(end) break;
//            po->add(SMT*gridX, -gridX*spacing, 0, spacing, 0, 0, 0,0,0);
//            commands.push_back(CQF::WAIT(SMT*gridX/8e-9));
//        }

//        printf("used %d of %d commands\n",commands.size(), go.pRPTY->getNum(RPTY::A2F_RSMax,0));
//        go.pRPTY->A2F_write(0,commands.data(),commands.size());
//        commands.clear();
//        if(go.pXPS->verifyPVTobj(po).retval!=0) {std::cout<<"retval was"<<go.pXPS->verifyPVTobj(po).retstr<<"\n";return;}
//        go.pXPS->execPVTobj(po, &ret);
//        ret.block_till_done();
//        po->clear();
//        go.pXPS->setGPIO(XPS::writingLaser,false);

//    }else{
//        std::string line;
//        std::ifstream datafile(filename);
//        double X,Y,T,I;
//        double oX=0, oY=0;
//        double dis;
//        int domax=go.pRPTY->getNum(RPTY::A2F_RSMax,0);
//        int32_t atElement=0;

//        std::string::size_type sza,szb;
//        if (datafile.is_open()){

//            if(!useLines){
//                commands.push_back(CQF::GPIO_MASK(0x40,0,0));
//                commands.push_back(CQF::GPIO_DIR (0x40,0,0));
//                commands.push_back(CQF::W4TRIG_GPIO(CQF::HIGH,false,0x40,0x00));
//                po->addAction(XPS::writingLaser,true);
//                while(getline(datafile,line)){
//                    if(end) break;
//                    if(line.size()<2 || line.find("#")!=std::string::npos);
//                    else{
//                        X=std::stod(line,&sza)/1000;
//                        Y=std::stod(line.substr(sza),&szb)/1000;
//                        I=std::stoi(line.substr(sza+szb));
//                        dis=sqrt(pow(X-oX,2)+pow(Y-oY,2));

//                        po->add(dis*SMT*100, X-oX, 0, Y-oY, 0, 0, 0,0,0);
//                        commands.push_back(CQF::WAIT(dis*SMT*100/8e-9));
//                        commands.push_back(CQF::TRIG_OTHER(1<<tab_monitor::RPTY_A2F_queue));    //for debugging purposes
//                        commands.push_back(CQF::SG_SAMPLE(CQF::O0td, I, 0));
//                        commands.push_back(CQF::WAIT(expTime/8e-9 - 3));
//                        commands.push_back(CQF::SG_SAMPLE(CQF::O0td, 0, 0));
//                        po->add(expTime, 0, 0, 0, 0, 0, 0,0,0);
//                        oX=X; oY=Y;
//                    }
//                }

//                if(go.pXPS->verifyPVTobj(po).retval!=0) {std::cout<<"retval was"<<go.pXPS->verifyPVTobj(po).retstr<<"\n";return;}
//                if(commands.size()<domax) domax=commands.size();
//                go.pRPTY->A2F_write(0,commands.data(),domax);
//                atElement+=domax;

//                go.pXPS->execPVTobj(po, &ret);
//                while(atElement<commands.size()){
//                    int num=go.pRPTY->getNum(RPTY::A2F_RSCur,0);
//                    if(num<domax/2){
//                        go.pRPTY->A2F_write(0,commands.data()+atElement,(domax/2<commands.size()-atElement)?(domax/2):(commands.size()-atElement));
//                        atElement+=domax/2;
//                    }
//                    if(num<domax/8) printf("warning: writing to RPTYa might be too slow: num=%d \n",num);
//                    else std::this_thread::sleep_for (std::chrono::microseconds(100));
//                }
//                ret.block_till_done();
//                po->clear();
//                commands.clear();

//            }else{
//                commands.push_back(CQF::GPIO_MASK(0x40,0,0));
//                commands.push_back(CQF::GPIO_DIR (0x40,0,0));
//                commands.push_back(CQF::W4TRIG_GPIO(CQF::HIGH,false,0x40,0x00));
//                po->addAction(XPS::writingLaser,true);
//                while(getline(datafile,line)){
//                    if(end) break;
//                    if(line.size()<2 || line.find("#")!=std::string::npos);
//                    else{
//                        X=std::stod(line,&sza)/1000;
//                        Y=std::stod(line.substr(sza),&szb)/1000;
//                        I=std::stoi(line.substr(sza+szb));
//                        dis=sqrt(pow(X-oX,2)+pow(Y-oY,2));

//                        if((expTime/8e-9 - 3)>dis*SMT*100/8e-9) SMTT=(expTime/8e-9 - 3)/(dis*100/8e-9);
//                        else SMTT=SMT;
//                        po->add(dis*SMTT*100, X-oX, 0, Y-oY, 0, 0, 0,0,0);
//                        commands.push_back(CQF::WAIT(dis*SMTT*100/8e-9 - (expTime/8e-9 - 3)));
//                        commands.push_back(CQF::TRIG_OTHER(1<<tab_monitor::RPTY_A2F_queue));    //for debugging purposes
//                        commands.push_back(CQF::SG_SAMPLE(CQF::O0td, I, 0));
//                        commands.push_back(CQF::WAIT(expTime/8e-9 - 3));
//                        commands.push_back(CQF::SG_SAMPLE(CQF::O0td, 0, 0));
//                        oX=X; oY=Y;
//                    }
//                }

//                if(go.pXPS->verifyPVTobj(po).retval!=0) {std::cout<<"retval was"<<go.pXPS->verifyPVTobj(po).retstr<<"\n";return;}
//                if(commands.size()<domax) domax=commands.size();
//                go.pRPTY->A2F_write(0,commands.data(),domax);
//                atElement+=domax;
//                go.pXPS->execPVTobj(po, &ret);
//                while(atElement<commands.size()){
//                    int num=go.pRPTY->getNum(RPTY::A2F_RSCur,0);
//                    if(num<domax/2){
//                        go.pRPTY->A2F_write(0,commands.data()+atElement,(domax/2<commands.size()-atElement)?(domax/2):(commands.size()-atElement));
//                        atElement+=domax/2;
//                    }
//                    if(num<domax/8) printf("warning: writing to RPTYa might be too slow: num=%d \n",num);
//                    else std::this_thread::sleep_for (std::chrono::microseconds(100));
//                }
//                ret.block_till_done();
//                po->clear();
//                commands.clear();
//            }

//            datafile.close();
//        }
//    }

//    go.pXPS->setGPIO(XPS::iuScopeLED,true);
//    go.pXPS->setGPIO(XPS::writingLaser,false);
////    printf("A2F_RSMax for queue %d is %d\n",0,go.pRPTY->getNum(RPTY::A2F_RSMax,0));
////    printf("A2F_RSCur for queue %d is %d\n",0,go.pRPTY->getNum(RPTY::A2F_RSCur,0));
////    printf("A2F_lostN for queue %d is %d\n",0,go.pRPTY->getNum(RPTY::A2F_lostN,0));
//    done=true;
//    end=true;
//}

