#include "playthread.h"

#include <SDL2/SDL.h>
#include <QDebug>
#include <QFile>

#define FILENAME "E:/bitch/in.wav"


typedef struct{
    int len = 0;
    int pulllen = 0; //每次从数据缓冲区拉取的数据
    Uint8* data = nullptr; //8字节的int指针
} AudioBuffer;

PlayThread::PlayThread(QObject *parent) : QThread(parent){
    connect(this, &PlayThread::finished, this, &PlayThread::deleteLater);
}

PlayThread::~PlayThread(){
    disconnect();
    isInterruptionRequested();
    quit();
    wait();
    qDebug() << "线程" << this->currentThread() << "被回收了";
}



//等待音频设备回调（会回调多次，默认在子线程进行，只要线程启动就会不断回调）
void pull_audio_data(void* userdata,
                     Uint8* stream, //向stream指向的空间填充pcm数据
                     int len //音频缓冲区大小
                     ){

    //清空stream
    SDL_memset(stream, 0, len);
    //取出AudioBuffer
    AudioBuffer* buffer = (AudioBuffer*)userdata;
    //文件数据还没准备好
    if(buffer->len <= 0) return;
    //取len、buffer->len的最小值：防止指针越界，保证数据正确
    buffer->pulllen = len - buffer->len > 0 ? buffer->len : len;

    //填充数据
    SDL_MixAudio(stream, buffer->data, buffer->pulllen, SDL_MIX_MAXVOLUME); //SDL_MIX_MAXVOLUME：最大混合音量
                                           //从buffer->data指向的位置开始读取buffer->pulllen长度数据到stream指向的音频缓冲区
    buffer->data += buffer->pulllen; //挪动指针：可能一次读取到data的数据是音频缓冲区的几倍
    buffer->len -= buffer->pulllen;


}

void PlayThread::run(){
    //1.初始化Audio子系统
   if(SDL_Init(SDL_INIT_AUDIO)){ //返回0-成功 负数-失败 //C语言非0即true
       qDebug() << "SDL_Init error" << SDL_GetError(); //返回sdl当前的错误信息
       return;
   }

    //2.加载WAV文件
    SDL_AudioSpec spec; //音频参数
    Uint8* data = nullptr; //指向加载到内存的整个PCM数据
    Uint32 len = 0; //载进内存的PCM数据大小
    if(!SDL_LoadWAV(FILENAME, &spec, &data, &len)){ //返回null：加载失败
        qDebug() << "SDL_LoadWAV error:" << SDL_GetError();
        SDL_Quit();
        return;
    }
    //设置回调
    spec.samples = 1024; //指定音频缓冲区样本数量
    spec.callback = pull_audio_data; //加载完毕后再回调
    //设置userdata
    AudioBuffer buffer;
    buffer.data = data;
    buffer.len = len;
    spec.userdata = &buffer;


    //3.打开设备-sdl2
    if(SDL_OpenAudio(&spec, nullptr)){ //返回0:成功 -1:失败
        qDebug() << "SDL_OpenAudio error" << SDL_GetError();
        SDL_Quit();
        return;
    }

    //4.开始播放（0-取消暂停）
    SDL_PauseAudio(0); //0-false 1-true //执行到这句代码就启动回调线程

    int sampleSize = SDL_AUDIO_BITSIZE(spec.format);
    int bytesPerSample = (sampleSize * spec.channels) >> 3;
    while(!isInterruptionRequested()){
       if(buffer.len > 0) continue; //只要PCM数据没全部填入音频缓冲区就循环等待

       if(buffer.len <= 0){  //PCM数据读取完毕
           int samples = buffer.pulllen / bytesPerSample; //样品数
           int ms = samples * 1000 / spec.freq; //最后一次读完数据后音频缓冲区剩余数据播放完毕需要的时间 //spec.freq(采样率)
           SDL_Delay(ms);
           break;
        }
    }

    //释放资源
    SDL_FreeWAV(data); //释放WAV文件
    SDL_CloseAudio(); //关闭设备
    SDL_Quit(); //清除子系统
}

