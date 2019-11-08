#include "tab_temp_plot.h"
#include "opencv2/opencv.hpp"
#include "mainwindow.h"
#include "UTIL/pipe.h"

tab_temp_plot::tab_temp_plot(QWidget* parent){
    imgMat=new cv::Mat;
    imgMat8bit=new cv::Mat;
    imgMatWLines=new cv::Mat;
    layout=new QVBoxLayout;
    parent->setLayout(layout);

    btnLayout=new QHBoxLayout;
    btnWidget=new QWidget;
    btnWidget->setLayout(btnLayout);

        load_file=new QPushButton;
        load_file->setText("Load File");
        load_file->setIcon(QPixmap(":/edit-add.svg"));
        connect(load_file, SIGNAL(released()), this, SLOT(on_load_file_clicked()));
        btnLayout->addWidget(load_file);

        inverted=new QCheckBox;
        inverted->setText("Invert depth");
        btnLayout->addWidget(inverted);

        QLabel* lambdalab=new QLabel;
        lambdalab->setText("LED central wavelength (in nm):");
        btnLayout->addWidget(lambdalab);

        lambda=new QDoubleSpinBox;
        lambda->setMaximum(2000);
        lambda->setMinimum(100);
        lambda->setValue(470.);
        btnLayout->addWidget(lambda);

        QLabel* font=new QLabel;
        font->setText("Font size:");
        btnLayout->addWidget(font);

        fontSB=new QSpinBox;
        fontSB->setMinimum(6);
        fontSB->setMaximum(50);
        fontSB->setValue(12);
        btnLayout->addWidget(fontSB);

        equalXYZ=new QCheckBox;
        equalXYZ->setText("Equal XYZ scaling");
        btnLayout->addWidget(equalXYZ);

        scale3D=new QDoubleSpinBox;
        scale3D->setMaximum(10);
        scale3D->setMinimum(0.1);
        scale3D->setSingleStep(0.1);
        scale3D->setValue(1);
        btnLayout->addWidget(scale3D);

        replot=new QPushButton;
        replot->setText("Replot");
        replot->setIcon(QPixmap(":/gtk-refresh.svg"));
        connect(replot, SIGNAL(released()), this, SLOT(on_replot_clicked()));
        btnLayout->addWidget(replot);
        replot->setEnabled(false);

        save=new QPushButton;
        save->setText("Save raw");
        save->setIcon(QPixmap(":/down.svg"));
        connect(save, SIGNAL(released()), this, SLOT(on_save_clicked()));
        btnLayout->addWidget(save);
        save->setEnabled(false);

        scor=new QPushButton;
        scor->setText("Correct Slope");
        scor->setIcon(QPixmap(":/emblem-nowrite.svg"));
        connect(scor, SIGNAL(released()), this, SLOT(on_scor_clicked()));
        btnLayout->addWidget(scor);
        scor->setEnabled(false);

        do2DFFT=new QCheckBox;
        do2DFFT->setText("2DFFT");
        btnLayout->addWidget(do2DFFT);
        connect(do2DFFT, SIGNAL(released()), this, SLOT(redraw()));

    btnLayout->addStretch(0);
    layout->addWidget(btnWidget);

    imgLabel=new plotSelLabel;
    imgLabel->setMouseTracking(true);
    imgLabel->ttp=this;
    layout->addWidget(imgLabel);
    sizeLabel=new QLabel;
    layout->addWidget(sizeLabel);


    layout->addStretch(0);
}


void tab_temp_plot::on_load_file_clicked(){
    QString fileName = QFileDialog::getOpenFileName(this,tr("Image"), "",tr("Images (*.png *.xpm *.jpg)"));
    if(fileName.isEmpty()) return;
    *imgMat=cv::imread(fileName.toStdString(), cv::IMREAD_ANYDEPTH);
    if(imgMat->data){
        if(imgMat->depth()!=CV_8U) imgMat->convertTo(*imgMat8bit, CV_8U, 1/256.);
        else imgMat->copyTo(*imgMat8bit);
        imgLabel->setPixmap(QPixmap::fromImage(QImage(imgMat8bit->data, imgMat8bit->cols, imgMat8bit->rows, imgMat8bit->step, QImage::Format_Indexed8)));
        sizeLabel->setText(util::toString("Horizontal: ",imgMat->cols*pmw->xps_x_sen/100," um\nVertical: ",imgMat->rows*pmw->xps_y_sen/100," um").c_str());
        imgMat->convertTo(*imgMat, CV_32F, 2*M_PI/(1<<16), -M_PI);

        replot->setEnabled(false);
        save->setEnabled(false);

        scor->setIcon(QPixmap(":/emblem-nowrite.svg"));
        scor->setEnabled(true);
        planecor[0]=0;
        planecor[1]=0;
        planecor[2]=0;
    }
}

