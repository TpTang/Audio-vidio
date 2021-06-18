#include "audiothread.h"


#include <QThread>
#include <QFile>
#include <QDebug>
#include <QString>
#include <QDateTime>
#include "ffmpegutil.h"
extern "C"{
#include <libavdevice/avdevice.h> //设备
#include <libavformat/avformat.h> //格式
#include <libavutil/avutil.h> //工具包
#include <libavcodec/avcodec.h>
}

#ifdef Q_OS_WIN
    //格式名称
    #define FMT_NAME "dshow"
    //设备名称
    #define DEVICE_NAME "audio=麦克风 (Realtek High Definition Audio)"
    //PCM文件名
    #define FILEPATH "E:/bitch/"
#else
    #define FMT_NAME "avfoundation"
    #define DEVICE_NAME ":0"
    #define FILEPATH "Users/mj/Desktop/out.pcm"
#endif



AudioThread::AudioThread(QObject* parent) : QThread(parent){
    connect(this, &AudioThread::finished,
            this, &AudioThread::deleteLater);
}

AudioThread::~AudioThread(){
    requestInterruption(); //对于正常流程这句代码无用
    quit();
    wait();
    qDebug() << QThread::currentThread() << "录音线程内存被回收";
}



void AudioThread::run(){
    //2.获取输入格式对象
    AVInputFormat* fmt = av_find_input_format(FMT_NAME);
    if(!fmt){
        qDebug() << "获取输入格式对象失败" << FMT_NAME;
        return;
    }

    //3.打开设备
    AVFormatContext* ctx = nullptr; //格式上下文、将来可以利用上下文操作对象 -  注意初始化指针对象

    int ret = avformat_open_input(&ctx, DEVICE_NAME, fmt, nullptr);
    if(ret < 0){
        char errbuf[1024];
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug() << "打开设备失败" << errbuf;
        return;
    }

    //4.采集数据
    //打开
    QString fileName = FILEPATH;
    fileName += QDateTime::currentDateTime().toString("HH_mm_ss");
    fileName += ".wav";
    QFile file(fileName);
    if(!file.open(QFile::WriteOnly)){
        avformat_close_input(&ctx);
        qDebug() << "文件无法打开" << fileName;
        return;
    }

    //4.1写入WAV文件头
    //获取输入流
    AVStream* stream = ctx->streams[0];
    //获取音频采样参数
    AVCodecParameters* params = stream->codecpar;
    WAVHeaders header;
    header.sampleRate = params->sample_rate;
    header.channels = params->channels;
    header.bitsPerSample = av_get_bits_per_sample(params->codec_id);
    header.audioFormat = params->codec_id >= AV_CODEC_ID_PCM_F32BE ?
                         AUDIO_FORMAT_FLOAT : AUDIO_FORMAT_PCM;
    header.blockAlign = header.bitsPerSample * header.channels >> 3;
    header.byteRate = header.blockAlign * header.sampleRate;
    header.dataChunkDataSize = 0;
    file.write((char*)&header, sizeof(WAVHeaders));

    unsigned long long ms = 0; //录音时长

    //4.2录入数据
    AVPacket pkt; //数据包对象
    while(!this->isInterruptionRequested()){
        ret = av_read_frame(ctx, &pkt); //录入PCM
        if(ret == 0){ //读取成功
            file.write((const char*)pkt.data, pkt.size); //写入数据
            header.dataChunkDataSize += pkt.size;
            //写入一点数据就根据数据大小计算录音时长并发送信号给ui线程处理
            ms = 1000.0 * header.dataChunkDataSize / header.byteRate; //每次采集的总长（B)/每秒采集数量（B）
            emit timeChanged(ms); //发射信号
        }else{
            //打印错误信息
            char errbuf[1024];
            av_strerror(ret, errbuf, sizeof(errbuf));
            qDebug() << "错误信息" << errbuf << "错误码" << ret;
            break;
        }
    }
    qDebug() << "数据录入完毕";
    //写入dataChunkDataSize
    file.seek(sizeof(WAVHeaders) - sizeof(header.dataChunkDataSize)); //最后4B
    file.write((char*)&header.dataChunkDataSize, sizeof(header.dataChunkDataSize));
    //写入riffChunkDataSize
    header.riffChunkDataSize = header.dataChunkDataSize
                                                   + sizeof(WAVHeaders)
                                                   - sizeof(header.riffChunkID)
                                                   - sizeof(header.riffChunkDataSize);
    file.seek(sizeof(header.riffChunkID)); //第4B
    file.write((char*)&header.riffChunkDataSize, sizeof(header.riffChunkDataSize));

    //5.释放资源
    file.close(); //关闭文件
    avformat_close_input(&ctx); //关闭读取设备
    qDebug() << "线程正常结束";

}

