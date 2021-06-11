#ifndef FFMPEGS_H
#define FFMPEGS_H


#include <QDebug>
#include <QFile>

extern "C"{
#include <libswresample/swresample.h> //重采样
#include <libavutil/avutil.h> //工具包
#include <libavformat/avformat.h>
}

#define ERROR_BUF(ret) char errbuf[1024]; av_strerror(ret, errbuf, sizeof(errbuf));

typedef struct {
    const char* fileName;
    int sampleRate;
    AVSampleFormat sampleFmt;
    int ChLayout;
} ResampleAudioSpec;

class FFmpegs{
public:
    FFmpegs();
    static void resampleAudio(ResampleAudioSpec& in, ResampleAudioSpec& out);
    static void resampleAudio(const char* inFileName,
                              int inSampleRate,
                              int inChLayout,
                              AVSampleFormat inSampleFmt,
                              const char* outFileName,
                              int outSampleRate,
                              int outChLayout,
                              AVSampleFormat outSampleFmt);
};

#endif // FFMPEGS_H
