#include "audiothread.h"


#include <QDebug>
#include "ffmpegs.h"



AudioThread::AudioThread(QObject* parent) : QThread(parent){ //参数列表调用父类构造函数（super）
    //线程结束时会自动生成finished信号,（但内存没被销毁，只是run()调用完）
    //当监听到线程结束时-finished，就会调用deleteLater销毁内存
    connect(this, &AudioThread::finished,
            this, &AudioThread::deleteLater); //生成对象时调用connect,但监听到线程结束信号才执行
}

AudioThread::~AudioThread(){ //只要当前对象有被销毁的趋势就会调用析构函数
    //断开监听当前线程的所有连接-当前线程内存被回收就不会发出信号了，没必要让其他对象监听我发出的信号
    this->disconnect(); //等价于 this->disconnect(this,nullptr, nullptr, nullptr)

    //在录音过程中点击主窗口关闭按钮时，由于录音线程还在执行，所以会导致子线程异常终止
    //解决：主窗口关闭，真正回收子线程对象内存时会调用子线程的析构->在析构内wait()让子线程正常结束即可
    requestInterruption(); //对于正常流程这句代码无用
    quit();
    wait();
    qDebug() << QThread::currentThread() << "录音线程内存被回收"; //对象销毁时自动调用
}


void AudioThread::run(){
    //输入参数
    ResampleAudioSpec in;
    in.fileName = "E:/bitch/sample.pcm";
    in.sampleFmt = AV_SAMPLE_FMT_S16;
    in.sampleRate = 44100;
    in.ChLayout = AV_CH_LAYOUT_STEREO; //立体声（2声道）


    //输出参数
    ResampleAudioSpec out;
    out.fileName = "E:/bitch/48000_f32le_1.pcm";
    out.sampleFmt = AV_SAMPLE_FMT_FLT;
    out.sampleRate = 48000;
    out.ChLayout = AV_CH_LAYOUT_MONO; //单声道

    FFmpegs::resampleAudio(in, out);

}

