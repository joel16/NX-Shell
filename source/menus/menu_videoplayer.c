/* Code heavily based on Minohh's SimplePlayer with some modifications
   My changes replaced all deprecated functions and allows the joypad to be used.
*/
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>

#include "SDL_helper.h"
#include "log.h"

#define DEF_SAMPLES 2048
#define DATATEST 30
#define FRAME_QUEUE_NUMBER 20

typedef struct PacketQueue{
    AVPacketList *first_pkt;
    AVPacketList *last_pkt;
    int nb_packets;
    int max_packets;
    int size;
    char * name;
    int abort_request;
    SDL_cond *cond_putable;
    SDL_cond *cond_getable;
    SDL_mutex *mutex;
} PacketQueue;

typedef struct FrameNode{
    AVFrame *frame;
} FrameNode;

typedef struct FrameQueue{
    FrameNode queue[FRAME_QUEUE_NUMBER];
    int read_index;
    int write_index;
    int nb;
    int max_nb;
    char *name;
    int abort_request;
    SDL_cond *writable_cond;
    SDL_cond *readable_cond;
    SDL_mutex *mutex;
} FrameQueue;

typedef struct RingBuffer{
    void *pHead;
    int rIndex;
    int wIndex;
    int len;
    int data_size;
    int abort_request;
    SDL_cond *cond;
    SDL_mutex *mutex;
} RingBuffer;

typedef struct SDL_Output{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *surface;  //not used yet
    SDL_Texture *texture;
    SDL_Rect rect;
    SDL_AudioDeviceID audio_dev;
    unsigned char *YPlane;
    unsigned char *UPlane;
    unsigned char *VPlane;
    int window_width;
    int window_height;
    int buf_size;
} SDL_Output;

typedef struct VideoState{
    s64 frame_cur_pts;
    s64 frame_last_pts;
    s64 cur_display_time;
    s64 last_display_time;
    s64 sleep_time;
    int is_first_frame;
    int last_frame_displayed;
    double time_base;
    AVFrame *cur_frame;
} VideoState;

typedef struct Codec{
    AVFormatContext *FCtx;
    AVCodecContext *CCtx;
    AVCodec *Codec;
    int stream;
} Codec;

typedef struct ReadThreadParam{
    AVFormatContext *FCtx;
    int AStream;
    int VStream;
} ReadThreadParam;

static int VideoStream, AudioStream;
static FrameQueue AFQ, VFQ;
static RingBuffer ring_buffer;
static PacketQueue APQ, VPQ;
static int read_finished;

static int packet_queue_init(PacketQueue *q, int max_packets, const char *name) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond_putable = SDL_CreateCond();
    q->cond_getable = SDL_CreateCond();
    q->max_packets = max_packets;
    q->abort_request = 0;
    q->name = strdup(name);
    return 0;
}

static int packet_queue_abort(PacketQueue *q) {
    SDL_LockMutex(q->mutex);
    q->abort_request = 1;
    SDL_CondSignal(q->cond_getable);
    SDL_CondSignal(q->cond_putable);
    SDL_UnlockMutex(q->mutex);
    return 0;
}

static int packet_queue_uninit(PacketQueue *q) {
    AVPacketList *pkt_node;

    for(; q->first_pkt;) {
        pkt_node = q->first_pkt;
        q->first_pkt = q->first_pkt->next;
        av_free(pkt_node);
    }

    SDL_DestroyMutex(q->mutex);
    SDL_DestroyCond(q->cond_putable);
    SDL_DestroyCond(q->cond_getable);
    memset(q, 0, sizeof(PacketQueue));
    return 0;
}

static int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
    AVPacketList *pkt_node;

    if (q->abort_request)
        return -1;

    pkt_node = av_malloc(sizeof(AVPacketList));
    if (!pkt_node) {
        return -1;
    }
    pkt_node->pkt = *pkt;
    pkt_node->next = NULL;

    SDL_LockMutex(q->mutex);
    if (q->nb_packets >= q->max_packets) {
        //fprintf(stdout, "%s packet nb :%d\n", q->name, q->nb_packets);
        SDL_CondWait(q->cond_putable, q->mutex);
    }

    if (q->abort_request) { //if queue is waiting cond_putable, it will come into here
        SDL_UnlockMutex(q->mutex);
        return -1;
    }

    if (!q->last_pkt) {
        q->first_pkt = pkt_node;
    }else{
        q->last_pkt->next = pkt_node;
    }
    q->last_pkt = pkt_node;
    q->nb_packets++;
    q->size += pkt_node->pkt.size;
    SDL_CondSignal(q->cond_getable);
    
    SDL_UnlockMutex(q->mutex);

    return 0;
}

