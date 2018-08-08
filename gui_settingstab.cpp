#include "mainwindow.h"
#include "ui_mainwindow.h"


void MainWindow::sync_settings(){
    ui->e_xps_ip->setText(QString::fromStdString(XPS_ip.get()));
    ui->e_xps_port->setValue(XPS_port.get());
    ui->e_xps_xaxis->setText(QString::fromStdString(Xaxis_groupname.get()));
    ui->e_xps_yaxis->setText(QString::fromStdString(Yaxis_groupname.get()));
    ui->e_xps_zaxis->setText(QString::fromStdString(Zaxis_groupname.get()));
    ui->e_xps_timeout->setValue(XPS_keepalive.get());

    ui->e_rpty_ip->setText(QString::fromStdString(RPTY_ip.get()));
    ui->e_rpty_port->setValue(RPTY_port.get());
    ui->e_rpty_timeout->setValue(RPTY_keepalive.get());
}


void MainWindow::on_e_xps_ip_editingFinished()      {lineedit_fun(ui->e_xps_ip,&XPS_ip);}
void MainWindow::on_e_xps_port_editingFinished()    {spinbox_fun(ui->e_xps_port,&XPS_port);}
void MainWindow::on_e_xps_xaxis_editingFinished()   {lineedit_fun(ui->e_xps_xaxis,&Xaxis_groupname);}
void MainWindow::on_e_xps_yaxis_editingFinished()   {lineedit_fun(ui->e_xps_yaxis,&Yaxis_groupname);}
void MainWindow::on_e_xps_zaxis_editingFinished()   {lineedit_fun(ui->e_xps_zaxis,&Zaxis_groupname);}
void MainWindow::on_e_xps_timeout_editingFinished() {spinbox_fun(ui->e_xps_timeout,&XPS_keepalive);}

void MainWindow::on_e_rpty_ip_editingFinished()     {lineedit_fun(ui->e_rpty_ip,&RPTY_ip);}
void MainWindow::on_e_rpty_port_editingFinished()   {spinbox_fun(ui->e_rpty_port,&RPTY_port);}
void MainWindow::on_e_rpty_timeout_editingFinished(){spinbox_fun(ui->e_rpty_timeout,&RPTY_keepalive);}
