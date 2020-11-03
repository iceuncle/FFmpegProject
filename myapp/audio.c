//
//  test.c
//  myapp
//
//  Created by tianyang on 2020/10/27.
//  Copyright © 2020 tianyang. All rights reserved.
//

#include "audio.h"
#include <string.h>


static int rec_status = 0;


void set_status(int status) {
    rec_status = status;
}


/**
 *   @brief open audio device
 *   @param
 *   @return succ: AVFormatContext*,  fail: NULL
 */
static
AVFormatContext* open_dev() {
    
    int ret = 0;
    char errors[1024] = {0, };
    
    //ctx
    AVFormatContext *fmt_ctx = NULL;
    AVDictionary *options = NULL;
    
    //[[video device]:[audio device]]
    char *devicename = ":0";
    
    //get format 采集格式
    AVInputFormat *iformat = av_find_input_format("avfoundation");

    //open device 打开设备
    if ((ret = avformat_open_input(&fmt_ctx, devicename, iformat, &options)) < 0) {
       av_strerror(ret, errors, 1024);
       av_log(NULL, AV_LOG_INFO,"Failed to open audio device, [%d]%s\n", ret, errors);
       return NULL;
    }
    
    return fmt_ctx;
}


static
AVCodecContext* open_coder() {
    //创建编码器
    //avcodec_find_encoder(AV_CODEC_ID_AAC);
    AVCodec *codec = avcodec_find_encoder_by_name("libfdk_aac");
    
    //创建codec上下文
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    
    codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;            //输入音频的采样大小   libfdk_aac要求的采样大小为16位，因此必须设置为16位
    codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;      //输入音频的channel_layout
    codec_ctx->channels = 2;                              //输入音频的channel个数
    codec_ctx->sample_rate = 44100;                       //输入音频的采样率
    codec_ctx->bit_rate = 0;                              //AAC_LC:128k, AAC_HE: 64k, AAC_HE_V2: 32k
    codec_ctx->profile = FF_PROFILE_AAC_HE_V2;            //如果设置了profile，bit_rate需要设置为0
    
    //打开编码器
    if(avcodec_open2(codec_ctx, codec, NULL) < 0) {
        //打开失败
        return NULL;
    }
    return codec_ctx;
}


static
SwrContext* init_swr() {
    SwrContext *swr_ctx = NULL;
    //channel, number/
    swr_ctx = swr_alloc_set_opts(NULL,                    //ctx
                                AV_CH_LAYOUT_STEREO,     //输出channel布局
                                AV_SAMPLE_FMT_S16,       //输出的采样格式
                                44100,                   //采样率
                                AV_CH_LAYOUT_STEREO,     //输入channel布局
                                AV_SAMPLE_FMT_FLT,       //输入的采样格式
                                44100,                   //输入的采样率
                                0, NULL);
    if(!swr_ctx) {
       
    }

    if(swr_init(swr_ctx) < 0 ) {
       //初始化失败
    }
    return swr_ctx;
}


//[in]
//[out]
//ret
//@brief encode audio data
static
void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *output) {
    
    int ret = 0;
    ret = avcodec_send_frame(ctx, frame);
           
    //如果ret>=0说明数据设置成功
    while (ret >= 0) {
       //获取编码后的音频数据，如果成功，需要重复获取，知道失败为止
       ret = avcodec_receive_packet(ctx, pkt);
        
       if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
           return;
       } else if(ret < 0){
           printf("Error, encoding audio frame\n");
           exit(-1);
       }
       
       //write file
       //fwrite(pkt.data, 1, pkt.size, outfile);
       //fwrite(dst_data[0], 1, dst_linesize, outfile);
       fwrite(pkt->data, 1, pkt->size, output);
       fflush(output);
    }
    return;
}

static
AVFrame* create_frame() {
    
    AVFrame *frame = NULL;
    
    //音频输入数据
    frame = av_frame_alloc();
    if (!frame) {
        printf("Error, No Memory!\n");
        goto __ERROR;
    }

    //set parameters
    frame->nb_samples =     512;                    //单通道一个音频帧的采样数
    frame->format =         AV_SAMPLE_FMT_S16;      //每个采样的大小
    frame->channel_layout = AV_CH_LAYOUT_STEREO;    //channel layout
     
    //alloc inner memory
    av_frame_get_buffer(frame, 0);                  //分配buffer大小: 采样数*采样大小(字节)*通道数  512*2*2=2048
    if (!frame->buf[0]) {
        printf("Error, Failed to alloc buf in frame!\n");
        //内存泄漏
        goto __ERROR;
    }
    
    return frame;
    
__ERROR:
    if (frame) {
        av_frame_free(&frame);
    }
    return NULL;
}


/**
 * @brief 创建重采样输入输出缓冲区
 */