static int packet_queue_get(PacketQueue *q, AVPacket *pkt) {
    AVPacketList *pkt_node;

    if (q->abort_request)
        return -1;

    SDL_LockMutex(q->mutex);
    if (q->nb_packets <= 0) {
        //fprintf(stdout, "%s packet nb is 0\n", q->name);
        SDL_CondWait(q->cond_getable, q->mutex);
    }
    
    if (q->abort_request) { //if queue is waiting cond_getable, it will come into here
        SDL_UnlockMutex(q->mutex);
        return -1;
    }

    pkt_node = q->first_pkt;
    q->first_pkt = q->first_pkt->next;
    q->nb_packets--;
    q->size -= pkt_node->pkt.size;
    if (!q->first_pkt) {
        q->last_pkt = NULL;
    }
    *pkt = pkt_node->pkt;
    av_free(pkt_node);
    SDL_CondSignal(q->cond_putable);

    SDL_UnlockMutex(q->mutex);
    return 0;
}

static int packet_queue_nb_packets(PacketQueue *q) {
    return q->nb_packets;
}

static int frame_queue_init(FrameQueue *frameq, const char *name) {
    int i;
    frameq->mutex = SDL_CreateMutex();
    frameq->writable_cond = SDL_CreateCond();
    frameq->readable_cond = SDL_CreateCond();
    frameq->max_nb = FRAME_QUEUE_NUMBER;
    frameq->name = strdup(name);
    frameq->write_index = 0;
    frameq->read_index = 0;
    frameq->nb = 0;
    frameq->abort_request = 0;
    for(i = 0; i < frameq->max_nb; i++)
        if (!(frameq->queue[i].frame = av_frame_alloc()))
            return AVERROR(ENOMEM);
    return 0;
}

static int frame_queue_uninit(FrameQueue *frameq) {
    int i;

    for(i = 0; i < frameq->max_nb; i++)
        av_free(frameq->queue[i].frame);

    SDL_DestroyMutex(frameq->mutex);
    SDL_DestroyCond(frameq->writable_cond);
    SDL_DestroyCond(frameq->readable_cond);
    memset(frameq, 0, sizeof(FrameQueue));
    return 0;
}

static int frame_queue_abort(FrameQueue *frameq) {
    SDL_LockMutex(frameq->mutex);
    SDL_CondSignal(frameq->readable_cond);
    SDL_CondSignal(frameq->writable_cond);
    frameq->abort_request = 1;
    SDL_UnlockMutex(frameq->mutex);
    return 0;
}

static int queue_frame(FrameQueue *frameq, FrameNode *fn) {
    FrameNode *f;

    if (frameq->abort_request)
        return -1;

    SDL_LockMutex(frameq->mutex);
    if (frameq->nb >= frameq->max_nb)
        SDL_CondWait(frameq->writable_cond, frameq->mutex);
    
    if (frameq->abort_request) { //if queue is waiting cond_writable, it will come into here
        SDL_UnlockMutex(frameq->mutex);
        return -1;
    }

    f = &frameq->queue[frameq->write_index];
    av_frame_move_ref(f->frame, fn->frame);
    //av_frame_unref(fn->frame);
    frameq->write_index++;
    frameq->nb++;
    if (frameq->write_index == frameq->max_nb)
        frameq->write_index = 0;
    SDL_CondSignal(frameq->readable_cond);
    SDL_UnlockMutex(frameq->mutex);

    return 0;
}

static int dequeue_frame(FrameQueue *frameq, FrameNode *fn) {
    FrameNode *f;
    
    if (frameq->abort_request)
        return -1;

    SDL_LockMutex(frameq->mutex);
    if (frameq->nb <= 0)
        SDL_CondWait(frameq->readable_cond, frameq->mutex);
    
    if (frameq->abort_request) { //if queue is waiting cond_readable, it will come into here
        SDL_UnlockMutex(frameq->mutex);
        return -1;
    }

    f = &frameq->queue[frameq->read_index];
    av_frame_move_ref(fn->frame, f->frame);
    //av_frame_unref(f->frame);
    frameq->read_index++;
    frameq->nb--;
    if (frameq->read_index == frameq->max_nb)
        frameq->read_index = 0;
    SDL_CondSignal(frameq->writable_cond);
    SDL_UnlockMutex(frameq->mutex);

    return 0;
}

