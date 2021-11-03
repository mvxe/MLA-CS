#include "GUI/mainwindow.h"
#include "ui_mainwindow.h"
#include "includes.h"

void MainWindow::sync_settings(){
    ui->sl_util_expo->blockSignals(true);

    ui->e_rpty_ip->setText(QString::fromStdString(go.pRPTY->IP.get()));
    ui->e_rpty_port->setValue(go.pRPTY->port.get());
    ui->e_rpty_timeout->setValue(go.pRPTY->keepalive.get());

    ui->sl_util_expo->setValue(go.pGCAM->utilCam->expo.get()*1000);
    ui->lab_util_expo->setText(QString::fromStdString(util::toString("Exposure: ",go.pGCAM->utilCam->expo.get()," us")));

    if (!go.pGCAM->iuScope->selected_ID.get().empty()) ui->cam1_select->setText(QString::fromStdString("camera ID: "+go.pGCAM->iuScope->selected_ID.get()));
    if (!go.pGCAM->utilCam->selected_ID.get().empty()) ui->cam2_select->setText(QString::fromStdString("camera ID: "+go.pGCAM->utilCam->selected_ID.get()));
    if (!go.pCNC->selected_ID.get().empty()) ui->cnc_select->setText(QString::fromStdString("serial ID: "+go.pCNC->selected_ID.get()));
    ui->sl_util_expo->blockSignals(false);
}



void MainWindow::on_e_rpty_ip_editingFinished()     {lineedit_fun(ui->e_rpty_ip,&go.pRPTY->IP);}
void MainWindow::on_e_rpty_port_editingFinished()   {spinbox_fun(ui->e_rpty_port,&go.pRPTY->port);}
void MainWindow::on_e_rpty_timeout_editingFinished(){spinbox_fun(ui->e_rpty_timeout,&go.pRPTY->keepalive);}

void MainWindow::on_sl_util_expo_valueChanged(int value){
    if(go.pGCAM->utilCam->connected){
        go.pGCAM->utilCam->set("ExposureTime",value/1000.);
        go.pGCAM->utilCam->expo.set(go.pGCAM->utilCam->get_dbl("ExposureTime"));
    }
}
int N=0;
void MainWindow::GUI_update(){
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
        ui->rpty_rst_btn->setEnabled(rpty_con);
    }
    if (cnc_con!=go.pCNC->connected){
        cnc_con=go.pCNC->connected;
        ui->si_stepCtrl->setPixmap(cnc_con?px_online:px_offline);
    }
    if (go.pRPTY->IP.resolved.changed())
        ui->e_rpty_ip_resolved->setText(QString::fromStdString(go.pRPTY->IP.is_name?go.pRPTY->IP.resolved.get():" "));

    if(go.pGCAM->utilCam->expo.changed())
        ui->lab_util_expo->setText(QString::fromStdString(util::toString("Exposure: ",go.pGCAM->utilCam->expo.get()," us")));

    const cv::Mat* dmat=dmat=utilCam_img->getUserMat();
    if (dmat!=nullptr){
        double aspect=(double)dmat->rows/dmat->cols;
        cv::Size dsize(ui->utilcam_stream->width(),aspect*ui->utilcam_stream->width());
        ui->utilcam_stream->setFixedHeight(aspect*ui->utilcam_stream->width());
        cv::Mat tmp;
        cv::resize(*dmat, tmp, dsize, 0, 0, cv::INTER_AREA);
        ui->utilcam_stream->setPixmap(QPixmap::fromImage(QImage(tmp.data, tmp.cols, tmp.rows, tmp.step, QImage::Format_Indexed8)));
        utilCam_img->freeUserMat();
    }

    if(matlk.try_lock()){
        if(mats->size()!=expsize) matsbar=true;
        if(matsbar){
            ui->progressBar->setMaximum(expsize);
            ui->progressBar->setFormat("Acquisition: %p%");
            ui->progressBar->setValue(mats->size());
        }
        ui->label_26->setText(QString::fromStdString(util::toString("Frames: ",mats->size(),"/",expsize)));
        matlk.unlock();
    }

}