static
void alloc_data_4_resample(uint8_t ***src_data,
                           int *src_linsize,
                           uint8_t ***dst_data,
                           int *dst_linesize) {
    
    
    //创建输入缓冲区
    av_samples_alloc_array_and_samples(src_data,            //输出缓冲区地址
                                      src_linsize,         //缓冲区大小
                                      2,                    //通道个数
                                      512,                  //单通道采样个数   4096/4/2
                                      AV_SAMPLE_FMT_FLT,    //采样格式
                                      0);

    //创建输出缓冲区
    av_samples_alloc_array_and_samples(dst_data,            //输出缓冲区地址
                                      dst_linesize,         //缓冲区大小
                                      2,                    //通道个数
                                      512,                  //单通道采样个数
                                      AV_SAMPLE_FMT_S16,    //采样格式
                                      0);
}


/*
 * @brief 释放重采样输入输出缓冲区
 */
static
void free_data_4_resample(uint8_t **src_data, uint8_t **dst_data) {
    //释放输入输出缓冲区
    if (src_data) {
      av_free(&src_data[0]);
    }
    av_freep(src_data);
    if (dst_data) {
      av_freep(&dst_data[0]);
    }
    av_freep(&dst_data);
}


static
void read_data_and_encode(AVFormatContext *fmt_ctx,
                          SwrContext *swr_ctx,
                          AVCodecContext *c_ctx,
                          FILE *outfile) {
    
    int ret = 0;
    
    //packet 从设备上读取的数据
    AVPacket pkt;
    //编码输入数据
    AVFrame *frame = NULL;
    //编码输入数据
    AVPacket *newpkt = NULL;
    
    //重采样输入数据
    uint8_t **src_data = NULL;
    int src_linsize = 0;
    //重采样输出数据
    uint8_t **dst_data = NULL;
    int dst_linesize= 0;
    
    
    //av_init_packet(&pkt);
    
    //音频输入数据
    frame = create_frame();
    if (!frame) {
        goto __ERROR;
    }

    //音频输出数据
    newpkt = av_packet_alloc();   //分配编码后的数据空间
    if (!newpkt) {
        printf("Error, Failed to alloc buf in frame!\n");
        goto __ERROR;
    }
    
    //分配重采样输入/输出缓冲区
    alloc_data_4_resample(&src_data, &src_linsize, &dst_data, &dst_linesize);

    //read data from device 读取数据
    while (rec_status) {
        
        ret = av_read_frame(fmt_ctx, &pkt);
        if (ret == -35) {
           usleep(100);
           continue;
        }
        
        if (ret != 0 || rec_status != 1) {
          break;
        }

        //进行内存拷贝，按字节进行拷贝
        memcpy((void*)src_data[0], (void*)pkt.data, pkt.size);

        //重采样
        swr_convert(swr_ctx,                        //重采样的上下文
                   dst_data,                       //输出结果缓冲区
                   512,                            //每个通道的采样数
                   (const uint8_t**) src_data,     //输入缓冲区
                   512);                           //输入单个通道的采样数

        //将重采样的数据拷贝到frame中
        memcpy((void*)frame->data[0], dst_data[0], dst_linesize);

        //encode
        encode(c_ctx, frame, newpkt, outfile);

        //release packet 释放packet
        av_packet_unref(&pkt);
    }

    //强制将编码器缓冲区中的音频进行编码输出
    encode(c_ctx, NULL, newpkt, outfile);

__ERROR:
    //释放 AVFrame 和 AVPacket
    if (frame) {
        av_frame_free(&frame);
    }
    if (newpkt) {
        av_packet_free(&newpkt);
    }
    
    //释放重采样缓冲区
    free_data_4_resample(src_data, dst_data);
}


void record_audio() {
    
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *c_ctx = NULL;
    SwrContext *swr_ctx = NULL;

    //set log level
    av_log_set_level(AV_LOG_DEBUG);
    
    //register audio device
    avdevice_register_all();
    
    //start record 开始录制
    rec_status = 1;
    
    //create file
    //char *out = "/Users/tianyang/Desktop/av_base/audio.pcm";
    char *out = "/Users/tianyang/Desktop/av_base/audio.aac";
    FILE *outfile = fopen(out, "wb+");
    if(!outfile) {
        printf("Error, Failed to open file!\n");
        goto __ERROR;
    }
    
    //打开设备
    fmt_ctx = open_dev();
    if (!fmt_ctx) {
        printf("Error, Failed to open devide!\n");
        goto __ERROR;
    }
    
    //打开编码器上下文
    c_ctx = open_coder();
    if (!c_ctx) {
       printf("Error, Failed to open coder!\n");
       goto __ERROR;
    }
    
    //初始化重采样上下文
    swr_ctx = init_swr();
    if (!swr_ctx) {
       printf("Error, Failed to alloc buf in frame!\n");
       goto __ERROR;
   }
    
    //encode
    read_data_and_encode(fmt_ctx, swr_ctx, c_ctx, outfile);
    
__ERROR:
    //释放重采样的上下文
    if (swr_ctx) {
        swr_free(&swr_ctx);
    }

    if (c_ctx) {
        avcodec_free_context(&c_ctx);
    }
    
    //close device and release ctx
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
    
    if (outfile) {
        //close file
        fclose(outfile);
    }
    
    av_log(NULL, AV_LOG_DEBUG, "finish!\n");
    
    return;
}
