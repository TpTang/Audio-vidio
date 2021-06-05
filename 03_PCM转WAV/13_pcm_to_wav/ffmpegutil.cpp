#include "ffmpegutil.h"

FFmpegUtil::FFmpegUtil(){

}

void FFmpegUtil::pcm2wav(WAVHeaders &header,
                         const char *pcmFileName,
                         const char *wavFileName){

    //打开PCM文件
    QFile pcmFile(pcmFileName);
    if(!pcmFile.open(QFile::ReadOnly)){
        qDebug() << "文件打开失败" << pcmFileName;
        return;
    }
    //打开WAV文件
    QFile wavFile(wavFileName);
    if(!wavFile.open(QFile::WriteOnly)){
        qDebug() << "文件打开失败" << wavFileName;
        pcmFile.close();
        return;
    }

    //写入文件头
    wavFile.write((const char*)&header, sizeof(WAVHeaders)); //将结构体对象写入文件头

    //写入PCM数据
    char buf[1024];
    int size;
    while((size = pcmFile.read(buf, sizeof(buf))) > 0){
        wavFile.write(buf, size);
    }

    //关闭文件
    pcmFile.close();
    wavFile.close();

}
