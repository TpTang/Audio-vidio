#ifndef FFMPEGS_H
#define FFMPEGS_H


#include <QDebug>
#include <QFile>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h> //工具包
}

#define ERROR_BUF(ret) char errbuf[1024]; av_strerror(ret, errbuf, sizeof(errbuf));

typedef struct {
    const char* fileName;
    int sampleRate;
    AVSampleFormat sampleFmt;
    int ChLayout;
} AudioEncodeSpec;

class FFmpegs{
public:
    FFmpegs();
    static void aacEncode(AudioEncodeSpec& in, const char* outFilename);

};

#endif // FFMPEGS_H
