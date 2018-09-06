#include "mainwindow.h"
#include "ui_mainwindow.h"

void MainWindow::sync_settings(){
    ui->sl_expo->blockSignals(true);
    ui->e_xps_ip->setText(QString::fromStdString(sw.XPS_ip.get()));
    ui->e_xps_port->setValue(sw.XPS_port.get());
    ui->e_xps_xyz->setText(QString::fromStdString(sw.XYZ_groupname.get()));
    ui->e_xps_timeout->setValue(sw.XPS_keepalive.get());

    ui->e_rpty_ip->setText(QString::fromStdString(sw.RPTY_ip.get()));
    ui->e_rpty_port->setValue(sw.RPTY_port.get());
    ui->e_rpty_timeout->setValue(sw.RPTY_keepalive.get());

    ui->sl_xsens->setValue(sw.xps_x_sen.get());
    ui->sl_ysens->setValue(sw.xps_y_sen.get());
    ui->sl_zsens->setValue(sw.xps_z_sen.get());
    ui->sl_expo->setValue(sw.iuScope_expo.get()*1000);
    ui->lab_expo->setText(QString::fromStdString(util::toString("Exposure: ",sw.iuScope_expo.get()," us")));

    if (!sw.iuScopeID.get().empty()) ui->cam1_select->setText(QString::fromStdString("camera ID: "+sw.iuScopeID.get()));
    ui->sl_expo->blockSignals(false);
}


void MainWindow::on_e_xps_ip_editingFinished()      {lineedit_fun(ui->e_xps_ip,&sw.XPS_ip);}
void MainWindow::on_e_xps_port_editingFinished()    {spinbox_fun(ui->e_xps_port,&sw.XPS_port);}
void MainWindow::on_e_xps_xyz_editingFinished()   {lineedit_fun(ui->e_xps_xyz,&sw.XYZ_groupname);}
void MainWindow::on_e_xps_timeout_editingFinished() {spinbox_fun(ui->e_xps_timeout,&sw.XPS_keepalive);}

void MainWindow::on_e_rpty_ip_editingFinished()     {lineedit_fun(ui->e_rpty_ip,&sw.RPTY_ip);}
void MainWindow::on_e_rpty_port_editingFinished()   {spinbox_fun(ui->e_rpty_port,&sw.RPTY_port);}
void MainWindow::on_e_rpty_timeout_editingFinished(){spinbox_fun(ui->e_rpty_timeout,&sw.RPTY_keepalive);}

void MainWindow::on_sl_xsens_valueChanged(int value){slider_fun(ui->sl_xsens,&sw.xps_x_sen,value);}
void MainWindow::on_sl_ysens_valueChanged(int value){slider_fun(ui->sl_ysens,&sw.xps_y_sen,value);}
void MainWindow::on_sl_zsens_valueChanged(int value){slider_fun(ui->sl_zsens,&sw.xps_z_sen,value);}
void MainWindow::on_sl_expo_valueChanged(int value) {
    if(sw.iuScope_st->connected.get()){
        sw.iuScope_st->set<double>("ExposureTime",value/1000);
        sw.iuScope_expo.set(sw.iuScope_st->get<double>("ExposureTime"));
        //std::cerr<<"exposure(us)="<<sw.iuScope_expo.get(true)<<"\n";
    }
}

void MainWindow::on_btn_X_dec_released(){go.pXPS->XYZMoveRelative(-1,0,0,false);}
void MainWindow::on_btn_Y_dec_released(){go.pXPS->XYZMoveRelative(0,-1,0,false);}
void MainWindow::on_btn_Z_dec_released(){go.pXPS->XYZMoveRelative(0,0,-1,false);}
void MainWindow::on_btn_X_min_released(){sw.Xaxis_min.set(sw.Xaxis_position.get());}
void MainWindow::on_btn_Y_min_released(){sw.Yaxis_min.set(sw.Yaxis_position.get());}
void MainWindow::on_btn_Z_min_released(){sw.Zaxis_min.set(sw.Zaxis_position.get());}
void MainWindow::on_btn_X_max_released(){sw.Xaxis_max.set(sw.Xaxis_position.get());}
void MainWindow::on_btn_Y_max_released(){sw.Yaxis_max.set(sw.Yaxis_position.get());}
void MainWindow::on_btn_Z_max_released(){sw.Zaxis_max.set(sw.Zaxis_position.get());}
void MainWindow::on_btn_X_inc_released(){go.pXPS->XYZMoveRelative(1,0,0,false);}
void MainWindow::on_btn_Y_inc_released(){go.pXPS->XYZMoveRelative(0,1,0,false);}
void MainWindow::on_btn_Z_inc_released(){go.pXPS->XYZMoveRelative(0,0,1,false);}

void MainWindow::GUI_update(){
    if (sw.XPS_connected.changed())
        ui->si_XPS->setPixmap(sw.XPS_connected.get()?*px_online:*px_offline);
    if (sw.iuScope_connected.changed())
        ui->si_iuScope->setPixmap(sw.iuScope_connected.get()?*px_online:*px_offline);
    if (sw.RPTY_connected.changed())
        ui->si_RPTY->setPixmap(sw.RPTY_connected.get()?*px_online:*px_offline);

    if (sw.XPS_ip.resolved.changed())
        ui->e_xps_ip_resolved->setText(QString::fromStdString(sw.XPS_ip.is_name?sw.XPS_ip.resolved.get():" "));

    if(sw.iuScope_expo.changed())
        ui->lab_expo->setText(QString::fromStdString(util::toString("Exposure: ",sw.iuScope_expo.get()," us")));

    const cv::Mat* dmat=sw.iuScope_img->getUserMat();
    if (dmat!=nullptr){
        ui->camera_stream->setPixmap(QPixmap::fromImage(QImage(dmat->data, dmat->cols, dmat->rows, dmat->step, QImage::Format_Indexed8).copy()));      //.copy() is here to make an actual copy, otherwise it segfaults when the other thread frees the mat //TODO generalize Format_Indexed8
        //std::cerr<<"timestamp: "<<sw.iuScope_img->getUserTimestamp()<<"\n";
        sw.iuScope_img->freeUserMat();
    }

    if (sw.GUI_disable.get()) {
        if (ui->centralWidget->isEnabled())
            ui->centralWidget->setEnabled(false);
    }
    else if (!ui->centralWidget->isEnabled())
        ui->centralWidget->setEnabled(true);

}
