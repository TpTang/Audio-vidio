#include "mainwindow.h"

#include <QApplication>
#include "ffmpegutil.h"

extern "C" {
#include <libavdevice/avdevice.h>
}

int main(int argc, char *argv[])
{
    qputenv("QT_SCALE_FACTOR", QByteArray("1"));

    WAVHeaders header;
    header.riffChunkDataSize = 2116836;
    header.channels = 2;
    header.sampleRate = 44100;
    header.bitsPerSample = 16;
    header.blockAlign = header.bitsPerSample * header.channels >> 3;
    header.byteRate = header.blockAlign * header.sampleRate;
    header.dataChunkDataSize = 2116800;

    FFmpegUtil::pcm2wav(header, "E:/bitch/sample.pcm", "E:/bitch/sample.wav");

    avdevice_register_all(); //注册设备

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
