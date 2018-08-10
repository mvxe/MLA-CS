#include "mainwindow.h"
#include "ui_mainwindow.h"


void MainWindow::sync_settings(){
    ui->e_xps_ip->setText(QString::fromStdString(sw.XPS_ip.get()));
    ui->e_xps_port->setValue(sw.XPS_port.get());
    ui->e_xps_xaxis->setText(QString::fromStdString(sw.Xaxis_groupname.get()));
    ui->e_xps_yaxis->setText(QString::fromStdString(sw.Yaxis_groupname.get()));
    ui->e_xps_zaxis->setText(QString::fromStdString(sw.Zaxis_groupname.get()));
    ui->e_xps_timeout->setValue(sw.XPS_keepalive.get());

    ui->e_rpty_ip->setText(QString::fromStdString(sw.RPTY_ip.get()));
    ui->e_rpty_port->setValue(sw.RPTY_port.get());
    ui->e_rpty_timeout->setValue(sw.RPTY_keepalive.get());
}


void MainWindow::on_e_xps_ip_editingFinished()      {lineedit_fun(ui->e_xps_ip,&sw.XPS_ip);}
void MainWindow::on_e_xps_port_editingFinished()    {spinbox_fun(ui->e_xps_port,&sw.XPS_port);}
void MainWindow::on_e_xps_xaxis_editingFinished()   {lineedit_fun(ui->e_xps_xaxis,&sw.Xaxis_groupname);}
void MainWindow::on_e_xps_yaxis_editingFinished()   {lineedit_fun(ui->e_xps_yaxis,&sw.Yaxis_groupname);}
void MainWindow::on_e_xps_zaxis_editingFinished()   {lineedit_fun(ui->e_xps_zaxis,&sw.Zaxis_groupname);}
void MainWindow::on_e_xps_timeout_editingFinished() {spinbox_fun(ui->e_xps_timeout,&sw.XPS_keepalive);}

void MainWindow::on_e_rpty_ip_editingFinished()     {lineedit_fun(ui->e_rpty_ip,&sw.RPTY_ip);}
void MainWindow::on_e_rpty_port_editingFinished()   {spinbox_fun(ui->e_rpty_port,&sw.RPTY_port);}
void MainWindow::on_e_rpty_timeout_editingFinished(){spinbox_fun(ui->e_rpty_timeout,&sw.RPTY_keepalive);}


void MainWindow::GUI_update(){
    if (sw.XPS_connected.changed())
        ui->si_XPS->setPixmap(sw.XPS_connected.get()?*px_online:*px_offline);
    if (sw.MAKO_connected.changed())
        ui->si_XPS->setPixmap(sw.MAKO_connected.get()?*px_online:*px_offline);
    if (sw.RPTY_connected.changed())
        ui->si_XPS->setPixmap(sw.RPTY_connected.get()?*px_online:*px_offline);

    if (sw.XPS_ip.resolved.changed())
        ui->e_xps_ip_resolved->setText(QString::fromStdString(sw.XPS_ip.is_name?sw.XPS_ip.resolved.get():" "));
}
