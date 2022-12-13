#include "GUI/mainwindow.h"
#include "DEV/GCAM/gcam.h"
#include "ui_mainwindow.h"
#include <opencv2/highgui/highgui.hpp>  // Video write

MainWindow::MainWindow(QApplication* qapp, std::vector<rtoml::vsr *> *confList) : qapp(qapp), QMainWindow(0), ui(new Ui::MainWindow) {
    connect(qapp,SIGNAL(aboutToQuit()),this,SLOT(program_exit()));
    shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q),this,SLOT(program_exit()));

    ui->setupUi(this);

    tabDev=new tab_devices(ui->tab_dev);
    for(auto& dev: go.GUIdevList){
        for(auto& widget: dev->connectionGUI)
            tabDev->addWidget(widget);
    }
    QPushButton* savebtn=new QPushButton("Save configuration");
    connect(savebtn,SIGNAL(released()),this,SLOT(onSaveBtn()));
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    spacer->setMinimumHeight(50);
    tabDev->addWidget(spacer);
    tabDev->addWidget(new twid(savebtn));

    tabCam=new tab_camera(ui->tab_camera);
    confList->push_back(&tabCam->conf);

    setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
}

MainWindow::~MainWindow(){
    delete ui;
    if(!cleanedTabs){
        delete tabDev;
        delete tabCam;
    }
}

void MainWindow::program_exit(){
    go.cleanup();
    delete tabDev;
    delete tabCam;
    cleanedTabs=true;
}

void MainWindow::on_tabWidget_currentChanged(int index){
    QString tabName=ui->tabWidget->tabText(index);
    QString lastTabName=ui->tabWidget->tabText(lastIndex);
    lastIndex=index;

    ui->centralWidget->setFocus();  //prevents random textboxes from receiving focus after tab switch

    if (tabName=="Nanostructuring") tabCam->tab_entered();
    else if(lastTabName=="Nanostructuring") tabCam->tab_exited();
}

void saveDialog::recursiveList(uint depth, varCheckList* _vcl){
    twid* tw=new twid(_vcl->chbox);
    tw->layout->insertSpacing(0,depth*10);
    layout.addWidget(tw);
    if(_vcl->conf->map!=nullptr) for(auto& item:*_vcl->conf->map){
        try{
            if(!((*_vcl->conf->map)[item.first]).changed()) continue;
        }catch(...){}
        _vcl->queue.emplace();
        _vcl->queue.back().chbox=new hQCheckBox(QString::fromStdString(item.first), this);
        _vcl->queue.back().chbox->setChecked(true);
        _vcl->queue.back().conf=&((*_vcl->conf->map)[item.first]);
        connect(_vcl->chbox,SIGNAL(stateChanged(int)),_vcl->queue.back().chbox,SLOT(parent_changed(int)));
        connect(_vcl->queue.back().chbox,SIGNAL(stateChanged(int)),_vcl->chbox,SLOT(child_changed(int)));
        recursiveList(depth+1, &_vcl->queue.back());
    }
}

void hQCheckBox::parent_changed(int state){
    if(state==Qt::Checked)
        setCheckState(Qt::Checked);
}
void hQCheckBox::child_changed(int state){
    if(state==Qt::Unchecked)
        setCheckState(Qt::Unchecked);
}

saveDialog::saveDialog(QWidget* parent, std::vector<rtoml::vsr *>& confList):QDialog(parent){
    connect(this,SIGNAL(finished(int)),this,SLOT(on_finished(int)));
    for(auto conf: confList){
        if(conf->changed()){
            vcl.emplace();
            vcl.back().chbox=new hQCheckBox(QString::fromStdString(conf->getConfFilename()), this);
            vcl.back().chbox->setChecked(true);
            vcl.back().conf=conf;
            recursiveList(0, &vcl.back());
        }
    }
    QWidget *widget = new QWidget();
    widget->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Expanding);
    widget->setLayout(&layout);
    qsa.setWidget(widget);
    qsa.setFrameShape(QFrame::NoFrame);
    qsa.horizontalScrollBar()->setStyleSheet("QScrollBar {height:0px;}");
    QVBoxLayout* topLayout=new QVBoxLayout;
    topLayout->addWidget(new QLabel("List of changed settings:"));
    if(layout.count()==0){topLayout->addWidget(new QLabel("No changes."));
        QPushButton* ok=new QPushButton("Ok");
        ok->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        connect(ok,SIGNAL(released()),this,SLOT(reject()));
        topLayout->addWidget(ok);
    }else{
        topLayout->addWidget(&qsa);
        QPushButton* accept=new QPushButton("Save checked entries");
        QPushButton* reject=new QPushButton("Exit without saving");
        accept->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        reject->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        connect(accept,SIGNAL(released()),this,SLOT(accept()));
        connect(reject,SIGNAL(released()),this,SLOT(reject()));
        topLayout->addWidget(new twid(accept,reject,true,false));
    }
    setLayout(topLayout);
    move(parent->frameGeometry().center()-rect().center());
}

void saveDialog::recursiveSave(varCheckList* _vcl){
    if(_vcl->chbox->isChecked()){
        _vcl->conf->save();
    }
    else while(!_vcl->queue.empty()){
        recursiveSave(&_vcl->queue.front());
        _vcl->queue.pop();
    }
}
varCheckList::~varCheckList(){
    while(!queue.empty()) queue.pop();
}

void saveDialog::on_finished(int result){
    if(result==QDialog::Accepted){
        while(!vcl.empty()){
            recursiveSave(&vcl.front());
            vcl.pop();
        }
    }
}

void MainWindow::onSaveBtn(){
    saveDialog sD(this,go.confList);
    sD.exec();
}
