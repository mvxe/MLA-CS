#include "mainwindow.h"
#include "../../includes.h"

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
