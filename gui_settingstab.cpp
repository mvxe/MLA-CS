#include "mainwindow.h"
#include "ui_mainwindow.h"

void MainWindow::on_e_xps_ip_editingFinished(){
    //std::string a = XPS_ip.get();
    //if (XPS_ip.set(ui->e_xps_ip->text().toStdString()))
    //    ui->e_xps_ip->setText(QString::fromStdString(a));
}

void MainWindow::on_e_xps_port_editingFinished(){
    int a = XPS_port.get();
    if (XPS_port.set(ui->e_xps_port->value()))
        ui->e_xps_port->setValue(a);
}

void MainWindow::on_e_xps_xaxis_editingFinished(){

}

void MainWindow::on_e_xps_yaxis_editingFinished(){

}

void MainWindow::on_e_xps_zaxis_editingFinished(){

}