static int frame_nb(FrameQueue *frameq) {
    return frameq->nb;
}

static void RB_Init(RingBuffer *rb, int len) {
    rb->pHead = malloc(len);
    rb->rIndex = 0;
    rb->wIndex = 0;
    rb->len = len;
    rb->data_size = 0;
    rb->abort_request = 0;
    rb->cond = SDL_CreateCond();
    rb->mutex = SDL_CreateMutex();
}

static void RB_Uninit(RingBuffer *rb) {

    free(rb->pHead);
    
    SDL_DestroyCond(rb->cond);
    SDL_DestroyMutex(rb->mutex);
    memset(rb, 0, sizeof(RingBuffer));
}

static int RB_abort(RingBuffer *rb) {

    SDL_LockMutex(rb->mutex);
    SDL_CondSignal(rb->cond);
    rb->abort_request = 1;
    SDL_UnlockMutex(rb->mutex);
    return 0;
}

static int RB_PushData(RingBuffer *rb, void *data, int size) {
    int free_size;
    int write_size;
    int w2t;

    if (rb->abort_request)
        return -1;

    SDL_LockMutex(rb->mutex);
    free_size = rb->len-rb->data_size;
    write_size = free_size>size ? size : free_size;
    if (!write_size) {
        SDL_CondWait(rb->cond, rb->mutex);
    }

    if (rb->abort_request) { // if RB is waiting cond it will come into here
        SDL_UnlockMutex(rb->mutex);
        return -1;
    }

    //fprintf(stdout, "wIndex=%d, free_size=%d, len=%d, data_size=%d\n", rb->wIndex, free_size, rb->len, rb->data_size);
    if (rb->wIndex+write_size<=rb->len) {
        //fprintf(stdout, "pHead=0x%x, wIndex addr=0x%x, data=0x%x\n", rb->pHead, &(rb->pHead[rb->wIndex]), data);
        memcpy(rb->pHead+rb->wIndex, data, write_size);
    }else{
        w2t = rb->len-rb->wIndex;
        memcpy(rb->pHead+rb->wIndex, data, w2t);
        memcpy(rb->pHead, data+w2t, write_size-w2t);
    }
    rb->wIndex = (rb->wIndex+write_size)%rb->len;
    rb->data_size += write_size;
    SDL_UnlockMutex(rb->mutex);
    return write_size;
}

static int RB_PullData(RingBuffer *rb, void *data, int size) {
    int read_size;
    int r2t;

    if (rb->abort_request)
        return -1;

    SDL_LockMutex(rb->mutex);
    read_size = rb->data_size>size ? size : rb->data_size;
    if (!read_size) {
        SDL_UnlockMutex(rb->mutex);
        return 0;
    }
    if (rb->rIndex+read_size<=rb->len) {
        memcpy(data, rb->pHead+rb->rIndex, read_size);
    }else{
        r2t = rb->len-rb->rIndex;
        memcpy(data, rb->pHead+rb->rIndex, r2t);
        memcpy(data+r2t, rb->pHead, read_size-r2t);
    }
    rb->rIndex = (rb->rIndex+read_size)%rb->len;
    rb->data_size -= read_size;
    SDL_CondSignal(rb->cond);
    SDL_UnlockMutex(rb->mutex);
    return read_size;
}

static void SimpleCallback(void* userdata, u8 *stream, int queryLen) {
    void *buf;
    int read_size = 0, len;
    
    len = queryLen;
    buf = (void *)stream;
    
    while(len > 0) {
        read_size = RB_PullData(&ring_buffer, buf, len);
        
        //fprintf(stdout, "frame.format = %d, chennels = %d, samplerate = %d, nb_samples=%d\n", fn.frame->format, fn.frame->channels, fn.frame->sample_rate, fn.frame->nb_samples);
        if (!read_size) {
            //fprintf(stdout, "no data to play\n");
            memset(stream+queryLen-len, 0, len);
        }else if (read_size<0) {
            memset(stream, 0, queryLen);
            break;
        }
        len = len - read_size;
        buf = buf + read_size;
    }
}

