#include "ffmpegs.h"

FFmpegs::FFmpegs(){

}

//检查采样格式
static int check_sample_fmt(const AVCodec* codec, enum AVSampleFormat sample_fmt){
    const enum AVSampleFormat* p = codec->sample_fmts; //p指向了编码器允许的采样格式
    while(*p != AV_SAMPLE_FMT_NONE){
        if(*p == sample_fmt) return 1;
        p++;
    }
    return 0;
}

//封装AAC编码  frame->编码器->packet
//返回负数：出错
//返回0：正确
static int encode(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, QFile& outFile){
    //发送数据到编码器
    int ret = avcodec_send_frame(ctx, frame); //返回0：成功
    if(ret < 0){
        ERROR_BUF(ret);
        qDebug() << "avcodec_send_frame:" << errbuf;
        return ret;
    }
    //不断从编码器中取出数据到AVPacket
    while(true){
        ret = avcodec_receive_packet(ctx, pkt);
        if(ret == AVERROR(EAGAIN) ||ret == AVERROR_EOF){
            return 0;
        }else if(ret < 0){
            return ret;
        }
        //成功从编码器拿到编码后的数据
        //将AVPacket里的数据写入文件
        outFile.write((char* )pkt->data, pkt->size);
        //释放pkt内部资源
        av_packet_unref(pkt);
    }
    return 0;
}

void FFmpegs::aacEncode(AudioEncodeSpec& in, const char* outFilename){
    //文件
    QFile inFile(in.fileName);
    QFile outFile(outFilename);

    //返回结果
    int ret = 0;

    //编码器
    AVCodec* codec = nullptr;

    //编码上下文
    AVCodecContext* ctx = nullptr;

    //存放编码前的数据（PCM）
    AVFrame* frame = nullptr;

    //存放编码后的数据（AAC）
    AVPacket* pkt = nullptr;


    //1.获取编码器
//    codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    codec = avcodec_find_encoder_by_name("libfdk_aac");
    //libfdk_aac对输入格式的要求：采样格式必须是16位整数
    //检查输入数据的采样格式
    if(!check_sample_fmt(codec, in.sampleFmt)){
        qDebug() << "unsupported sample format" << av_get_sample_fmt_name(in.sampleFmt);
        return;
    }

    //2.创建编码上下文
    if(!(ctx = avcodec_alloc_context3(codec))){
        qDebug() << "avcodec_alloc_context3 error";
        return;
    }
    //告知编码上下文PCM参数及编码规格等
    ctx->sample_rate = in.sampleRate;
    ctx->sample_fmt = in.sampleFmt;
    ctx->channel_layout = in.ChLayout;
    ctx->bit_rate = 32000; //比特率32k
    ctx->profile = FF_PROFILE_AAC_HE_V2; //规格
    //打开fdk_aac编码器并设置其特有的vbr属性(这步操作可选）
    //AVDictionary* options = nullptr;
    //av_dict_set(&options, "vbr", "1", 0);

    //3.打开编码器
    ret = avcodec_open2(ctx, codec, nullptr);
    if(ret < 0){
        ERROR_BUF(ret);
        qDebug()<< "avcodec_open2 error" << errbuf;
        goto end;
    }

    //4.创建frame和packet缓冲区
    frame = av_frame_alloc();
    if(!frame){
        qDebug() << "av_frame_alloc error";
        goto end;
    }
    //设置参数使frame里要样本为单位
    frame->nb_samples = ctx->frame_size; //frame缓冲区中样本帧数量-由上下文的建议值决定
    frame->format = in.sampleFmt;
    frame->channel_layout = in.ChLayout;
    // 创建AVFrame内部的缓冲区
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        ERROR_BUF(ret);
        qDebug() << "av_frame_get_buffer error" << errbuf;
        goto end;
    }

    pkt = av_packet_alloc();
    if(!pkt){
        qDebug() << "av_packet_alloc";
        goto end;
    }
    //打开文件
    if(!inFile.open(QFile::ReadOnly)){
        qDebug() << "file open error:" << in.fileName;
        goto end;
    }
    if(!outFile.open(QFile::WriteOnly)){
        qDebug() << "file open error:" << outFilename;
        goto end;
    }

    //4.编码
    while((ret = inFile.read((char* )frame->data[0], frame->linesize[0])) > 0){
        //进行编码
        if(encode(ctx, frame, pkt, outFile) < 0){
            goto end;
        }
    }
    //刷新缓冲区
    encode(ctx, nullptr, pkt, outFile);

end:
    //关闭文件
    inFile.close();
    outFile.close();

    //释放资源
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&ctx);
}

