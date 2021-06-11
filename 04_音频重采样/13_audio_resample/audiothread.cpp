#include "audiothread.h"


#include <QFile>
#include <QDebug>

extern "C"{
#include <libswresample/swresample.h> //重采样
#include <libavutil/avutil.h> //工具包
}

#define ERROR_BUF(ret) char errbuf[1024]; av_strerror(ret, errbuf, sizeof(errbuf));


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
    const char* inFileName = "E:/bitch/sample.pcm";
    QFile inFile(inFileName);

    const char* outFileName = "E:/bitch/48000_f32le_1.pcm";
    QFile outFile(outFileName);

    //输入参数
    AVSampleFormat inSampleFmt = AV_SAMPLE_FMT_S16;
    int inSampleRate = 44100;
    int inChLayout = AV_CH_LAYOUT_STEREO; //立体声（2声道）

    //创建输入缓冲区
    uint8_t** inData = nullptr; //指向输入缓冲区的外层指针
    int inLinesize = 0; //缓冲区大小
    int inChs = av_get_channel_layout_nb_channels(inChLayout); //声道数
    int inBytesPerSample = inChs * av_get_bytes_per_sample(inSampleFmt); //每个输入样本占字节数
    int inSamples = 1024; //缓冲区样本数量


    //输出参数
    AVSampleFormat outSampleFmt = AV_SAMPLE_FMT_FLT;
    int outSampleRate = 48000;
    int outChLayout = AV_CH_LAYOUT_MONO; //单声道

    //创建输出缓冲区
    uint8_t** outData = nullptr; //指向输入缓冲区的外层指针
    int outLinesize = 0; //缓冲区大小
    int outChs = av_get_channel_layout_nb_channels(outChLayout); //声道数
    int outBytesPerSample = outChs * av_get_bytes_per_sample(outSampleFmt); //每个输出样本占字节数
    int outSamples = 1024; //缓冲区样本数量


    int ret = 0; //函数调用返回结果
    int len = 0; //每次读入数据大小

    //1.创建重采样上下文
    SwrContext* ctx = swr_alloc_set_opts(nullptr,
                                         outChLayout, outSampleFmt, outSampleRate,
                                         inChLayout, inSampleFmt, inSampleRate,
                                         0, nullptr);
    if(ctx == NULL){
        qDebug() << "swr_alloc_set_opts error";
        return;
    }

    //2.初始化重采样上下文
    ret = swr_init(ctx);
    if(ret < 0){
        ERROR_BUF(ret);
        qDebug() << "swr_init error:" << errbuf;
        goto end;
    }

    //3.重采样
    //创建输入缓冲区
    ret = av_samples_alloc_array_and_samples(&inData, &inLinesize,
                                             inChs,inSamples,
                                             inSampleFmt, 1);
    if(ret < 0){
        ERROR_BUF(ret);
        qDebug() << "av_samples_alloc_array_and_samples:" << errbuf;
        goto end;
    }

    //创建输出缓冲区
    ret = av_samples_alloc_array_and_samples(&outData, &outLinesize,
                                             outChs,outSamples,
                                             outSampleFmt, 1);
    if(ret < 0){
        ERROR_BUF(ret);
        qDebug() << "av_samples_alloc_array_and_samples:" << errbuf;
        goto end;
    }

    //读取数据
    if(!inFile.open(QFile::ReadOnly)){ //打开读文件失败
        qDebug() << "file open error:" << inFileName;
        goto end;
    }

    if(!outFile.open(QFile::WriteOnly)){ //打开写文件失败
        qDebug() << "file open error:" << outFileName;
        goto end;
    }

    //重采样
    while((len = inFile.read((char*)inData[0], inLinesize)) > 0){
        inSamples = len / inBytesPerSample; //真正读取的样本数量
        //ret：转换后的输出缓冲区的样本数量
        ret = swr_convert(ctx,
                    outData, outSamples,
                    (const uint8_t **)inData, inSamples);
        if(ret < 0){
            ERROR_BUF(ret);
            qDebug() << "swr_convert error:" << errbuf;
        }
        //将转换后的数据写入文件
        outFile.write((char *)outData[0], ret * outBytesPerSample);
    }

end:
    //关闭文件
    inFile.close();
    outFile.close();
    //释放输入缓冲区
    av_freep(&inData);
    //释放输出缓冲区
    av_freep(&outData);
    //释放重采样上下文
    swr_free(&ctx);
}