void tab_temp_plot::calcPlaneCor(){
    gnuplot a;
    a.POUT<<"f(x,y)=a*x+b*y+c\n";
    a.POUT<<"a=0.01\n";
    a.POUT<<"b=0.01\n";
    a.POUT<<"c=0.01\n";

    a.POUT<<"fit f(x,y) \"-\" using 1:2:3 via a,b,c\n";
    for(int i=0;i<imgMat->cols;i++){
        for(int j=0;j<imgMat->rows;j++)
            a.POUT<<i<<" "<<j<<" "<<imgMat->at<float>(j,i)<<"\n";
        a.POUT<<"\n";
    }
    a.POUT<<"e\n";
    a.POUT<<"print \"afdgdsfgdf\"\n";
    a.POUT.flush();
    char rep[256];
    for (int i=0;i!=100;i++){
           a.PERR.getline(rep,256);
           printf("%s",rep);
           if(std::string(rep).find("afdgdsfgdf")!=std::string::npos) break;
    }
    a.POUT<<"print a\n";a.POUT.flush();
    a.PERR.getline(rep,256); planecor[0] = atof(rep);
    std::cerr <<"\n" << planecor[0] <<"\n";
    a.POUT<<"print b\n";a.POUT.flush();
    a.PERR.getline(rep,256); planecor[1] = atof(rep);
    std::cerr <<"\n" << planecor[1] <<"\n";
    a.POUT<<"print c\n";a.POUT.flush();
    a.PERR.getline(rep,256); planecor[2] = atof(rep);
    std::cerr <<"\n" << planecor[2] <<"\n";
}

void tab_temp_plot::on_scor_clicked(){
    calcPlaneCor();
    scor->setIcon(QPixmap(":/emblem-ok.svg"));
}

double tab_temp_plot::corr(int i, int j){
    return planecor[0]*i+planecor[1]*j+planecor[2];
}

void tab_temp_plot::on_replot_clicked(){
    if(hasSmth) plot();
}

void tab_temp_plot::on_save_clicked(){
    QString fileName = QFileDialog::getSaveFileName(this,tr("Text"), "",tr("Text (*.txt *.dat *.csv)"));
    if(fileName.isEmpty()) return;
    fsave(fileName.toStdString());
}

struct cvect{
    float val;
    int pos;
    bool deleted=false;
};
bool compfun (cvect i,cvect j) { return (i.val>j.val); }

