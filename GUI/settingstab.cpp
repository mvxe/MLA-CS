#include "GUI/mainwindow.h"
#include "ui_mainwindow.h"
#include "includes.h"


void MainWindow::sync_settings(){
    ui->sl_expo->blockSignals(true);
    ui->sl_util_expo->blockSignals(true);
    ui->e_xps_ip->setText(QString::fromStdString(go.pXPS->IP.get()));
    ui->e_xps_port->setValue(go.pXPS->port.get());
    ui->e_xps_xyz->setText(QString::fromStdString(go.pXPS->groupGetName(XPS::mgroup_XYZ)));
    ui->e_xps_timeout->setValue(go.pXPS->keepalive.get());

    ui->e_rpty_ip->setText(QString::fromStdString(go.pRPTY->IP.get()));
    ui->e_rpty_port->setValue(go.pRPTY->port.get());
    ui->e_rpty_timeout->setValue(go.pRPTY->keepalive.get());

//    ui->sl_xsens->setValue(xps_x_sen);
//    ui->sl_ysens->setValue(xps_y_sen);
//    ui->sl_zsens->setValue(xps_z_sen);
    ui->sl_expo->setValue(go.pGCAM->iuScope->expo.get()*1000);
    ui->lab_expo->setText(QString::fromStdString(util::toString("Exposure: ",go.pGCAM->iuScope->expo.get()," us")));

    ui->sl_util_expo->setValue(go.pGCAM->utilCam->expo.get()*1000);
    ui->lab_util_expo->setText(QString::fromStdString(util::toString("Exposure: ",go.pGCAM->utilCam->expo.get()," us")));

    if (!go.pGCAM->iuScope->selected_ID.get().empty()) ui->cam1_select->setText(QString::fromStdString("camera ID: "+go.pGCAM->iuScope->selected_ID.get()));
    if (!go.pGCAM->utilCam->selected_ID.get().empty()) ui->cam2_select->setText(QString::fromStdString("camera ID: "+go.pGCAM->utilCam->selected_ID.get()));
    ui->sl_expo->blockSignals(false);
    ui->sl_util_expo->blockSignals(false);
}


void MainWindow::on_e_xps_ip_editingFinished()      {lineedit_fun(ui->e_xps_ip,&go.pXPS->IP);}
void MainWindow::on_e_xps_port_editingFinished()    {spinbox_fun(ui->e_xps_port,&go.pXPS->port);}
void MainWindow::on_e_xps_xyz_editingFinished()     {ui->e_xps_xyz->blockSignals(true); //this prevents two signals to emmit when pressing enter (a QT bug workaround)
                                                     go.pXPS->groupSetName(XPS::mgroup_XYZ, ui->e_xps_xyz->text().toStdString());
                                                     ui->e_xps_xyz->clearFocus();
                                                     ui->e_xps_xyz->blockSignals(false);}
void MainWindow::on_e_xps_timeout_editingFinished() {spinbox_fun(ui->e_xps_timeout,&go.pXPS->keepalive);}

void MainWindow::on_e_rpty_ip_editingFinished()     {lineedit_fun(ui->e_rpty_ip,&go.pRPTY->IP);}
void MainWindow::on_e_rpty_port_editingFinished()   {spinbox_fun(ui->e_rpty_port,&go.pRPTY->port);}
void MainWindow::on_e_rpty_timeout_editingFinished(){spinbox_fun(ui->e_rpty_timeout,&go.pRPTY->keepalive);}

