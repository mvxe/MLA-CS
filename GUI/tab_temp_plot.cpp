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
        lambda->setValue(565.);
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
    *imgMat=cv::imread(fileName.toStdString(), CV_LOAD_IMAGE_ANYDEPTH);
    if(imgMat->data){
        if(imgMat->depth()!=CV_8U) imgMat->convertTo(*imgMat8bit, CV_8U, 1/256.);
        else imgMat->copyTo(*imgMat8bit);
        imgLabel->setPixmap(QPixmap::fromImage(QImage(imgMat8bit->data, imgMat8bit->cols, imgMat8bit->rows, imgMat8bit->step, QImage::Format_Indexed8)));
        sizeLabel->setText(util::toString("Horizontal: ",imgMat->cols*pmw->xps_x_sen/100," um\nVertical: ",imgMat->rows*pmw->xps_y_sen/100," um").c_str());
        imgMat->convertTo(*imgMat, CV_32F, 2*M_PI/(1<<16), -M_PI);

        replot->setEnabled(false);
        save->setEnabled(false);
    }
}

void tab_temp_plot::on_replot_clicked(){
    if(hasSmth) plot();
}

void tab_temp_plot::on_save_clicked(){
    QString fileName = QFileDialog::getSaveFileName(this,tr("Text"), "",tr("Text (*.txt *.dat *.csv)"));
    if(fileName.isEmpty()) return;
    fsave(fileName.toStdString());
}