void tab_temp_plot::redraw(){
    if(do2DFFT->isChecked()){
        cv::Mat padded;
        int m = cv::getOptimalDFTSize( imgMat->rows );
        int n = cv::getOptimalDFTSize( imgMat->cols ); // on the border add zero values
        cv::copyMakeBorder(*imgMat, padded, 0, m - imgMat->rows, 0, n - imgMat->cols, cv::BORDER_CONSTANT, cv::Scalar::all(0));
        cv::Mat planes[] = {cv::Mat_<float>(padded), cv::Mat::zeros(padded.size(), CV_32F)};
        cv::Mat complexI;
        cv::merge(planes, 2, complexI);
        cv::dft(complexI, complexI);
        cv::split(complexI, planes);                   // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
      //cv::magnitude(planes[0], planes[1], planes[0]);// planes[0] = magnitude
        cv::phase(planes[0], planes[1], planes[0]);// planes[0] = phase
        cv::Mat magI = planes[0];
      //magI += cv::Scalar::all(1);                    // switch to logarithmic scale
      //cv::log(magI, magI);
        // crop the spectrum, if it has an odd number of rows or columns
        magI = magI(cv::Rect(0, 0, magI.cols & -2, magI.rows & -2));
        std::cout<<"phase at 0,1: "<<magI.at<float>(0,1)<<"\n";
        std::cout<<"phase at 1,0: "<<magI.at<float>(1,0)<<"\n";
        // rearrange the quadrants of Fourier image  so that the origin is at the image center
        int cx = magI.cols/2;
        int cy = magI.rows/2;

        cv::Mat q0(magI, cv::Rect(0, 0, cx, cy));   // Top-Left - Create a ROI per quadrant
        cv::Mat q1(magI, cv::Rect(cx, 0, cx, cy));  // Top-Right
        cv::Mat q2(magI, cv::Rect(0, cy, cx, cy));  // Bottom-Left
        cv::Mat q3(magI, cv::Rect(cx, cy, cx, cy)); // Bottom-Right

        cv::Mat tmp;                           // swap quadrants (Top-Left with Bottom-Right)
        q0.copyTo(tmp);
        q3.copyTo(q0);
        tmp.copyTo(q3);

        q1.copyTo(tmp);                    // swap quadrant (Top-Right with Bottom-Left)
        q2.copyTo(q1);
        tmp.copyTo(q2);


        cv::Mat hVec;
        reduce(magI, hVec, 0, cv::REDUCE_AVG);
        std::vector<cvect> hStdVec;
        hStdVec.reserve(hVec.cols);
        for(int i=0;i!=hVec.cols;i++){
            hStdVec.emplace_back();
            hStdVec.back().val=(float)hVec.at<float>(0, i);
            hStdVec.back().pos=i;
        }

        normalize(magI, magI, 0, 255, cv::NORM_MINMAX); // Transform the matrix with float values into a
                                                // viewable image form (float between values 0 and 1).
        magI.convertTo(magI, CV_8U);

//        cv::threshold(magI, magI, 100, 255, cv::THRESH_BINARY);
        //cv::Canny(magI, magI,  100, 200);
        cv::cvtColor(magI, *imgMatWLines, cv::COLOR_GRAY2BGR);





//        std::vector<cvect> peaks;
//        cvect *max = &(*std::max_element(hStdVec.begin(),hStdVec.end(),compfun));
//        if(max->deleted==false){
//            max->deleted=true;
//            peaks.push_back(*max);
//        }

//        for(int i=0;i!=hStdVec.size();i++){
//            printf("%f %d\n",hStdVec[i].val, hStdVec[i].pos);
//        }printf("\n");

//        std::sort (hStdVec.begin(), hStdVec.end(), compfun);
//        for(int i=0;i!=40;i++){
//            printf("%f %d\n",hStdVec[i].val, hStdVec[i].pos);
//        }printf("\n");


    }
    else cv::cvtColor(*imgMat8bit, *imgMatWLines, cv::COLOR_GRAY2BGR);

    if(isRight==false) cv::line(*imgMatWLines, cv::Point(X1,Y1), cv::Point(X2,Y2), {0,255,0});
    else cv::rectangle(*imgMatWLines, cv::Point(X1,Y1), cv::Point(X2,Y2), {255,0,0});
    imgLabel->setPixmap(QPixmap::fromImage(QImage(imgMatWLines->data, imgMatWLines->cols, imgMatWLines->rows, imgMatWLines->step, QImage::Format_RGB888)));
}

void tab_temp_plot::streamData(std::ostream *stream, bool is_line){
    bool inv=!inverted->isChecked();
    double _lambda=lambda->value();
    if(!is_line){
        for(int i=(X1<X2)?X1:X2;i<=((X1<X2)?X2:X1);i++){
            for(int j=(Y1<Y2)?Y1:Y2;j<=((Y1<Y2)?Y2:Y1);j++)
                *stream<<i*pmw->xps_x_sen/100<<" "<<j*pmw->xps_x_sen/100<<" "<<(inv?-1:1)*(imgMat->at<float>(j,i)-corr(i,j))/2/M_PI*_lambda/2<<"\n";
            *stream<<"\n";
        }
    }else{
        double len;
        len=sqrt(pow(X2-X1,2)+pow(Y2-Y1,2));
        std::cerr<<"len= "<<len<<"\n";
        if((int)len<=0) return;
        double *lineData=new double[(int)len+1]();
        for(int i=0;i<=len;i++){
            double _X=X2+(X1-X2)*(len-i+0.01)/len;  //the 0.01 fixes a weird floor/ceil bug
            double _Y=Y2+(Y1-Y2)*(len-i+0.01)/len;
            lineData[i]=(1-(_X-floor(_X)))*(1-(_Y-floor(_Y)))*(imgMat->at<float>(floor(_Y),floor(_X))-corr(floor(_X),floor(_Y)))+
                        (1-(ceil(_X) -_X))*(1-(ceil(_Y) -_Y))*(imgMat->at<float>( ceil(_Y), ceil(_X))-corr( ceil(_X), ceil(_Y)))+
                        (1-(_X-floor(_X)))*(1-(ceil(_Y) -_Y))*(imgMat->at<float>( ceil(_Y),floor(_X))-corr(floor(_X), ceil(_Y)))+
                        (1-(ceil(_X) -_X))*(1-(_Y-floor(_Y)))*(imgMat->at<float>(floor(_Y), ceil(_X))-corr( ceil(_X),floor(_Y)));
        }
        //std::cout<<"scaling X,Y= " << std::setprecision(6) <<pmw->xps_x_sen<<", "<<pmw->xps_y_sen<<"\n";
        if(inv) for(int i=0;i<=len;i++) lineData[i]*=-1;
        double minval=1e10;
        for(int i=0;i<=len;i++){
            lineData[i]*=_lambda/2/2/M_PI;
            if(lineData[i]<minval) minval=lineData[i];
        }
        for(int i=0;i<=len;i++) lineData[i]-=minval;
        double scale=( pow(X2-X1,2)/pow(len,2)*pmw->xps_x_sen+ pow(Y2-Y1,2)/pow(len,2)*pmw->xps_y_sen)/100;        //in um

        for(int i=0;i<=len;i++)
            *stream<<i*scale<<" "<<lineData[i]<<"\n";
    }
}