void MainWindow::on_sl_xsens_valueChanged(int value){xps_x_sen=value;}
void MainWindow::on_sl_ysens_valueChanged(int value){xps_y_sen=value;}
void MainWindow::on_sl_zsens_valueChanged(int value){xps_z_sen=value;}
void MainWindow::on_sl_expo_valueChanged(int value) {
    if(go.pGCAM->iuScope->connected){
        go.pGCAM->iuScope->set("ExposureTime",value/1000.);
        go.pGCAM->iuScope->expo.set(go.pGCAM->iuScope->get_dbl("ExposureTime"));
    }
}
void MainWindow::on_sl_util_expo_valueChanged(int value){
    if(go.pGCAM->utilCam->connected){
        go.pGCAM->utilCam->set("ExposureTime",value/1000.);
        go.pGCAM->utilCam->expo.set(go.pGCAM->utilCam->get_dbl("ExposureTime"));
    }
}
int N=0;
void MainWindow::GUI_update(){
    if (xps_con!=go.pXPS->connected){
        xps_con=go.pXPS->connected;
        ui->si_XPS->setPixmap(xps_con?px_online:px_offline);
    }
    if (iuScope_con!=go.pGCAM->iuScope->connected){
        iuScope_con=go.pGCAM->iuScope->connected;
        ui->si_iuScope->setPixmap(iuScope_con?px_online:px_offline);
    }
    if (utilCam_con!=go.pGCAM->utilCam->connected){
        utilCam_con=go.pGCAM->utilCam->connected;
        ui->si_utilCam->setPixmap(utilCam_con?px_online:px_offline);
    }
    if (rpty_con!=go.pRPTY->connected){
        rpty_con=go.pRPTY->connected;
        ui->si_RPTY->setPixmap(rpty_con?px_online:px_offline);
    }

    if (go.pXPS->IP.resolved.changed())
        ui->e_xps_ip_resolved->setText(QString::fromStdString(go.pXPS->IP.is_name?go.pXPS->IP.resolved.get():" "));
    if (go.pRPTY->IP.resolved.changed())
        ui->e_rpty_ip_resolved->setText(QString::fromStdString(go.pRPTY->IP.is_name?go.pRPTY->IP.resolved.get():" "));

    if(go.pGCAM->iuScope->expo.changed())
        ui->lab_expo->setText(QString::fromStdString(util::toString("Exposure: ",go.pGCAM->iuScope->expo.get()," us")));
    if(go.pGCAM->utilCam->expo.changed())
        ui->lab_util_expo->setText(QString::fromStdString(util::toString("Exposure: ",go.pGCAM->utilCam->expo.get()," us")));

    const cv::Mat* dmat=iuScope_img->getUserMat();
    if (dmat!=nullptr){
        double aspect=(double)dmat->rows/dmat->cols;
        cv::Size dsize(ui->camera_stream->width(),aspect*ui->camera_stream->width());
        ui->camera_stream->setFixedHeight(aspect*ui->camera_stream->width());
        cv::resize(*dmat, *onDisplay, dsize, 0, 0, cv::INTER_AREA);
        ui->camera_stream->setPixmap(QPixmap::fromImage(QImage(onDisplay->data, onDisplay->cols, onDisplay->rows, onDisplay->step, QImage::Format_Indexed8)));
                //if we wanted to make an actual copy, we'd use QImage(dmat->data, dmat->cols, dmat->rows, dmat->step, QImage::Format_Indexed8).copy()
                //but here resize copies into onDisplay
        //std::cerr<<"timestamp: "<<iuScope_img->getUserTimestamp()<<"\n";
        iuScope_img->freeUserMat();
    }
    dmat=utilCam_img->getUserMat();
    if (dmat!=nullptr){
        double aspect=(double)dmat->rows/dmat->cols;
        cv::Size dsize(ui->utilcam_stream->width(),aspect*ui->utilcam_stream->width());
        ui->utilcam_stream->setFixedHeight(aspect*ui->utilcam_stream->width());
        cv::resize(*dmat, *onDisplay, dsize, 0, 0, cv::INTER_AREA);
        ui->utilcam_stream->setPixmap(QPixmap::fromImage(QImage(onDisplay->data, onDisplay->cols, onDisplay->rows, onDisplay->step, QImage::Format_Indexed8)));
        utilCam_img->freeUserMat();
    }


    if (disable.get()) {
        if (ui->centralWidget->isEnabled())
            ui->centralWidget->setEnabled(false);
    }
    else if (!ui->centralWidget->isEnabled())
        ui->centralWidget->setEnabled(true);

    donth++;
    if(donth==10){
        donth=0;
        if(go.pXPS->connected){ if(go.pXPS->isQueueEmpty()){
            XPS::raxis ret = go.pXPS->getPos(XPS::mgroup_XYZ);
            ui->lbl_position->setText(QString::fromStdString(util::toString("Position: X=", ret.pos[0], "mm , Y=", ret.pos[1], "mm , Z=", ret.pos[2], "mm")));
        }}
        else ui->lbl_position->setText(QString::fromStdString(util::toString("Position: NC")));
    }


}
