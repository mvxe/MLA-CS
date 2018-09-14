#include "mainwindow.h"
#include "ui_mainwindow.h"

void MainWindow::sync_settings(){
    ui->sl_expo->blockSignals(true);
    ui->e_xps_ip->setText(QString::fromStdString(go.pXPS->IP.get()));
    ui->e_xps_port->setValue(go.pXPS->port.get());
    ui->e_xps_xyz->setText(QString::fromStdString(go.pXPS->groupGetName(XPS::mgroup_XYZ)));
    ui->e_xps_timeout->setValue(go.pXPS->keepalive.get());

    ui->e_rpty_ip->setText(QString::fromStdString(sw.RPTY_ip.get()));
    ui->e_rpty_port->setValue(sw.RPTY_port.get());
    ui->e_rpty_timeout->setValue(sw.RPTY_keepalive.get());

    ui->sl_xsens->setValue(sw.xps_x_sen.get());
    ui->sl_ysens->setValue(sw.xps_y_sen.get());
    ui->sl_zsens->setValue(sw.xps_z_sen.get());
    ui->sl_expo->setValue(go.pMAKO->iuScope->expo.get()*1000);
    ui->lab_expo->setText(QString::fromStdString(util::toString("Exposure: ",go.pMAKO->iuScope->expo.get()," us")));

    if (!go.pMAKO->iuScope->ID.get().empty()) ui->cam1_select->setText(QString::fromStdString("camera ID: "+go.pMAKO->iuScope->ID.get()));
    ui->sl_expo->blockSignals(false);
}


void MainWindow::on_e_xps_ip_editingFinished()      {lineedit_fun(ui->e_xps_ip,&go.pXPS->IP);}
void MainWindow::on_e_xps_port_editingFinished()    {spinbox_fun(ui->e_xps_port,&go.pXPS->port);}
void MainWindow::on_e_xps_xyz_editingFinished()     {ui->e_xps_xyz->blockSignals(true); //this prevents two signals to emmit when pressing enter (a QT bug workaround)
                                                     go.pXPS->groupSetName(XPS::mgroup_XYZ, ui->e_xps_xyz->text().toStdString());
                                                     ui->e_xps_xyz->clearFocus();
                                                     ui->e_xps_xyz->blockSignals(false);}
void MainWindow::on_e_xps_timeout_editingFinished() {spinbox_fun(ui->e_xps_timeout,&go.pXPS->keepalive);}

void MainWindow::on_e_rpty_ip_editingFinished()     {lineedit_fun(ui->e_rpty_ip,&sw.RPTY_ip);}
void MainWindow::on_e_rpty_port_editingFinished()   {spinbox_fun(ui->e_rpty_port,&sw.RPTY_port);}
void MainWindow::on_e_rpty_timeout_editingFinished(){spinbox_fun(ui->e_rpty_timeout,&sw.RPTY_keepalive);}

void MainWindow::on_sl_xsens_valueChanged(int value){slider_fun(ui->sl_xsens,&sw.xps_x_sen,value);}
void MainWindow::on_sl_ysens_valueChanged(int value){slider_fun(ui->sl_ysens,&sw.xps_y_sen,value);}
void MainWindow::on_sl_zsens_valueChanged(int value){slider_fun(ui->sl_zsens,&sw.xps_z_sen,value);}
void MainWindow::on_sl_expo_valueChanged(int value) {
    if(go.pMAKO->iuScope->connected){
        go.pMAKO->iuScope->set<double>("ExposureTime",value/1000);
        go.pMAKO->iuScope->expo.set(go.pMAKO->iuScope->get<double>("ExposureTime"));
    }
}

void MainWindow::on_btn_X_dec_released(){go.pXPS->limit=false; go.pXPS->MoveRelative(XPS::mgroup_XYZ,-1,0,0);}
void MainWindow::on_btn_Y_dec_released(){go.pXPS->limit=false; go.pXPS->MoveRelative(XPS::mgroup_XYZ,0,-1,0);}
void MainWindow::on_btn_Z_dec_released(){go.pXPS->limit=false; go.pXPS->MoveRelative(XPS::mgroup_XYZ,0,0,-1);}
void MainWindow::on_btn_X_min_released(){go.pXPS->setLimit(XPS::mgroup_XYZ,0,XPS::min);}
void MainWindow::on_btn_Y_min_released(){go.pXPS->setLimit(XPS::mgroup_XYZ,1,XPS::min);}
void MainWindow::on_btn_Z_min_released(){go.pXPS->setLimit(XPS::mgroup_XYZ,2,XPS::min);}
void MainWindow::on_btn_X_max_released(){go.pXPS->setLimit(XPS::mgroup_XYZ,0,XPS::max);}
void MainWindow::on_btn_Y_max_released(){go.pXPS->setLimit(XPS::mgroup_XYZ,1,XPS::max);}
void MainWindow::on_btn_Z_max_released(){go.pXPS->setLimit(XPS::mgroup_XYZ,2,XPS::max);}
void MainWindow::on_btn_X_inc_released(){go.pXPS->limit=false; go.pXPS->MoveRelative(XPS::mgroup_XYZ,1,0,0);}
void MainWindow::on_btn_Y_inc_released(){go.pXPS->limit=false; go.pXPS->MoveRelative(XPS::mgroup_XYZ,0,1,0);}
void MainWindow::on_btn_Z_inc_released(){go.pXPS->limit=false; go.pXPS->MoveRelative(XPS::mgroup_XYZ,0,0,1);}

void MainWindow::GUI_update(){
    if (xps_con!=go.pXPS->connected){
        xps_con=go.pXPS->connected;
        ui->si_XPS->setPixmap(xps_con?px_online:px_offline);
    }
    if (iuScope_con!=go.pMAKO->iuScope->connected){
        iuScope_con=go.pMAKO->iuScope->connected;
        ui->si_iuScope->setPixmap(iuScope_con?px_online:px_offline);
    }
    if (sw.RPTY_connected.changed())
        ui->si_RPTY->setPixmap(sw.RPTY_connected.get()?px_online:px_offline);

    if (go.pXPS->IP.resolved.changed())
        ui->e_xps_ip_resolved->setText(QString::fromStdString(go.pXPS->IP.is_name?go.pXPS->IP.resolved.get():" "));

    if(go.pMAKO->iuScope->expo.changed())
        ui->lab_expo->setText(QString::fromStdString(util::toString("Exposure: ",go.pMAKO->iuScope->expo.get()," us")));

    const cv::Mat* dmat=iuScope_img->getUserMat();
    if (dmat!=nullptr){
        ui->camera_stream->setPixmap(QPixmap::fromImage(QImage(dmat->data, dmat->cols, dmat->rows, dmat->step, QImage::Format_Indexed8).copy()));      //.copy() is here to make an actual copy, otherwise it segfaults when the other thread frees the mat //TODO generalize Format_Indexed8
        //std::cerr<<"timestamp: "<<iuScope_img->getUserTimestamp()<<"\n";
        iuScope_img->freeUserMat();
    }

    if (sw.GUI_disable.get()) {
        if (ui->centralWidget->isEnabled())
            ui->centralWidget->setEnabled(false);
    }
    else if (!ui->centralWidget->isEnabled())
        ui->centralWidget->setEnabled(true);

}
