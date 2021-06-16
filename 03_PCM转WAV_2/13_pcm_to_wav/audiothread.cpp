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
}

#ifdef Q_OS_WIN
    //格式名称
    #define FMT_NAME "dshow"
    //设备名称
    #define DEVICE_NAME "audio=麦克风 (2- Realtek High Definition Audio)"
    //PCM文件名
    #define FILEPATH "E:/bitch/"
#else
    #define FMT_NAME "avfoundation"
    #define DEVICE_NAME ":0"
    #define FILEPATH "Users/mj/Desktop/out.pcm"
#endif



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


void showSpec(AVFormatContext* ctx){
    AVStream* stream = ctx->streams[0];   //获取输入流
    AVCodecParameters* params = stream->codecpar; //获取音频输入参数
    qDebug() << "声道数：" << params->channels;
    qDebug() << "采样率：" << params->sample_rate;
    qDebug() << "采样格式：" << params->format;
    qDebug() << "每个样本占用字节数（单声道）："
             << av_get_bytes_per_sample((AVSampleFormat)params->format);
}

void AudioThread::run(){
    //一阶段：录制
    //2.获取输入格式对象
    AVInputFormat* fmt = av_find_input_format(FMT_NAME);
    if(!fmt){
        qDebug() << "获取输入格式对象失败" << FMT_NAME;
        return;
    }


    //3.打开设备
    AVFormatContext* ctx = nullptr; //格式上下文、将来可以利用上下文操作对象 -  注意初始化指针对象

    int ret = avformat_open_input(&ctx, DEVICE_NAME, fmt, nullptr); //打开设备 返回0表示成功，负数表示失败
                                 //传入空的ctx到内部去，内部通过指向指针的指针修改外部ctx的指向，外部ctx就获得了指向
    if(ret < 0){
        char errbuf[1024]; //程序将错误信息填在这个数组里返回给外界
        av_strerror(ret, errbuf, sizeof(errbuf)); //传入数组大小（sizeof）
        qDebug() << "打开设备失败" << errbuf;
        return;
    }

    //打印录音设备参数信息
    //showSpec(ctx);


    //4.采集数据
    //4.1读取数据
    AVPacket pkt; //数据包对象
    //文件名
    QString fileName = FILEPATH;
    fileName += QDateTime::currentDateTime().toString("HH_mm_ss");
    QString wavFileName = fileName;
    fileName += ".pcm"; 
    wavFileName += ".wav";
    QFile file(fileName);

    if(!file.open(QFile::WriteOnly)){ //打开文件失败则关闭设备并return
        avformat_close_input(&ctx);
        qDebug() << "文件无法打开" << fileName;
        return;
    }

    //能来到这里说明文件打开成功-开始写数据
//    int count = 10; //控制读取次数

    //isInterruptionRequested()[1]:当外界调用requestInterruption()[2]时，
                                 //[2]自动调用[1]去设置[1]里面的bool变量并返回
    while(!this->isInterruptionRequested()){ //不中断线程且在录音次数内
        ret = av_read_frame(ctx, &pkt); //将读取数据的码返回出来 //重复利用上文ret
        if(ret == 0){ //读取成功
            file.write((const char*)pkt.data, pkt.size); //写入数据
        }else{
            //打印错误信息
            char errbuf[1024];
            av_strerror(ret, errbuf, sizeof(errbuf));
            qDebug() << "错误信息" << errbuf << "错误码" << ret;
            break;
        }

    }
    qDebug() << "数据录入完毕";
    
    //二阶段：
    //5.PCM转WAV
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
    FFmpegUtil::pcm2wav(header,fileName.toUtf8().data(), wavFileName.toUtf8().data());

    //6.释放资源
    file.close(); //关闭文件
    avformat_close_input(&ctx); //关闭读取设备

    qDebug() << "线程正常结束";


}

void AudioThread::setStop(bool stop){
    this->_stop = stop; //由外界调用setStop()来控制_stop的值
}

