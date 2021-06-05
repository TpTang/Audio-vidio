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

void MainWindow::on_audioButton_clicked()
{
    //开启子线程处理按钮点击信号
 //   AudioThread* audioThread = new AudioThread(this); //构造函数传参this（MainWindow）->MainWindow销毁自己才(就)销毁->无需delete thread
 //   audioThread->start(); //启动子线程

     //为了实现一个按钮两个逻辑，将线程对象保存起来
     if(this->_audionThread == nullptr){ //线程对象为空则进入循环 //点击了"开始录音"
        this->_audionThread = new AudioThread(this); //给线程对象赋值
        this->_audionThread->start(); //启动线程
        //设置按钮文字
        ui->audioButton->setText("结束录音"); //ui对象可以拿到MiainWindown里的所有对象
        connect(this->_audionThread,&AudioThread::finished, [this](){ //线程意外终止时，设置按钮"结束录音"->"开始录音"
            this->_audionThread = nullptr;
            this->ui->audioButton->setText("开始录音"); //Lambda要访问外部变量得捕捉外部对象
        } );
     }else{ //点击了"结束录音"
 //     this->_audionThread->setStop(true); //结束线程
        this->_audionThread->requestInterruption(); //代替setStop()去中断线程
        this->_audionThread = nullptr;
        ui->audioButton->setText("开始录音"); //设置按钮文字
      }
}
