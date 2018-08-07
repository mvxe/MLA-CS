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


void MainWindow::on_e_xps_ip_editingFinished(){
    if (XPS_ip.set(ui->e_xps_ip->text().toStdString()))
        ui->e_xps_ip->setText(QString::fromStdString(XPS_ip.get()));
}

void MainWindow::on_e_xps_port_editingFinished(){
    if (XPS_port.set(ui->e_xps_port->value()))
        ui->e_xps_port->setValue(XPS_port.get());
}

void MainWindow::on_e_xps_xaxis_editingFinished(){
    if (Xaxis_groupname.set(ui->e_xps_xaxis->text().toStdString()))
        ui->e_xps_xaxis->setText(QString::fromStdString(Xaxis_groupname.get()));
}

void MainWindow::on_e_xps_yaxis_editingFinished(){
    if (Yaxis_groupname.set(ui->e_xps_yaxis->text().toStdString()))
        ui->e_xps_yaxis->setText(QString::fromStdString(Yaxis_groupname.get()));
}

void MainWindow::on_e_xps_zaxis_editingFinished(){
    if (Zaxis_groupname.set(ui->e_xps_zaxis->text().toStdString()))
        ui->e_xps_zaxis->setText(QString::fromStdString(Zaxis_groupname.get()));
}

void MainWindow::on_e_xps_timeout_editingFinished(){
    if (XPS_keepalive.set(ui->e_xps_timeout->value()))
        ui->e_xps_timeout->setValue(XPS_keepalive.get());
}







void MainWindow::on_e_rpty_ip_editingFinished(){
    if (RPTY_ip.set(ui->e_rpty_ip->text().toStdString()))
        ui->e_rpty_ip->setText(QString::fromStdString(RPTY_ip.get()));
}

void MainWindow::on_e_rpty_port_editingFinished(){
    if (RPTY_port.set(ui->e_rpty_port->value()))
        ui->e_rpty_port->setValue(RPTY_port.get());
}

void MainWindow::on_e_rpty_timeout_editingFinished(){
    if (RPTY_keepalive.set(ui->e_rpty_timeout->value()))
        ui->e_rpty_timeout->setValue(RPTY_keepalive.get());
}
