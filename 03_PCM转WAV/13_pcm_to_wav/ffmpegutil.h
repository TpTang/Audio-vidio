#ifndef FFMPEGUTIL_H
#define FFMPEGUTIL_H


#include <QObject>
#include <QDebug>
#include <QFile>

typedef struct{
    //RIFF chunk id
    uint8_t riffChunkID[4] = {'R', 'I', 'F', 'F'}; //uint8_t:本质上就是个char
    //RIFF chunk data size : 文件总长-8B
    uint32_t riffChunkDataSize;
    // 输出文件格式：WAV
    uint8_t format[4] = {'W', 'A', 'V', 'E'};
    //fmt chunk id
    uint8_t fmtChunkID[4] = {'f', 'm', 't', ' '};
    //fmt chunk data size
    uint32_t fmtChunkDataSize = 16;
    // 1表示PCM数据
    uint16_t audioFormat = 1;
    //channels
    uint16_t channels;
    //sample rate
    uint32_t sampleRate;
    //byte rate : sampleRate * blockAlign
    uint32_t byteRate;
    //一个样本的字节数：bitspersample * channels >> 8
    uint16_t blockAlign;
    //位深度
    uint16_t bitsPerSample;
    //data chunk id
    uint8_t dataChunkID[4] = {'d', 'a', 't', 'a'};
    //data chunk data size : 即PCM数据大小
    uint32_t dataChunkDataSize;

}WAVHeaders;

class FFmpegUtil{
public:
    FFmpegUtil();
    static void pcm2wav(WAVHeaders& header,
                        const char* pcmFileName,
                        const char* wavFileName);
};

#endif // FFMPEGUTIL_H