static int InitSDLAVOutput(SDL_Output *pOutput, int width, int height, int freq, int channels) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_AudioSpec wanted, obtained;
    unsigned char *YPlane;
    unsigned char *UPlane;
    unsigned char *VPlane;

    /*init SDL video display*/
    window = SDL_CreateWindow("Simple Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, 0);
    if (!window) {
        DEBUG_LOG("SDL create window failed\n");
        return -1;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        DEBUG_LOG("SDL create renderer failed\n");
        return -1;
    }

    for (int i = 0; i < 2; i++) {
    	if (SDL_JoystickOpen(i) == NULL) {
    		DEBUG_LOG("SDL_JoystickOpen: %s\n", SDL_GetError());
    		return -1;
    	}
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texture) {
        DEBUG_LOG("SDL create texture failed\n");
        return -1;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    YPlane = (unsigned char *)malloc(width*height);
    UPlane = (unsigned char *)malloc(width*height/4);
    VPlane = (unsigned char *)malloc(width*height/4);

    pOutput->window = window;
    pOutput->renderer = renderer;
    pOutput->texture = texture;
    pOutput->YPlane = YPlane;
    pOutput->UPlane = UPlane;
    pOutput->VPlane = VPlane;
    pOutput->window_width = width;
    pOutput->window_height = height;
    pOutput->buf_size = width*height;

    /*init SDL Audio Output*/
    memset(&wanted, 0, sizeof(wanted));
    wanted.freq = freq;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = channels;
    wanted.samples = DEF_SAMPLES;
    wanted.silence = 0;
    wanted.callback = SimpleCallback;

    pOutput->audio_dev = SDL_OpenAudioDevice(NULL, 0, &wanted, &obtained, 0);
    if (!pOutput->audio_dev) {
        DEBUG_LOG("SDL Open Audio failed, reason:%s\n", SDL_GetError());
        return -1;
    }

    return 0;
}

static void UninitSDLAVOutput(SDL_Output *pOutput) {
    if (!pOutput)
        return;
    if (pOutput->texture)
        SDL_DestroyTexture(pOutput->texture);
    if (pOutput->renderer)
        SDL_DestroyRenderer(pOutput->renderer);
    if (pOutput->window)
        SDL_DestroyWindow(pOutput->window);
    if (pOutput->YPlane)
        free(pOutput->YPlane);
    if (pOutput->UPlane)
        free(pOutput->UPlane);
    if (pOutput->VPlane)
        free(pOutput->VPlane);
    if (pOutput->audio_dev)
        SDL_CloseAudioDevice(pOutput->audio_dev);
}

static void DisplayFrame(SDL_Output *pOutput) {
    if (0!=SDL_UpdateYUVTexture(pOutput->texture, NULL, \
                pOutput->YPlane, pOutput->window_width, \
                pOutput->UPlane, pOutput->window_width/2, \
                pOutput->VPlane, pOutput->window_width/2)) {
        fprintf(stdout, "Render Update Texture failed, reason: %s\n", SDL_GetError());
    }
    SDL_RenderCopyEx(pOutput->renderer, pOutput->texture, NULL, NULL, 0, NULL, 0);
    SDL_RenderPresent(pOutput->renderer);
}

static int DecodeVideoFrame(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt) {
    int ret;

    *got_frame = 0;

    if (pkt) {
        ret = avcodec_send_packet(avctx, pkt);
        // In particular, we don't expect AVERROR(EAGAIN), because we read all
        // decoded frames with avcodec_receive_frame() until done.
        if (ret < 0)
            return ret == AVERROR_EOF ? 0 : ret;
    }

    ret = avcodec_receive_frame(avctx, frame);
    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
        return ret;
    if (ret >= 0)
        *got_frame = 1;

    return 0;
}

static int VideoThread(void *arg) {
    fprintf(stdout, "VideoThread start\n");
    Codec *c = arg;
    AVFrame *pFrame;
    AVCodecContext *pCodecCtx = c->CCtx;
    AVPacket packet;
    FrameNode fn;
    int frameFinished = 0;
    int ret;

    pFrame = av_frame_alloc();
    if (pFrame == NULL) {
        DEBUG_LOG("cannot get buffer of frame\n");
        return -1;
    }

    while(1) {
        ret = packet_queue_get(&VPQ, &packet);
        if (ret<0)
            break;

        //Decode video frame
        DecodeVideoFrame(pCodecCtx, pFrame, &frameFinished, &packet);
        av_packet_unref(&packet);
        if (frameFinished) {
            fn.frame = pFrame;
        
            ret = queue_frame(&VFQ, &fn);
            if (ret<0)
                break;
        }
    }

    av_packet_unref(&packet);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    
    fprintf(stdout, "VideoThread exit\n");
    return 0;
}

static int AudioThread(void *arg) {
    fprintf(stdout, "AudioThread start\n");
    Codec *c = arg;
    AVCodecContext *pCodecCtx = c->CCtx;
    AVPacket packet;
    AVFrame *pFrame = NULL;
    int Stream = c->stream;
    int i = 0, size = 0;
    void *buf=NULL, *ptr;
    short *itr;
    int frame_size, write_size, left_size;
    unsigned int sample_count;
    float *channel0, *channel1;
    short sample0, sample1;
    unsigned int audio_sleep;
    int ret;

    pFrame = av_frame_alloc();
    if (pFrame == NULL) {
        DEBUG_LOG("cannot get buffer of frame\n");
        return -1;
    }
    

    audio_sleep = (unsigned int)((240*DEF_SAMPLES/4)/(pCodecCtx->sample_rate)/2*1000.0);
    bool frameFinished = false;

    //Read from stream into packet
    while(1) {
        ret = packet_queue_get(&APQ, &packet);
        if (ret<0)
            break;
        //Only deal with the video stream of the type "videoStream"
        if (packet.stream_index==Stream) {
            //Decode audio frame
            ret = avcodec_receive_frame(pCodecCtx, pFrame);
            if (ret == 0)
 				frameFinished = true;
 		    if (ret == AVERROR(EAGAIN))
 		        ret = 0;
 		    if (ret == 0)
 		        ret = avcodec_send_packet(pCodecCtx, &packet);
 		    if (ret == AVERROR(EAGAIN))
 		        ret = 0;
            av_packet_unref(&packet);
            if (frameFinished) {
                size = av_samples_get_buffer_size(NULL, pCodecCtx->channels, pFrame->nb_samples, pCodecCtx->sample_fmt, 1);
                if (!buf)
                    buf = malloc(size);
                ptr = buf;
                itr = (short *)buf;
                
                sample_count = pFrame->nb_samples;
                channel0 = (float *)pFrame->data[0];
                channel1 = (float *)pFrame->data[1];
                //normal PCM is mixed track, but fltp "p" means planar
                if (pFrame->format == AV_SAMPLE_FMT_FLTP) 
                {
                    for(i=0; i<sample_count; i++) { //stereo 
                        sample0 = (short)(channel0[i]*32767.0f);
                        sample1 = (short)(channel1[i]*32767.0f);
                        itr[2*i] = sample0;
                        itr[2*i+1] = sample1;
                    }
                    frame_size = sample_count*4;
                }else{
                    memcpy(itr, pFrame->data[0], pFrame->linesize[0]);
                    frame_size = pFrame->linesize[0];
                }

                left_size = frame_size;
                while(left_size) {
                    write_size = RB_PushData(&ring_buffer, ptr, left_size);
                    if (write_size == 0)
                        SDL_Delay(audio_sleep);
                    else if (write_size<0)
                        break;
                    //fprintf(stdout, "new write_size = %d, audio_sleep=%d, ptr=0x%x\n", write_size, audio_sleep, ptr);
                    ptr += write_size;
                    left_size -= write_size;
                }
            }
        }
    }

    //free buffers
    if (buf)
        free(buf);

    av_packet_unref(&packet);
    av_free(pFrame);
    avcodec_close(pCodecCtx);

    fprintf(stdout, "AudioThread exit\n");
    return 0;
}

/**********************************************************
 * Param: type: AVMEDIA_TYPE_VIDEO/AVMEDIA_TYPE_AUDIO
 *        pFormatCtx: the AVFormatContext for codec init
 *        c: Codec for saving params
 * ********************************************************/
static int CodecInit(int type, AVFormatContext *pFormatCtx, Codec *c) {
    AVCodecContext *pCodecCtx = NULL;
    AVCodec *pCodec = NULL;
    int audioVideo = -1;
    int Stream = -1;

    if (!pFormatCtx)
        return -1;
    
    if (type == AVMEDIA_TYPE_AUDIO)
        audioVideo = 0;
    else if (type == AVMEDIA_TYPE_VIDEO)
        audioVideo = 1;
    
    //One file may have multipule streams ,only get the first video stream
    Stream=-1;
    Stream = av_find_best_stream(pFormatCtx, type, -1, -1, NULL, 0);
    
    if (Stream<0) {
        DEBUG_LOG("no video stream\n");
        return -1;
    }else{
        fprintf(stdout, "stream %d is %s\n", Stream, audioVideo?"video":"audio");
    }

    //copy param from format context to codec context
    pCodecCtx = avcodec_alloc_context3(NULL);
    if (avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[Stream]->codecpar)<0) {
        DEBUG_LOG("copy param from format context to codec context failed\n");
        return -1;
    }
    
    //find the codec
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec==NULL) {
        DEBUG_LOG("Unsupported codec,codec id %d\n", pCodecCtx->codec_id);
        return -1;
    }else{
        fprintf(stdout, "codec id is %d\n", pCodecCtx->codec_id);
    }

    //Open codec
    if (avcodec_open2(pCodecCtx, pCodec, NULL)<0) {
        DEBUG_LOG("open codec failed\n");
        return -1;
    }

    c->FCtx = pFormatCtx;
    c->CCtx = pCodecCtx;
    c->Codec = pCodec;
    c->stream = Stream;

    return 0;
}

