#include "playthread.h"

#include <SDL2/SDL.h>
#include <QDebug>
#include <QFile>

#define FILENAME "E:/bitch/out.pcm"
#define SAMPLE_RATE 44100
#define SAMPLE_FORMAT AUDIO_S16LSB
//#define SAMPLE_SIZE (AUDIO_S16LSB & 0xFF) //16
//#define SAMPLE_SIZE (AUDIO_S16LSB & SDL_AUDIO_MASK_BITSIZE)
#define SAMPLE_SIZE SDL_AUDIO_BITSIZE(SAMPLE_FORMAT) //16
#define CHANNELS 2
#define SAMPLES 1024 //音频缓冲区样品数量
#define BYTES_PER_SAMPLE ((SAMPLE_SIZE *CHANNELS) >> 8) //每一个样品所占字节大小
#define BUFFER_SIZE (BYTES_PER_SAMPLE * SAMPLES) //音频缓冲区字节大小
                                                 //注：将项目的数据缓冲区和音频缓冲区设置成了一样大小

//int bufferLen = 0;
//char* bufferData = nullptr;

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
void pull_audio_data(void *userdata,
                     Uint8 * stream, //向stream指向的空间填充pcm数据
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


   //音频参数
    SDL_AudioSpec spec;
    spec.freq = SAMPLE_RATE; //采样率
    spec.format = SAMPLE_FORMAT; //采样格式（=s16le)
    spec.channels = CHANNELS; //声道
    spec.samples = SAMPLES; //音频缓冲区的样品数量（2的幂），一个样品：16位，2声道->32bit => 1024样品=4096B
    //将用户自定义参数传递给回调函数
    AudioBuffer buffer;
    spec.userdata = &buffer;
    spec.callback = pull_audio_data; //回调


    //2.打开设备-sdl2
    if(SDL_OpenAudio(&spec, nullptr)){ //返回0:成功 -1:失败
        qDebug() << "SDL_OpenAudio error" << SDL_GetError();
        SDL_Quit();
        return;
    }


    //3.打开文件
    QFile file(FILENAME);
    if(!file.open(QFile::ReadOnly)){
        qDebug() << "file.open error" << FILENAME;
        SDL_CloseAudio(); //关闭设备
        SDL_Quit();
        return;
    }

    //4.开始播放（0-取消暂停）
    /*
     * SDL播放音频有两种模式：
     * push(推):程序主动推送数据给音频设备
     * pull(拉):音频设备主动向程序拉取数据
     */
    SDL_PauseAudio(0); //0-false 1-true //执行到这句代码就启动回调线程，
    Uint8 data[BUFFER_SIZE]; //存放从文件读取出来的数据  //BUFFER_SIZE:理论大小
    while(!isInterruptionRequested()){
       if(buffer.len > 0) continue; //只要读取的数据没全部填入音频缓冲区就退出
       buffer.len = file.read((char*)data, BUFFER_SIZE); //指定每次读BUFFER_SIZE B大小的数据 //buffer.len：实际大小
       if(buffer.len <= 0){  //读取数据错误或读到文件末尾
           //避免音频还在播放就退出循环去关闭设备
           int samples = buffer.pulllen / BYTES_PER_SAMPLE; //样品数
           int ms = samples * 1000 / SAMPLE_RATE; //最后一次读完数据后音频缓冲区剩余数据播放完毕需要的时间
           SDL_Delay(ms);
           break;
        }
       buffer.data = data;
    }

    //释放资源
    file.close(); //关闭文件
    SDL_CloseAudio(); //关闭设备
    SDL_Quit(); //清除子系统
}

