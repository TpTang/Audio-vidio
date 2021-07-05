#include "audiothread.h"


#include <QDebug>
#include "ffmpegs.h"



AudioThread::AudioThread(QObject* parent) : QThread(parent){
    connect(this, &AudioThread::finished,
            this, &AudioThread::deleteLater);
}

AudioThread::~AudioThread(){
    this->disconnect();

    requestInterruption();
    quit();
    wait();
    qDebug() << QThread::currentThread() << "录音线程内存被回收";
}


void AudioThread::run(){
    AudioEncodeSpec in;
    in.fileName = "E:/bitch/sample.pcm";
    in.sampleRate = 44100;
    in.sampleFmt = AV_SAMPLE_FMT_S16;
    in.ChLayout = AV_CH_LAYOUT_STEREO;

    FFmpegs::aacEncode(in, "E:/bitch/out.aac");
}

