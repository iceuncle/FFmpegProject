//
//  video.c
//  myapp
//
//  Created by tianyang on 2020/10/27.
//  Copyright © 2020 tianyang. All rights reserved.
//

#include "video.h"
#include <string.h>

#define V_WIDTH 640
#define V_HEIGHT 480

static int rec_status = 0;


void set_video_status(int status) {
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
    //0: 机器的摄像头
    //1: 桌面
    char *devicename = "0";
    
    //get format 采集格式
    AVInputFormat *iformat = av_find_input_format("avfoundation");
    
    //分辨率
    av_dict_set(&options, "video_size", "640x480", 0);
    //帧率
    av_dict_set(&options, "framerate", "30", 0);
    //设置采集的格式
    av_dict_set(&options, "pixel_format", "nv12", 0);

    //open device 打开设备
    if ((ret = avformat_open_input(&fmt_ctx, devicename, iformat, &options)) < 0) {
       av_strerror(ret, errors, 1024);
       av_log(NULL, AV_LOG_INFO,"Failed to open audio device, [%d]%s\n", ret, errors);
       return NULL;
    }
    
    return fmt_ctx;
}

/**
 * @brief 打开编码器
 */
static void open_encoder(int width, int height, AVCodecContext **enc_ctx) {
    int ret = 0;
    AVCodec *codec = NULL;
    codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        printf("Codec libx264 not found\n");
        exit(1);
    }
    *enc_ctx = avcodec_alloc_context3(codec);
    if (!enc_ctx) {
        printf("Could not allocate video context!\n");
        exit(1);
    }
    //SPS/PPS
    (*enc_ctx)->profile = FF_PROFILE_H264_HIGH_444;
    (*enc_ctx)->level = 50; //表示LEVEL是5.0
   
    //设置分辨率
    (*enc_ctx)->width = width;    //640
    (*enc_ctx)->height = height;  // 480
    
    //GOP
    (*enc_ctx)->gop_size = 250;    //最大的GOP的值
    (*enc_ctx)->keyint_min = 25;   //option  最小的I帧的间隔
    
    //设置B帧数量  为了减少码流可以设置b帧
    (*enc_ctx)->max_b_frames = 3;  //option
    (*enc_ctx)->has_b_frames = 1;  //option
    
    //参考帧的数量 参考帧越大，处理的越慢，还原性会越好
    (*enc_ctx)->refs = 3;   //option
    
    //设置输入的YUV格式
    (*enc_ctx)->pix_fmt = AV_PIX_FMT_YUV420P;
    
    //设置码率
    (*enc_ctx)->bit_rate = 600000;    //600kbps
    
    //设置帧率
    (*enc_ctx)->time_base = (AVRational){1, 25};   //帧与帧之间的间隔是time_base
    (*enc_ctx)->framerate = (AVRational){25, 1};   //帧率，每秒25帧
    
    ret = avcodec_open2((*enc_ctx), codec, NULL);
    if (ret < 0) {
        printf("Could not open codec: %s!\n", av_err2str(ret));
        exit(1);
    }
}


static AVFrame* create_frame(int width, int height) {
    int ret = 0;
    AVFrame* frame = NULL;
    frame = av_frame_alloc();
    if (!frame) {
        printf("Error, No memory!\n");
        goto __ERROR;
    }

    //设置参数
    frame->width = width;
    frame->height = height;
    frame->format = AV_PIX_FMT_YUV420P;
    
    //alloc inner memory
    ret = av_frame_get_buffer(frame, 32);  //按32位对齐
    if(ret < 0) {
        printf("Error, Failed to alloc buffer for frame!\n");
        goto __ERROR;
    }
    return frame;
    
__ERROR:
    if (frame) {
        av_frame_free(&frame);
    }
    return NULL;
}

static void encode(AVCodecContext *enc_ctx,
                   AVFrame *frame,
                   AVPacket *newpkt,
                   FILE *outfile) {
    int ret = 0;
    
    if (frame) {
        printf("send frame to encoder, pts=%lld", frame->pts);
    }
    
    //送原始数据给编码器进行编码
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        printf("Error, Failed to send a frame for encoding\n");
        exit(1);
    }
    
    //从编码器获取编码好的数据
    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, newpkt);
        
        //如果编码器数据不足时会返回 EAGAIN，或者到数据尾时会返回 AVERROR_EOF
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if(ret < 0) {
            printf("Error, Failed to encode!\n");
            exit(1);
        }
        
        fwrite(newpkt->data, 1, newpkt->size, outfile);
        av_packet_unref(newpkt);
    }

}

void record_video() {
    int base = 0;
    
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *enc_ctx = NULL;

    //set log level
    av_log_set_level(AV_LOG_DEBUG);
    
    //register audio device
    avdevice_register_all();
    
    //start record 开始录制
    rec_status = 1;
    
    //create file
    char *youvout = "/Users/tianyang/Desktop/av_base/video.yuv";
    FILE *yuvoutFile = fopen(youvout, "wb+");
    
    char *out = "/Users/tianyang/Desktop/av_base/video.h264";
    FILE *outFile = fopen(out, "wb+");
    
    
    if(!yuvoutFile) {
        printf("Error, Failed to open file!\n");
        goto __ERROR;
    }
    
    //打开设备
    fmt_ctx = open_dev();
    if (!fmt_ctx) {
        printf("Error, Failed to open devide!\n");
        goto __ERROR;
    }
    
    //打开编码器
    open_encoder(V_WIDTH, V_HEIGHT, &enc_ctx);
    
    //创建AVFrame
    AVFrame* frame = create_frame(V_WIDTH, V_HEIGHT);
    
    //创建编码后输出的Packet
    AVPacket * newpkt = av_packet_alloc();
    if (!newpkt) {
        printf("Error, Failed to alloc avpacket!\n");
        goto __ERROR;
    }
    
    int ret = 0;
    //packet 从设备上读取的数据
    AVPacket pkt;

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

        printf("packet size is %d(%p)\n", pkt.size, pkt.data);

        
        //YYYYYYYYUVUV  NV12
        //YYYYYYYYUUVV  YUV420
        
        memcpy(frame->data[0], pkt.data, 307200);  //copy Y data  640*480 = 307200
        //307200之后，是UV
        for (int i=0; i < 307200/4; i++) {
            frame->data[1][i] = pkt.data[307200+i*2];
            frame->data[2][i] = pkt.data[307201+i*2];
        }
        
        
        //640*480*1.5    (宽 x 高) x (yuv420=1.5/ yuv422=2/ yuv444=3)
        //fwrite(pkt.data, 1, 460800, yuvoutFile);
        fwrite(frame->data[0], 1, 307200, yuvoutFile);
        fwrite(frame->data[1], 1, 307200/4, yuvoutFile);
        fwrite(frame->data[2], 1, 307200/4, yuvoutFile);
        fflush(yuvoutFile);
 
        //设置连续pts
        frame->pts = base++;
        encode(enc_ctx, frame, newpkt, outFile);
        
        //release packet 释放packet
        av_packet_unref(&pkt);
    }
    
    encode(enc_ctx, NULL, newpkt, outFile);
    
__ERROR:
    //close device and release ctx
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
    
    if (yuvoutFile) {
        //close file
        fclose(yuvoutFile);
    }
    
    av_log(NULL, AV_LOG_DEBUG, "finish!\n");
    
    return;
}
