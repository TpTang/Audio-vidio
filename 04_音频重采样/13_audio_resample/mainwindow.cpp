#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <audiothread.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_audioButton_clicked(){
    //开启线程进行重采样
    _audionThread  = new AudioThread(this);
    _audionThread->start();
}