static int ReadThread(void *arg) {
    fprintf(stdout, "ReadThread start\n");
    ReadThreadParam *param = arg;
    AVFormatContext *pFormatCtx = param->FCtx;
    int AudioStream = param->AStream;
    int VideoStream = param->VStream;
    int ret;

    AVPacket packet;
    while(1) {
        ret = av_read_frame(pFormatCtx, &packet);
        if (ret>=0) {
            if (packet.stream_index==AudioStream) {
                ret = packet_queue_put(&APQ, &packet);
            }else if (packet.stream_index==VideoStream) {
                ret = packet_queue_put(&VPQ, &packet);
            }
            if (ret<0)
                break;
        }else{
            //read finished
            //av_usleep(1000000);
            break;
        }
    }

    read_finished = 1;
    fprintf(stdout, "ReadThread exit\n");
    return 0;
}

static int Display(SDL_Output *Output, VideoState *vs) {
    FrameNode frameNode;
    s64 delay, pts_delay, time;
    int ret;

    //video display loop
    frameNode.frame = vs->cur_frame;
    if (vs->last_frame_displayed||vs->is_first_frame) {
        if (read_finished && !frame_nb(&VFQ))
            return 0;
        ret = dequeue_frame(&VFQ, &frameNode);
        if (ret<0)
            return -1;
    
        memcpy(Output->YPlane, vs->cur_frame->data[0], Output->buf_size);
        memcpy(Output->UPlane, vs->cur_frame->data[1], Output->buf_size/4);
        memcpy(Output->VPlane, vs->cur_frame->data[2], Output->buf_size/4);
            
        vs->frame_last_pts = vs->frame_cur_pts;
        vs->frame_cur_pts = vs->cur_frame->pts * vs->time_base * 1000000;
        pts_delay = vs->frame_cur_pts - vs->frame_last_pts;
        vs->cur_display_time = vs->last_display_time + pts_delay;
        vs->last_frame_displayed = 0;
    }

    time = av_gettime_relative();
    if (!vs->is_first_frame) {
        delay = vs->cur_display_time - time;

        if (delay <= 0) {
            vs->last_display_time = time;
            DisplayFrame(Output);
            av_frame_unref(vs->cur_frame);
            vs->last_frame_displayed = 1;
            vs->sleep_time = 0;
        }else if (delay > 10000) {
            vs->sleep_time = 10000;
        }else{
            vs->sleep_time = delay;
        }
    }else{
        vs->last_display_time = time;
        DisplayFrame(Output);
        av_frame_unref(vs->cur_frame);
        vs->last_frame_displayed = 1;
        vs->is_first_frame = 0;
        vs->sleep_time = 0;
    }
    return 0;
}