void tab_temp_plot::plot(){
    bool inv=!inverted->isChecked();
    double _lambda=lambda->value();
    bool eqXYZ=equalXYZ->isChecked();
    if(isRight){   //rectangle
        gnuplot a;
        a.POUT<<"set terminal wxt enhanced **persist**\n";
        a.POUT<<"set tics font \"Helvetica,"<<fontSB->value()<<"\"\n";
        a.POUT<<"set xlabel font \"Helvetica,"<<fontSB->value()<<"\"\n";
        a.POUT<<"set ylabel font \"Helvetica,"<<fontSB->value()<<"\"\n";
        a.POUT<<"set cblabel font \"Helvetica,"<<fontSB->value()<<"\"\n";
        a.POUT<<"set xlabel \"XPosition/ um\"\n";
        a.POUT<<"set ylabel \"YPosition/ um\"\n";
        if(eqXYZ) a.POUT<<"set cblabel \"Height / um\"\n";
        else      a.POUT<<"set cblabel \"Height / nm\"\n";
        if(eqXYZ) a.POUT<<"set view equal xyz\n";
        else      a.POUT<<"set view equal xy\n";
        if(eqXYZ) a.POUT<<"unset ztics\n";
        a.POUT<<"set xyplane 0\n";
        a.POUT<<"set hidden3d\n";
        a.POUT<<"set view ,,"<<scale3D->value()<<"\n";
        a.POUT<<"splot \"-\" using 1:2:3 w pm3d pt 6 notitle\n";
        streamData(&a.POUT, false);
        a.POUT<<"e\n";
        a.POUT.flush();
//        std::string as;while(1){
//        a.PERR>>as;
//        std::cerr<<as<<" ";}
//        std::this_thread::sleep_for (std::chrono::seconds(10)); //TODO if gnuplot closes, the graph cannot be exported as PDF. FIX this
    }else{         //line
        gnuplot a;
        a.POUT<<"set terminal wxt enhanced **persist**\n";
        a.POUT<<"set tics font \"Helvetica,"<<fontSB->value()<<"\"\n";
        a.POUT<<"set xlabel font \"Helvetica,"<<fontSB->value()<<"\"\n";
        a.POUT<<"set ylabel font \"Helvetica,"<<fontSB->value()<<"\"\n";
        a.POUT<<"set xlabel \"Position / um\"\n";
        a.POUT<<"set ylabel \"Height / nm\"\n";
        //a.POUT<<"set xrange [0:"<<len*scale<<"]\n";
        a.POUT<<"plot \"-\" using 1:2 w lp lt 6 notitle\n";
        streamData(&a.POUT, true);
        a.POUT<<"e\n";
        a.POUT.flush();
        //std::this_thread::sleep_for (std::chrono::seconds(10)); //TODO if gnuplot closes, the graph cannot be exported as PDF. FIX this

    }
}

void tab_temp_plot::fsave(std::string filename){
    std::ofstream ofile;
    ofile.open (filename);
    bool inv=!inverted->isChecked();
    double _lambda=lambda->value();
    if(isRight){   //rectangle
        streamData(&ofile, false);
    }else{  //TODO: the first part is literarily copy paste from above
        streamData(&ofile, true);
    }
    ofile.close();
}


void plotSelLabel::mouseMoveEvent(QMouseEvent *event){
    if(ttp->active){
        ttp->X2=event->pos().x();
        ttp->Y2=event->pos().y();
        ttp->redraw();
    }
}

void plotSelLabel::mousePressEvent(QMouseEvent *event){
    if (event->button()==Qt::RightButton) {
        ttp->isRight=true;
    }else ttp->isRight=false;
    ttp->X1=event->pos().x();
    ttp->Y1=event->pos().y();
    ttp->active=true;
}
void plotSelLabel::mouseReleaseEvent(QMouseEvent *event){
    ttp->X2=event->pos().x();
    ttp->Y2=event->pos().y();
    ttp->redraw();
    ttp->hasSmth=true;
    ttp->plot();
    ttp->active=false;
    ttp->replot->setEnabled(true);
    ttp->save->setEnabled(true);
}
