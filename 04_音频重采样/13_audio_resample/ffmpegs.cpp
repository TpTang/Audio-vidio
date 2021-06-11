#include "ffmpegs.h"

FFmpegs::FFmpegs(){

}

void FFmpegs::resampleAudio(ResampleAudioSpec& in, ResampleAudioSpec& out){
    resampleAudio(in.fileName, in.sampleRate, in.ChLayout, in.sampleFmt,
                  out.fileName, out.sampleRate, out.ChLayout, out.sampleFmt);
}
void FFmpegs::resampleAudio(const char *inFileName,
                            int inSampleRate,
                            int inChLayout,
                            AVSampleFormat inSampleFmt,
                            const char *outFileName,
                            int outSampleRate,
                            int outChLayout,
                            AVSampleFormat outSampleFmt){
    QFile inFile(inFileName);
    QFile outFile(outFileName);
    //创建输入缓冲区
    uint8_t** inData = nullptr; //指向输入缓冲区的外层指针
    int inLinesize = 0; //缓冲区大小
    int inChs = av_get_channel_layout_nb_channels(inChLayout); //声道数
    int inBytesPerSample = inChs * av_get_bytes_per_sample(inSampleFmt); //每个输入样本占字节数
    int inSamples = 1024; //缓冲区样本数量

    //创建输出缓冲区
    uint8_t** outData = nullptr; //指向输入缓冲区的外层指针
    int outLinesize = 0; //缓冲区大小
    int outChs = av_get_channel_layout_nb_channels(outChLayout); //声道数
    int outBytesPerSample = outChs * av_get_bytes_per_sample(outSampleFmt); //每个输出样本占字节数
//    int outSamples = 1024; //缓冲区样本数量 //重采样后时间一样，采样率变了，样本数量也变了，
                                            //输出缓冲区大小与输入缓冲区大小也不一样了
    int outSamples = av_rescale_rnd(outSampleRate, inSamples,inSampleRate, AV_ROUND_UP);
    /*
     * inSampleRate     inSamples
     * ------------  =  ---------
     * outSampleRate    outSamples(求）
     */
    int ret = 0; //函数调用返回结果
    int len = 0; //每次读入数据大小
    //1.创建重采样上下文
    SwrContext* ctx = swr_alloc_set_opts(nullptr,outChLayout, outSampleFmt, outSampleRate,
                                         inChLayout, inSampleFmt, inSampleRate,0, nullptr);
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
    ret = av_samples_alloc_array_and_samples(&inData, &inLinesize,inChs,inSamples,
                                             inSampleFmt, 1);
    if(ret < 0){
        ERROR_BUF(ret);
        qDebug() << "av_samples_alloc_array_and_samples:" << errbuf;
        goto end;
    }
    //创建输出缓冲区
    ret = av_samples_alloc_array_and_samples(&outData, &outLinesize,outChs,outSamples,
                                             outSampleFmt, 1);
    if(ret < 0){
        ERROR_BUF(ret);
        qDebug() << "av_samples_alloc_array_and_samples:" << errbuf;
        goto end;
    }
    //重采样
    if(!inFile.open(QFile::ReadOnly)){ //打开读文件失败
        qDebug() << "file open error:" << inFileName;
        goto end;
    }
    if(!outFile.open(QFile::WriteOnly)){ //打开写文件失败
        qDebug() << "file open error:" << outFileName;
        goto end;
    }
//    while((len = inFile.read((char*)inData[0], inLinesize)) > 0){ //双声道时inData是指向两个uint8_t的指针数组
      //inData[0] == *inData
    while((len = inFile.read((char*) *inData, inLinesize)) > 0){
        inSamples = len / inBytesPerSample; //真正读取的样本数量
        //ret：转换后的输出缓冲区的样本数量
        ret = swr_convert(ctx, outData, outSamples,(const uint8_t **)inData, inSamples);
        if(ret < 0){
            ERROR_BUF(ret);
            qDebug() << "swr_convert error:" << errbuf;
        }
        //将转换后的数据写入文件
        outFile.write((char *) *outData, ret * outBytesPerSample);
    }
    //检查输出缓冲区是否还有残留样本（已经重采样过了的）
    while((ret = swr_convert(ctx, outData, outSamples, nullptr, 0)) > 0){ //条件成立：缓冲区有残留
        outFile.write((char*) *outData, ret * outBytesPerSample);
    }

end:
    //关闭文件
    inFile.close();
    outFile.close();
    //释放输入缓冲区
    if(inData){
        av_freep(&inData[0]); //先释放缓冲区 //取址：为了内部能使inData==nullptr
    }
    av_freep(&inData); //再释放指向缓冲区的堆
    //释放输出缓冲区
    if(outData){
        av_freep(&outData[0]);
    }
    av_freep(&outData);
    //释放重采样上下文
    swr_free(&ctx);
}
