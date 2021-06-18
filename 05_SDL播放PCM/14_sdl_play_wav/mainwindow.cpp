#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <SDL2/SDL.h>
#include <QDebug>
#include "playthread.h"


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



//使用SDL播放pcm音频
void MainWindow::on_playButton_clicked(){
   if(this->_playThread == nullptr){
       this->_playThread = new PlayThread(this);
       this->_playThread->start();
       ui->playButton->setText("暂停播放");
       //监听播放线程正常播放完毕
       connect(this->_playThread, &PlayThread::finished, [this](){
           this->_playThread = nullptr;
           this->ui->playButton->setText("开始播放");
       });
   }else{
       this->_playThread->isInterruptionRequested();
       this->_playThread = nullptr;
       ui->playButton->setText("开始播放");

   }

}