int Menu_PlayVideo(char *path) {
	ReadThreadParam param;
    Codec ACodec, VCodec;
    AVFormatContext *pFormatCtx = NULL;
    SDL_Output Output;
    int ret, duration_total_seconds, duration_hours, duration_minutes, duration_seconds;
    SDL_Event event;
    VideoState vs;
    SDL_Thread *read_tid, *audio_tid, *video_tid;
    //bool paused = false;

    DEBUG_LOG("%s:\n", path);

    //Open and get stream info
    pFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&pFormatCtx, path, NULL, NULL)!=0) {
        DEBUG_LOG("open input failed\n");
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL)<0) {
        DEBUG_LOG("find stream info failed\n");
        return -1;
    }

    av_dump_format(pFormatCtx, 0, path, 0);

    duration_total_seconds = (int)pFormatCtx->duration / AV_TIME_BASE;
    duration_hours = duration_total_seconds / 3600;
    duration_minutes = duration_total_seconds % 3600 / 60;
    duration_seconds = duration_total_seconds % 60;

    DEBUG_LOG("Duration: %02d:%02d:%02d, Bit Rate: %ld kb/s\n", duration_hours, duration_minutes, duration_seconds, pFormatCtx->bit_rate / 1000);

    if (CodecInit(AVMEDIA_TYPE_VIDEO, pFormatCtx, &VCodec)!=0)
        return -1;
    if (CodecInit(AVMEDIA_TYPE_AUDIO, pFormatCtx, &ACodec)!=0)
        return -1;

    //Init SDL
    ret = InitSDLAVOutput(&Output, VCodec.CCtx->width, VCodec.CCtx->height, ACodec.CCtx->sample_rate, ACodec.CCtx->channels);
    if (ret != 0) {
        DEBUG_LOG("init SDL output error:%s\n", SDL_GetError());
        UninitSDLAVOutput(&Output);
        return -1;
    }
   
    packet_queue_init(&APQ, 500, "audio queue");
    packet_queue_init(&VPQ, 300, "video queue");
    frame_queue_init(&VFQ, "video frame queue");
    RB_Init(&ring_buffer, 240*DEF_SAMPLES);
  
    param.FCtx = ACodec.FCtx;
    param.AStream = ACodec.stream;
    param.VStream = VCodec.stream;

    memset(&vs, 0, sizeof(VideoState));
    vs.time_base = av_q2d(VCodec.FCtx->streams[VCodec.stream]->time_base);
    vs.last_frame_displayed = 0;
    vs.is_first_frame = 1;
    vs.cur_frame = av_frame_alloc();

    read_tid    = SDL_CreateThread(ReadThread, "ReadThread", &param);
    video_tid   = SDL_CreateThread(VideoThread, "VideoThread", &VCodec);
    audio_tid   = SDL_CreateThread(AudioThread, "AudioThread", &ACodec);
    //start to play audio
    SDL_PauseAudioDevice(Output.audio_dev, 0);
    
    while(appletMainLoop()) {
        Display(&Output, &vs);
        SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
        switch(event.type) {
        	case SDL_JOYBUTTONDOWN:
        		//abort queue will cause threads break from loop
        		if (event.jbutton.which == 0 && event.jbutton.button == 1) {
            		packet_queue_abort(&APQ);
            		packet_queue_abort(&VPQ);
            		frame_queue_abort(&VFQ);
            		RB_abort(&ring_buffer);

            		SDL_WaitThread(read_tid, NULL);
            		SDL_WaitThread(video_tid, NULL);
            		SDL_WaitThread(audio_tid, NULL);

            		/* queue_abort : set queue->abort_request
            		 * queue->abort_request leads thread loop break
            		 * we should wait the thread loop return ,then uninit the queues
            		 * */
            		UninitSDLAVOutput(&Output);//close audio device will wait the callback thread return, 
            
            		packet_queue_uninit(&APQ);
            		packet_queue_uninit(&VPQ);
            		frame_queue_uninit(&VFQ);
            		RB_Uninit(&ring_buffer);
           
            		//audio codec close in AudioThread
            		//video codec close in VideoThread
            
            		av_free(vs.cur_frame);
            		avformat_close_input(&pFormatCtx);
            		goto exit;
            	}
            	break;
        	}
        av_usleep(vs.sleep_time);
        SDL_PumpEvents();
    }

    exit:
    	return 0;
}