void tab_temp_plot::redraw(){
    cv::cvtColor(*imgMat8bit, *imgMatWLines, cv::COLOR_GRAY2BGR);
    if(isRight==false) cv::line(*imgMatWLines, cv::Point(X1,Y1), cv::Point(X2,Y2), {0,255,0});
    else cv::rectangle(*imgMatWLines, cv::Point(X1,Y1), cv::Point(X2,Y2), {255,0,0});
    imgLabel->setPixmap(QPixmap::fromImage(QImage(imgMatWLines->data, imgMatWLines->cols, imgMatWLines->rows, imgMatWLines->step, QImage::Format_RGB888)));
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
//            a.POUT<<"unset xlabel\n";
//            a.POUT<<"set format x ''\n";
//            a.POUT<<"unset ylabel\n";
//            a.POUT<<"set format y ''\n";
//            a.POUT<<"unset zlabel\n";
//            a.POUT<<"set format z ''\n";
//            a.POUT<<"set format cb ''\n";
//            a.POUT<<"unset cblabel\n";
        a.POUT<<"set view ,,"<<scale3D->value()<<"\n";
        //a.POUT<<"splot \"-\" using 1:2:3 w lines palette pt 6 notitle\n";
        a.POUT<<"splot \"-\" using 1:2:3 w pm3d pt 6 notitle\n";
        for(int i=(X1<X2)?X1:X2;i<=((X1<X2)?X2:X1);i++){
            for(int j=(Y1<Y2)?Y1:Y2;j<=((Y1<Y2)?Y2:Y1);j++)
                a.POUT<<i*pmw->xps_x_sen/100<<" "<<j*pmw->xps_x_sen/100<<" "<<(inv?-1:1)*imgMat->at<float>(j,i)/2/M_PI*_lambda/2/(eqXYZ?1000:1)<<"\n";
            a.POUT<<"\n";
        }

        a.POUT<<"e\n";
        a.POUT.flush();
//        std::string as;while(1){
//        a.PERR>>as;
//        std::cerr<<as<<" ";}
//        std::this_thread::sleep_for (std::chrono::seconds(10)); //TODO if gnuplot closes, the graph cannot be exported as PDF. FIX this
    }else{         //line
        double len;
        len=sqrt(pow(X2-X1,2)+pow(Y2-Y1,2));
        std::cerr<<"len= "<<len<<"\n";
        if((int)len<=0) return;
        double *lineData=new double[(int)len+1]();
        for(int i=0;i<=len;i++){
            double _X=X2+(X1-X2)*(len-i+0.01)/len;  //the 0.01 fixes a weird floor/ceil bug
            double _Y=Y2+(Y1-Y2)*(len-i+0.01)/len;
            lineData[i]=(1-(_X-floor(_X)))*(1-(_Y-floor(_Y)))*imgMat->at<float>(floor(_Y),floor(_X))+
                        (1-(ceil(_X) -_X))*(1-(ceil(_Y) -_Y))*imgMat->at<float>( ceil(_Y), ceil(_X))+
                        (1-(_X-floor(_X)))*(1-(ceil(_Y) -_Y))*imgMat->at<float>( ceil(_Y),floor(_X))+
                        (1-(ceil(_X) -_X))*(1-(_Y-floor(_Y)))*imgMat->at<float>(floor(_Y), ceil(_X));
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

        gnuplot a;
        a.POUT<<"set terminal wxt enhanced **persist**\n";
        a.POUT<<"set tics font \"Helvetica,"<<fontSB->value()<<"\"\n";
        a.POUT<<"set xlabel font \"Helvetica,"<<fontSB->value()<<"\"\n";
        a.POUT<<"set ylabel font \"Helvetica,"<<fontSB->value()<<"\"\n";
        a.POUT<<"set xlabel \"Position / um\"\n";
        a.POUT<<"set ylabel \"Height / nm\"\n";
        a.POUT<<"set xrange [0:"<<len*scale<<"]\n";
        a.POUT<<"plot \"-\" using 1:2 w lp lt 6 notitle\n";
        for(int i=0;i<=len;i++)
            a.POUT<<i*scale<<" "<<lineData[i]<<"\n";
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
        for(int i=(X1<X2)?X1:X2;i<=((X1<X2)?X2:X1);i++){
            for(int j=(Y1<Y2)?Y1:Y2;j<=((Y1<Y2)?Y2:Y1);j++)
                ofile<<i*pmw->xps_x_sen/100<<" "<<j*pmw->xps_x_sen/100<<" "<<(inv?-1:1)*imgMat->at<float>(j,i)/2/M_PI*_lambda/2<<"\n";
            ofile<<"\n";
        }
    }else{  //TODO: the first part is literarily copy paste from above
        double len;
        len=sqrt(pow(X2-X1,2)+pow(Y2-Y1,2));
        if((int)len<=0) return;
        double *lineData=new double[(int)len+1]();
        for(int i=0;i<=len;i++){
            double _X=X2+(X1-X2)*(len-i+0.01)/len;  //the 0.01 fixes a weird floor/ceil bug
            double _Y=Y2+(Y1-Y2)*(len-i+0.01)/len;
            lineData[i]=(1-(_X-floor(_X)))*(1-(_Y-floor(_Y)))*imgMat->at<float>(floor(_Y),floor(_X))+
                        (1-(ceil(_X) -_X))*(1-(ceil(_Y) -_Y))*imgMat->at<float>( ceil(_Y), ceil(_X))+
                        (1-(_X-floor(_X)))*(1-(ceil(_Y) -_Y))*imgMat->at<float>( ceil(_Y),floor(_X))+
                        (1-(ceil(_X) -_X))*(1-(_Y-floor(_Y)))*imgMat->at<float>(floor(_Y), ceil(_X));
        }
        if(inv) for(int i=0;i<=len;i++) lineData[i]*=-1;
        double minval=1e10;
        for(int i=0;i<=len;i++){
            lineData[i]*=_lambda/2/2/M_PI;
            if(lineData[i]<minval) minval=lineData[i];
        }
        for(int i=0;i<=len;i++) lineData[i]-=minval;
        double scale=( pow(X2-X1,2)/pow(len,2)*pmw->xps_x_sen+ pow(Y2-Y1,2)/pow(len,2)*pmw->xps_y_sen)/100;        //in um

        for(int i=0;i<=len;i++)
            ofile<<i*scale<<" "<<lineData[i]<<"\n";
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
