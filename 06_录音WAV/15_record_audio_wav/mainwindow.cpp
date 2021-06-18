#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <audiothread.h>
#include <QTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    onTimeChanged(0); //初始化时间-开启主窗口就显示初始时间
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onTimeChanged(unsigned long long ms){
    QTime time(0, 0, 0, 0);
    QString textTime = time.addMSecs(ms).toString("mm:ss:z");
    ui->timeLable->setText(textTime.left(7));
}
void MainWindow::on_audioButton_clicked(){
     if(this->_audionThread == nullptr){
        this->_audionThread = new AudioThread(this);
        this->_audionThread->start(); //启动线程

        //监听录音时长信号
         connect(_audionThread, &AudioThread::timeChanged, this, &MainWindow::onTimeChanged);
        //监听线程意外终止信号
        connect(this->_audionThread, &AudioThread::finished, [this](){ //线程意外终止时，设置按钮"结束录音"->"开始录音"
            this->_audionThread = nullptr;
            this->ui->audioButton->setText("开始录音"); //Lambda要访问外部变量得捕捉外部对象
        } );
        ui->audioButton->setText("结束录音"); //ui对象可以拿到MiainWindown里的所有对象
     }else{ //点击了"结束录音"
        this->_audionThread->requestInterruption();
        this->_audionThread = nullptr;
        ui->audioButton->setText("开始录音"); //设置按钮文字
      }

}
