#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>

#include <libavutil/avstring.h>
#include <libavutil/eval.h>
#include <libavutil/mathematics.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
#include <libavutil/dict.h>
#include <libavutil/parseutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/avassert.h>
#include <libavutil/time.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavcodec/avfft.h>
#include <libswresample/swresample.h>

#include <SDL2/SDL_thread.h>

#include "SDL_helper.h"
#include "log.h"

const char program_name[] = "ffplay";
const int program_birth_year = 2003;

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio
 * callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to
 * compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* polls for possible required screen refresh at least this often, should be
 * less than 1/fps */
#define REFRESH_RATE 0.01

static unsigned sws_flags = SWS_BICUBIC;

typedef struct MyAVPacketList {
    AVPacket pkt;
    struct MyAVPacketList *next;
    int serial;
} MyAVPacketList;

typedef struct PacketQueue {
    MyAVPacketList *first_pkt, *last_pkt;
    int nb_packets;     // total number of packets
    int size;           // total size of packets
    int64_t duration;   // totoal duration of packets
    int abort_request;  // quit
    int serial;         // index??
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, VIDEO_PICTURE_QUEUE_SIZE)

typedef struct AudioParams {
    int freq;  // sample rate
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} AudioParams;

typedef struct Clock {
    double pts;        /* clock base */
    double pts_drift;  /* clock base minus time at which we updated the clock */
    double last_updated;
    int serial;        /* clock is based on a packet with this serial */
    int paused;
    int *queue_serial; /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct Frame {
    AVFrame *frame;
    int serial;
    double pts;      /* presentation timestamp for the frame */
    double duration; /* estimated duration of the frame */
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
    int flip_v;
} Frame;

// ring buffer
typedef struct FrameQueue {
    Frame queue[FRAME_QUEUE_SIZE];
    int rindex;        // read index
    int windex;        // write index
    int size;          // frame count
    int max_size;      // queue capacity
    int rindex_shown;  // read index of displayed frame
    SDL_mutex *mutex;
    SDL_cond *cond;
    PacketQueue *pktq;
} FrameQueue;

typedef struct Decoder {
    AVPacket pkt;
    PacketQueue *queue;
    AVCodecContext *avctx;
    int pkt_serial;
    int finished;
    int packet_pending;
    SDL_cond *empty_queue_cond;
    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    SDL_Thread *decoder_tid;
} Decoder;

typedef struct VideoState {
    SDL_Thread *read_tid;
    int abort_request;
    int force_refresh;
    int paused;
    int last_paused;
    int queue_attachments_req;
    int read_pause_return;
    AVFormatContext *ic;
    
    Clock audclk;
    Clock vidclk;
    
    FrameQueue pictq;
    FrameQueue sampq;
    
    Decoder auddec;
    Decoder viddec;
    
    int audio_stream;
    
    double audio_clock;
    int audio_clock_serial;
    AVStream *audio_st;
    PacketQueue audioq;
    int audio_hw_buf_size; // sdl audio buffer size = samples * nb_channels * channel_layout
    uint8_t *audio_buf;
    uint8_t *audio_buf1;
    unsigned int audio_buf_size; /* in bytes */
    unsigned int audio_buf1_size;
    int audio_buf_index; /* in bytes */
    int audio_write_buf_size;
    struct AudioParams audio_src;
    struct AudioParams audio_tgt;
    struct SwrContext *swr_ctx;
    
    SDL_Texture *vid_texture;
    
    double frame_timer;
    int video_stream;
    AVStream *video_st;
    PacketQueue videoq;
    double max_frame_duration;  // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
    struct SwsContext *img_convert_ctx;
    int eof;
    
    char *filename;
    int width, height, xleft, ytop;
    
    SDL_cond *continue_read_thread;
} VideoState;

/* options specified by the user */
static const char *input_filename;
static const char *window_title;
static int default_width = 640;
static int default_height = 480;
static int screen_width = 0;
static int screen_height = 0;

/* current context */
static int64_t audio_callback_time;

static AVPacket flush_pkt;

#define FF_QUIT_EVENT (SDL_USEREVENT + 2)

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_RendererInfo renderer_info = {0};
static SDL_AudioDeviceID audio_dev;

static const struct TextureFormatEntry {
    enum AVPixelFormat format;
    int texture_fmt;
} sdl_texture_format_map[] = {
    {AV_PIX_FMT_RGB8, SDL_PIXELFORMAT_RGB332},
    {AV_PIX_FMT_RGB444, SDL_PIXELFORMAT_RGB444},
    {AV_PIX_FMT_RGB555, SDL_PIXELFORMAT_RGB555},
    {AV_PIX_FMT_BGR555, SDL_PIXELFORMAT_BGR555},
    {AV_PIX_FMT_RGB565, SDL_PIXELFORMAT_RGB565},
    {AV_PIX_FMT_BGR565, SDL_PIXELFORMAT_BGR565},
    {AV_PIX_FMT_RGB24, SDL_PIXELFORMAT_RGB24},
    {AV_PIX_FMT_BGR24, SDL_PIXELFORMAT_BGR24},
    {AV_PIX_FMT_0RGB32, SDL_PIXELFORMAT_RGB888},
    {AV_PIX_FMT_0BGR32, SDL_PIXELFORMAT_BGR888},
    {AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888},
    {AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888},
    {AV_PIX_FMT_RGB32, SDL_PIXELFORMAT_ARGB8888},
    {AV_PIX_FMT_RGB32_1, SDL_PIXELFORMAT_RGBA8888},
    {AV_PIX_FMT_BGR32, SDL_PIXELFORMAT_ABGR8888},
    {AV_PIX_FMT_BGR32_1, SDL_PIXELFORMAT_BGRA8888},
    {AV_PIX_FMT_YUV420P, SDL_PIXELFORMAT_IYUV},
    {AV_PIX_FMT_YUYV422, SDL_PIXELFORMAT_YUY2},
    {AV_PIX_FMT_UYVY422, SDL_PIXELFORMAT_UYVY},
    {AV_PIX_FMT_NONE, SDL_PIXELFORMAT_UNKNOWN},
};

static int packet_queue_put_private(PacketQueue *q, AVPacket *pkt) {
    MyAVPacketList *pkt1;
    
    if (q->abort_request) return -1;
    
    pkt1 = av_malloc(sizeof(MyAVPacketList));
    if (!pkt1) return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;
    if (pkt == &flush_pkt) q->serial++;
    pkt1->serial = q->serial;
    
    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size + sizeof(*pkt1);
    q->duration += pkt1->pkt.duration;
    /* XXX: should duplicate packet data in DV case */
    SDL_CondSignal(q->cond);
    return 0;
}

static int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
    int ret;
    
    SDL_LockMutex(q->mutex);
    ret = packet_queue_put_private(q, pkt);
    SDL_UnlockMutex(q->mutex);
    
    if (pkt != &flush_pkt && ret < 0) av_packet_unref(pkt);
    
    return ret;
}

// flush stream
static int packet_queue_put_nullpacket(PacketQueue *q, int stream_index) {
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;
    pkt->stream_index = stream_index;
    return packet_queue_put(q, pkt);
}

/* packet queue handling */
static int packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    if (!q->mutex) {
        DEBUG_LOG("SDL_CreateMutex(): %s\n", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    q->cond = SDL_CreateCond();
    if (!q->cond) {
        DEBUG_LOG("SDL_CreateCond(): %s\n", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    q->abort_request = 1;
    return 0;
}

static void packet_queue_flush(PacketQueue *q) {
    MyAVPacketList *pkt, *pkt1;
    
    SDL_LockMutex(q->mutex);
    for (pkt = q->first_pkt; pkt; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }
    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
    q->duration = 0;
    SDL_UnlockMutex(q->mutex);
}

static void packet_queue_destroy(PacketQueue *q) {
    packet_queue_flush(q);
    SDL_DestroyMutex(q->mutex);
    SDL_DestroyCond(q->cond);
}

static void packet_queue_abort(PacketQueue *q) {
    SDL_LockMutex(q->mutex);
    
    q->abort_request = 1;
    
    SDL_CondSignal(q->cond);
    
    SDL_UnlockMutex(q->mutex);
}

static void packet_queue_start(PacketQueue *q) {
    SDL_LockMutex(q->mutex);
    q->abort_request = 0;
    packet_queue_put_private(q, &flush_pkt);
    SDL_UnlockMutex(q->mutex);
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial) {
    MyAVPacketList *pkt1;
    int ret;
    
    SDL_LockMutex(q->mutex);
    for (;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }
        
        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt) q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size + sizeof(*pkt1);
            q->duration -= pkt1->pkt.duration;
            *pkt = pkt1->pkt;
            if (serial) *serial = pkt1->serial;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    
    return ret;
}

// 初始化解码器
static void decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, SDL_cond *empty_queue_cond) {
    memset(d, 0, sizeof(Decoder));
    d->avctx = avctx;
    d->queue = queue;
    d->empty_queue_cond = empty_queue_cond;
    d->start_pts = AV_NOPTS_VALUE;
    d->pkt_serial = -1;
}

// 解码 packet queue -> packet -> frame -> frame queue
static int decoder_decode_frame(Decoder *d, AVFrame *frame) {
    int ret = AVERROR(EAGAIN);
    
    for (;;) {
        AVPacket pkt;
        
        // 1 -1 // 首次不会执行，之后会一直执行
        // 1 1
        // 1 1
        // ...
        if (d->queue->serial == d->pkt_serial) {
            do {
                if (d->queue->abort_request) return -1;
                
                switch (d->avctx->codec_type) {
                    case AVMEDIA_TYPE_VIDEO:
                        ret = avcodec_receive_frame(d->avctx, frame);
                        if (ret >= 0) {
                            frame->pts = frame->best_effort_timestamp;
                        }
                        break;
                        
                    case AVMEDIA_TYPE_AUDIO:
                        ret = avcodec_receive_frame(d->avctx, frame);
                        if (ret >= 0) {
                            AVRational tb = (AVRational){1, frame->sample_rate};
                            if (frame->pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(frame->pts, d->avctx->pkt_timebase, tb);
                            else if (d->next_pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
                            if (frame->pts != AV_NOPTS_VALUE) {
                                d->next_pts = frame->pts + frame->nb_samples;
                                d->next_pts_tb = tb;
                            }
                        }
                        break;
                    default:
                        break;
                }
                if (ret == AVERROR_EOF) {
                    d->finished = d->pkt_serial;
                    avcodec_flush_buffers(d->avctx);
                    return 0;
                }
                if (ret >= 0) return 1;
            } while (ret != AVERROR(EAGAIN));
        }
        
        // queue->serial 初始值为 -1，往队列添加 flush_pkt 时变为 1，后续不再改变
        // d->pkt_serial 在调用 packet_queue_get 时赋值为 queue->serial
        do {
            if (d->queue->nb_packets == 0)  // 队列为空
                SDL_CondSignal(d->empty_queue_cond);
            if (d->packet_pending) {  // 异常情况，有待处理的包
                av_packet_move_ref(&pkt, &d->pkt);
                d->packet_pending = 0;
            } else {
                // 从队列获取包
                if (packet_queue_get(d->queue, &pkt, 1, &d->pkt_serial) < 0) return -1;
            }
        } while (d->queue->serial != d->pkt_serial);
        
        if (pkt.data == flush_pkt.data) {  // 取到 flush_pkt，目前仅在调用 packet_queue_start 时放入一个 flush_pkt
            avcodec_flush_buffers(d->avctx);
            d->finished = 0;
            d->next_pts = d->start_pts;
            d->next_pts_tb = d->start_pts_tb;
        } else {
            if (avcodec_send_packet(d->avctx, &pkt) == AVERROR(EAGAIN)) {
                DEBUG_LOG(
                       "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
                d->packet_pending = 1;
                av_packet_move_ref(&d->pkt, &pkt);
            }
            av_packet_unref(&pkt);
        }
    }
}

static void decoder_destroy(Decoder *d) {
    av_packet_unref(&d->pkt);
    avcodec_free_context(&d->avctx);
}

static void frame_queue_unref_item(Frame *vp) { av_frame_unref(vp->frame); }

static int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size) {
    int i;
    memset(f, 0, sizeof(FrameQueue));
    if (!(f->mutex = SDL_CreateMutex())) {
        DEBUG_LOG("SDL_CreateMutex(): %s\n", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    if (!(f->cond = SDL_CreateCond())) {
        DEBUG_LOG("SDL_CreateCond(): %s\n", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    f->pktq = pktq;
    f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
    for (i = 0; i < f->max_size; i++)
        if (!(f->queue[i].frame = av_frame_alloc())) return AVERROR(ENOMEM);
    return 0;
}

static void frame_queue_destory(FrameQueue *f) {
    int i;
    for (i = 0; i < f->max_size; i++) {
        Frame *vp = &f->queue[i];
        frame_queue_unref_item(vp);
        av_frame_free(&vp->frame);
    }
    SDL_DestroyMutex(f->mutex);
    SDL_DestroyCond(f->cond);
}

static void frame_queue_signal(FrameQueue *f) {
    SDL_LockMutex(f->mutex);
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

static Frame *frame_queue_peek(FrameQueue *f) {
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

static Frame *frame_queue_peek_next(FrameQueue *f) {
    return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

static Frame *frame_queue_peek_last(FrameQueue *f) {
    return &f->queue[f->rindex];
}

static Frame *frame_queue_peek_writable(FrameQueue *f) {
    /* wait until we have space to put a new frame */
    SDL_LockMutex(f->mutex);
    while (f->size >= f->max_size && !f->pktq->abort_request) {
        SDL_CondWait(f->cond, f->mutex);
    }
    SDL_UnlockMutex(f->mutex);
    
    if (f->pktq->abort_request) return NULL;
    
    return &f->queue[f->windex];
}

static Frame *frame_queue_peek_readable(FrameQueue *f) {
    /* wait until we have a readable a new frame */
    SDL_LockMutex(f->mutex);
    while (f->size - f->rindex_shown <= 0 && !f->pktq->abort_request) {
        SDL_CondWait(f->cond, f->mutex);
    }
    SDL_UnlockMutex(f->mutex);
    
    if (f->pktq->abort_request) return NULL;
    
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

static void frame_queue_push(FrameQueue *f) {
    if (++f->windex == f->max_size) f->windex = 0;
    SDL_LockMutex(f->mutex);
    f->size++;
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

static void frame_queue_next(FrameQueue *f) {
    if (!f->rindex_shown) {
        f->rindex_shown = 1;
        return;
    }
    
    frame_queue_unref_item(&f->queue[f->rindex]);
    if (++f->rindex == f->max_size) f->rindex = 0;
    SDL_LockMutex(f->mutex);
    f->size--;
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

/* return the number of undisplayed frames in the queue */
static int frame_queue_nb_remaining(FrameQueue *f) { return f->size - f->rindex_shown; }

// 终止解码操作
static void decoder_abort(Decoder *d, FrameQueue *fq) {
    packet_queue_abort(d->queue);
    frame_queue_signal(fq);
    SDL_WaitThread(d->decoder_tid, NULL);
    d->decoder_tid = NULL;
    packet_queue_flush(d->queue);
}

static int realloc_texture(SDL_Texture **texture, Uint32 new_format, int new_width, int new_height,
                           SDL_BlendMode blendmode) {
    Uint32 format;
    int access, w, h;
    if (!*texture || SDL_QueryTexture(*texture, &format, &access, &w, &h) < 0 || new_width != w || new_height != h ||
        new_format != format) {
        if (*texture) SDL_DestroyTexture(*texture);
        if (!(*texture = SDL_CreateTexture(renderer, new_format, SDL_TEXTUREACCESS_STREAMING, new_width, new_height)))
            return -1;
        if (SDL_SetTextureBlendMode(*texture, blendmode) < 0) return -1;
        DEBUG_LOG("Created %dx%d texture with %s.\n", new_width, new_height,
               SDL_GetPixelFormatName(new_format));
    }
    return 0;
}

// set_default_window_size => calculate_display_rect(&rect, 0, 0, INT_MAX, frame.height, frame.width, frame.height,
// sar); video_image_display => calculate_display_rect(&rect, is->xleft, is->ytop, is->width, is->height, vp->width,
// vp->height, vp->sar);
static void calculate_display_rect(SDL_Rect *rect, int scr_xleft, int scr_ytop, int scr_width, int scr_height,
                                   int pic_width, int pic_height, AVRational pic_sar) {
    float aspect_ratio;
    int width, height, x, y;
    
    if (pic_sar.num == 0)
        aspect_ratio = 0;
    else
        aspect_ratio = av_q2d(pic_sar);
    
    if (aspect_ratio <= 0.0) aspect_ratio = 1.0;
    aspect_ratio *= (float)pic_width / (float)pic_height;
    
    /* XXX: we suppose the screen has a 1.0 pixel ratio */
    height = scr_height;
    width = lrint(height * aspect_ratio) & ~1;
    if (width > scr_width) {
        width = scr_width;
        height = lrint(width / aspect_ratio) & ~1;
    }
    x = (scr_width - width) / 2;
    y = (scr_height - height) / 2;
    rect->x = scr_xleft + x;
    rect->y = scr_ytop + y;
    rect->w = FFMAX(width, 1);
    rect->h = FFMAX(height, 1);
}

static void get_sdl_pix_fmt_and_blendmode(int format, Uint32 *sdl_pix_fmt, SDL_BlendMode *sdl_blendmode) {
    int i;
    *sdl_blendmode = SDL_BLENDMODE_NONE;
    *sdl_pix_fmt = SDL_PIXELFORMAT_UNKNOWN;
    if (format == AV_PIX_FMT_RGB32 || format == AV_PIX_FMT_RGB32_1 || format == AV_PIX_FMT_BGR32 ||
        format == AV_PIX_FMT_BGR32_1)
        *sdl_blendmode = SDL_BLENDMODE_BLEND;
    for (i = 0; i < FF_ARRAY_ELEMS(sdl_texture_format_map) - 1; i++) {
        if (format == sdl_texture_format_map[i].format) {
            *sdl_pix_fmt = sdl_texture_format_map[i].texture_fmt;
            return;
        }
    }
}

static int upload_texture(SDL_Texture **tex, AVFrame *frame, struct SwsContext **img_convert_ctx) {
    int ret = 0;
    Uint32 sdl_pix_fmt;
    SDL_BlendMode sdl_blendmode;
    get_sdl_pix_fmt_and_blendmode(frame->format, &sdl_pix_fmt, &sdl_blendmode);
    
    ret = realloc_texture(tex, sdl_pix_fmt == SDL_PIXELFORMAT_UNKNOWN ? SDL_PIXELFORMAT_ARGB8888 : sdl_pix_fmt,
                          frame->width, frame->height, sdl_blendmode);
    if (ret < 0) return -1;
    
    switch (sdl_pix_fmt) {
        case SDL_PIXELFORMAT_UNKNOWN:
            /* This should only happen if we are not using avfilter... */
            *img_convert_ctx =
            sws_getCachedContext(*img_convert_ctx, frame->width, frame->height, frame->format, frame->width,
                                 frame->height, AV_PIX_FMT_BGRA, sws_flags, NULL, NULL, NULL);
            if (*img_convert_ctx != NULL) {
                uint8_t *pixels[4];
                int pitch[4];
                if (!SDL_LockTexture(*tex, NULL, (void **)pixels, pitch)) {
                    sws_scale(*img_convert_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0, frame->height,
                              pixels, pitch);
                    SDL_UnlockTexture(*tex);
                }
            } else {
                DEBUG_LOG("Cannot initialize the conversion context\n");
                ret = -1;
            }
            break;
            
        case SDL_PIXELFORMAT_IYUV:
            if (frame->linesize[0] > 0 && frame->linesize[1] > 0 && frame->linesize[2] > 0) {
                ret = SDL_UpdateYUVTexture(*tex, NULL, frame->data[0], frame->linesize[0], frame->data[1],
                                           frame->linesize[1], frame->data[2], frame->linesize[2]);
            } else if (frame->linesize[0] < 0 && frame->linesize[1] < 0 && frame->linesize[2] < 0) {
                ret = SDL_UpdateYUVTexture(
                                           *tex, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1), -frame->linesize[0],
                                           frame->data[1] + frame->linesize[1] * (AV_CEIL_RSHIFT(frame->height, 1) - 1), -frame->linesize[1],
                                           frame->data[2] + frame->linesize[2] * (AV_CEIL_RSHIFT(frame->height, 1) - 1), -frame->linesize[2]);
            } else {
                DEBUG_LOG("Mixed negative and positive linesizes are not supported.\n");
                return -1;
            }
            break;
            
        default:
            if (frame->linesize[0] < 0) {
                ret = SDL_UpdateTexture(*tex, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1),
                                        -frame->linesize[0]);
            } else {
                ret = SDL_UpdateTexture(*tex, NULL, frame->data[0], frame->linesize[0]);
            }
    }
    
    return ret;
}

static void set_sdl_yuv_conversion_mode(AVFrame *frame) {
    SDL_YUV_CONVERSION_MODE mode = SDL_YUV_CONVERSION_AUTOMATIC;
    if (frame && (frame->format == AV_PIX_FMT_YUV420P || frame->format == AV_PIX_FMT_YUYV422 ||
                  frame->format == AV_PIX_FMT_UYVY422)) {
        if (frame->color_range == AVCOL_RANGE_JPEG)
            mode = SDL_YUV_CONVERSION_JPEG;
        else if (frame->colorspace == AVCOL_SPC_BT709)
            mode = SDL_YUV_CONVERSION_BT709;
        else if (frame->colorspace == AVCOL_SPC_BT470BG || frame->colorspace == AVCOL_SPC_SMPTE170M ||
                 frame->colorspace == AVCOL_SPC_SMPTE240M)
            mode = SDL_YUV_CONVERSION_BT601;
    }
    SDL_SetYUVConversionMode(mode);
}

static void video_image_display(VideoState *is) {
    Frame *vp;
    SDL_Rect rect;
    
    vp = frame_queue_peek_last(&is->pictq);
    
    calculate_display_rect(&rect, is->xleft, is->ytop, is->width, is->height, vp->width, vp->height, vp->sar);
    
    if (!vp->uploaded) {
        if (upload_texture(&is->vid_texture, vp->frame, &is->img_convert_ctx) < 0) return;
        vp->uploaded = 1;
        vp->flip_v = vp->frame->linesize[0] < 0;
    }
    
    set_sdl_yuv_conversion_mode(vp->frame);
    SDL_RenderCopyEx(renderer, is->vid_texture, NULL, &rect, 0, NULL, vp->flip_v ? SDL_FLIP_VERTICAL : 0);
    set_sdl_yuv_conversion_mode(NULL);
}

static void stream_component_close(VideoState *is, int stream_index) {
    AVFormatContext *ic = is->ic;
    AVCodecParameters *codecpar;
    
    if (stream_index < 0 || stream_index >= ic->nb_streams) return;
    
    codecpar = ic->streams[stream_index]->codecpar;
    switch (codecpar->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            decoder_abort(&is->auddec, &is->sampq);
            SDL_CloseAudioDevice(audio_dev);
            decoder_destroy(&is->auddec);
            swr_free(&is->swr_ctx);
            av_freep(&is->audio_buf1);
            is->audio_buf1_size = 0;
            is->audio_buf = NULL;
            break;
        case AVMEDIA_TYPE_VIDEO:
            decoder_abort(&is->viddec, &is->pictq);
            decoder_destroy(&is->viddec);
            break;
        default:
            break;
    }
    
    ic->streams[stream_index]->discard = AVDISCARD_ALL;
    switch (codecpar->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            is->audio_st = NULL;
            is->audio_stream = -1;
            break;
        case AVMEDIA_TYPE_VIDEO:
            is->video_st = NULL;
            is->video_stream = -1;
            break;
            break;
        default:
            break;
    }
}

static void stream_close(VideoState *is) {
    /* XXX: use a special url_shutdown call to abort parse cleanly */
    is->abort_request = 1;
    SDL_WaitThread(is->read_tid, NULL);
    
    /* close each stream */
    if (is->audio_stream >= 0) stream_component_close(is, is->audio_stream);
    if (is->video_stream >= 0) stream_component_close(is, is->video_stream);
    
    avformat_close_input(&is->ic);
    
    packet_queue_destroy(&is->videoq);
    packet_queue_destroy(&is->audioq);
    
    /* free all pictures */
    frame_queue_destory(&is->pictq);
    frame_queue_destory(&is->sampq);
    SDL_DestroyCond(is->continue_read_thread);
    sws_freeContext(is->img_convert_ctx);
    av_free(is->filename);
    if (is->vid_texture) SDL_DestroyTexture(is->vid_texture);
    av_free(is);
}

static void do_exit(VideoState *is) {
    if (is) {
        stream_close(is);
    }
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    avformat_network_deinit();
    return;
}

static void sigterm_handler(int sig) { exit(123); }

static void set_default_window_size(int width, int height, AVRational sar) {
    SDL_Rect rect;
    calculate_display_rect(&rect, 0, 0, INT_MAX, height, width, height, sar);
    default_width = rect.w;
    default_height = rect.h;
}

static int video_open(VideoState *is) {
    int w, h;
    
    if (screen_width) {
        w = screen_width;
        h = screen_height;
    } else {
        w = default_width;
        h = default_height;
    }
    
    if (!window_title) window_title = input_filename;
    SDL_SetWindowTitle(window, window_title);
    
    SDL_SetWindowSize(window, w, h);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);
    
    is->width = w;
    is->height = h;
    
    return 0;
}

/* display the current picture, if any */
static void video_display(VideoState *is) {
    if (!is->width) video_open(is);
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    video_image_display(is);
    SDL_RenderPresent(renderer);
}

static double get_clock(Clock *c) {
    if (*c->queue_serial != c->serial) return NAN;
    if (c->paused) {
        return c->pts;
    } else {
        double time = av_gettime_relative() / 1000000.0;
        return c->pts_drift + time;
    }
}

static void set_clock_at(Clock *c, double pts, int serial, double time) {
    c->pts = pts;
    c->last_updated = time;
    c->pts_drift = c->pts - time;
    c->serial = serial;
}

static void set_clock(Clock *c, double pts, int serial) {
    double time = av_gettime_relative() / 1000000.0;
    set_clock_at(c, pts, serial, time);
}

static void init_clock(Clock *c, int *queue_serial) {
    c->paused = 0;
    c->queue_serial = queue_serial;
    set_clock(c, NAN, -1);
}

/* pause or resume the video */
static void stream_toggle_pause(VideoState *is) {
    if (is->paused) {
        is->frame_timer += av_gettime_relative() / 1000000.0 - is->vidclk.last_updated;
        if (is->read_pause_return != AVERROR(ENOSYS)) {
            is->vidclk.paused = 0;
        }
        set_clock(&is->vidclk, get_clock(&is->vidclk), is->vidclk.serial);
    }
    is->paused = is->audclk.paused = is->vidclk.paused = !is->paused;
}

static void toggle_pause(VideoState *is) {
    stream_toggle_pause(is);
}

static double compute_target_delay(double delay, VideoState *is) {
    double sync_threshold, diff = 0;
    
    /* update delay to follow master synchronisation source */
    /* if video is slave, we try to correct big delays by duplicating or
     * deleting a frame */
    diff = get_clock(&is->vidclk) - get_clock(&is->audclk);
    
    /* skip or repeat frame. We take into account the delay to compute the
     threshold. I still don't know if it is the best guess */
    sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
    if (!isnan(diff) && fabs(diff) < is->max_frame_duration) {
        if (diff <= -sync_threshold)
            delay = FFMAX(0, delay + diff);
        else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
            delay = delay + diff;
        else if (diff >= sync_threshold)
            delay = 2 * delay;
    }
    
    printf("video: delay=%0.3f A-V=%f\n", delay, -diff);
    
    return delay;
}

static double vp_duration(VideoState *is, Frame *vp, Frame *nextvp) {
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (isnan(duration) || duration <= 0 || duration > is->max_frame_duration)
            return vp->duration;
        else
            return duration;
    } else {
        return 0.0;
    }
}

static void update_video_pts(VideoState *is, double pts, int serial) {
    /* update current video pts */
    set_clock(&is->vidclk, pts, serial);
}

/* called to display each frame */
static void video_refresh(void *opaque, double *remaining_time) {
    VideoState *is = opaque;
    double time;
    
    if (is->video_st) {
    retry:
        if (frame_queue_nb_remaining(&is->pictq) == 0) {
            // nothing to do, no picture to display in the queue
        } else {
            double last_duration, duration, delay;
            Frame *vp, *lastvp;
            
            /* dequeue the picture */
            lastvp = frame_queue_peek_last(&is->pictq); // f->rindex
            vp = frame_queue_peek(&is->pictq); // f->rindex + f->rindex_shown
            
            if (vp->serial != is->videoq.serial) {
                frame_queue_next(&is->pictq);
                goto retry;
            }
            
            if (lastvp->serial != vp->serial)
                is->frame_timer = av_gettime_relative() / 1000000.0;
            
            if (is->paused)
                goto display;
            
            /* compute nominal last_duration */
            last_duration = vp_duration(is, lastvp, vp);
            delay = compute_target_delay(last_duration, is);
            
            time = av_gettime_relative() / 1000000.0;
            if (time < is->frame_timer + delay) {
                *remaining_time = FFMIN(is->frame_timer + delay - time, *remaining_time);
                goto display;
            }
            
            is->frame_timer += delay;
            if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX)
                is->frame_timer = time;
            
            SDL_LockMutex(is->pictq.mutex);
            if (!isnan(vp->pts))
                update_video_pts(is, vp->pts, vp->serial);
            SDL_UnlockMutex(is->pictq.mutex);
            
            // 丢帧？？
            if (frame_queue_nb_remaining(&is->pictq) > 1) {
                Frame *nextvp = frame_queue_peek_next(&is->pictq); // f->rindex + f->rindex_shown + 1
                duration = vp_duration(is, vp, nextvp);
                if (time > is->frame_timer + duration) {
                    frame_queue_next(&is->pictq);
                    goto retry;
                }
            }
            
            frame_queue_next(&is->pictq);
            is->force_refresh = 1;
        }
    display:
        /* display picture */
        if (is->force_refresh && is->pictq.rindex_shown)
            video_display(is);
    }
    is->force_refresh = 0;
}

static int queue_picture(VideoState *is, AVFrame *src_frame, double pts, double duration, int serial) {
    Frame *vp;
    
    if (!(vp = frame_queue_peek_writable(&is->pictq))) return -1;
    
    vp->sar = src_frame->sample_aspect_ratio;
    vp->uploaded = 0;
    
    vp->width = src_frame->width;
    vp->height = src_frame->height;
    vp->format = src_frame->format;
    
    vp->pts = pts;
    vp->duration = duration;
    vp->serial = serial;
    
    set_default_window_size(vp->width, vp->height, vp->sar);
    
    av_frame_move_ref(vp->frame, src_frame);
    frame_queue_push(&is->pictq);
    return 0;
}

static int get_video_frame(VideoState *is, AVFrame *frame) {
    int got_picture;
    
    if ((got_picture = decoder_decode_frame(&is->viddec, frame)) < 0)
        return -1;
    
    if (got_picture) {
        double dpts = NAN;
        
        if (frame->pts != AV_NOPTS_VALUE)
            dpts = av_q2d(is->video_st->time_base) * frame->pts;
        
        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);
        
        if (frame->pts != AV_NOPTS_VALUE) {
            double diff = dpts - get_clock(&is->audclk);
            if (!isnan(diff) &&
                fabs(diff) < AV_NOSYNC_THRESHOLD &&
                diff < 0 &&
                is->viddec.pkt_serial == is->vidclk.serial &&
                is->videoq.nb_packets) {
                av_frame_unref(frame);
                got_picture = 0;
            }
        }
    }
    
    return got_picture;
}

static int audio_thread(void *arg) {
    VideoState *is = arg;
    AVFrame *frame = av_frame_alloc();
    Frame *af;
    int got_frame = 0;
    AVRational tb;
    int ret = 0;
    
    if (!frame) return AVERROR(ENOMEM);
    
    do {
        if ((got_frame = decoder_decode_frame(&is->auddec, frame)) < 0) goto the_end;
        
        if (got_frame) {
            tb = (AVRational){1, frame->sample_rate};
            
            if (!(af = frame_queue_peek_writable(&is->sampq))) goto the_end;
            
            af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
            af->serial = is->auddec.pkt_serial;
            af->duration = av_q2d((AVRational){frame->nb_samples, frame->sample_rate});
            
            av_frame_move_ref(af->frame, frame);
            frame_queue_push(&is->sampq);
        }
    } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
the_end:
    av_frame_free(&frame);
    return ret;
}

// 启动解码线程
static int decoder_start(Decoder *d, int (*fn)(void *), void *arg) {
    packet_queue_start(d->queue);
    d->decoder_tid = SDL_CreateThread(fn, "decoder", arg);
    if (!d->decoder_tid) {
        DEBUG_LOG("SDL_CreateThread(): %s\n", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    return 0;
}

// 视频解码线程
static int video_thread(void *arg) {
    VideoState *is = arg;
    AVFrame *frame = av_frame_alloc();
    double pts;
    double duration;
    int ret;
    AVRational tb = is->video_st->time_base;
    AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);  // 帧率, e.g. 24/60
    
    if (!frame) return AVERROR(ENOMEM);
    
    for (;;) {
        ret = get_video_frame(is, frame);
        if (ret < 0) goto the_end;
        if (!ret) continue;
        
        duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
        pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
        ret = queue_picture(is, frame, pts, duration, is->viddec.pkt_serial);
        av_frame_unref(frame);
        
        if (ret < 0) goto the_end;
    }
the_end:
    av_frame_free(&frame);
    return 0;
}

/**
 * Decode one audio frame and return its uncompressed size.
 *
 * The processed audio frame is decoded, converted if required, and
 * stored in is->audio_buf, with size in bytes given by the return
 * value.
 */
static int audio_decode_frame(VideoState *is) {
    int data_size, resampled_data_size;
    int64_t dec_channel_layout;
    av_unused double audio_clock0;
    Frame *af;
    
    if (is->paused) return -1;
    
    do {
        if (!(af = frame_queue_peek_readable(&is->sampq)))
            return -1;
        frame_queue_next(&is->sampq);
    } while (af->serial != is->audioq.serial);
    
    data_size = av_samples_get_buffer_size(NULL, af->frame->channels, af->frame->nb_samples, af->frame->format, 1);
    
    dec_channel_layout = (af->frame->channel_layout &&
                          af->frame->channels == av_get_channel_layout_nb_channels(af->frame->channel_layout))
    ? af->frame->channel_layout
    : av_get_default_channel_layout(af->frame->channels);
    
    // 音频格式转换
    if (af->frame->format != is->audio_src.fmt || dec_channel_layout != is->audio_src.channel_layout ||
        af->frame->sample_rate != is->audio_src.freq ) {
        swr_free(&is->swr_ctx);
        is->swr_ctx = swr_alloc_set_opts(NULL, is->audio_tgt.channel_layout, is->audio_tgt.fmt, is->audio_tgt.freq,
                                         dec_channel_layout, af->frame->format, af->frame->sample_rate, 0, NULL);
        if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
            DEBUG_LOG(
                   "Cannot create sample rate converter for conversion of %d Hz %s "
                   "%d channels to %d Hz %s %d channels!\n",
                   af->frame->sample_rate, av_get_sample_fmt_name(af->frame->format), af->frame->channels,
                   is->audio_tgt.freq, av_get_sample_fmt_name(is->audio_tgt.fmt), is->audio_tgt.channels);
            swr_free(&is->swr_ctx);
            return -1;
        }
        is->audio_src.channel_layout = dec_channel_layout;
        is->audio_src.channels = af->frame->channels;
        is->audio_src.freq = af->frame->sample_rate;
        is->audio_src.fmt = af->frame->format;
    }
    
    if (is->swr_ctx) {
        const uint8_t **in = (const uint8_t **)af->frame->extended_data;
        uint8_t **out = &is->audio_buf1;
        int out_count = af->frame->nb_samples * is->audio_tgt.freq / af->frame->sample_rate + 256;
        int out_size = av_samples_get_buffer_size(NULL, is->audio_tgt.channels, out_count, is->audio_tgt.fmt, 0);
        int len2;
        if (out_size < 0) {
            DEBUG_LOG("av_samples_get_buffer_size() failed\n");
            return -1;
        }
        av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
        if (!is->audio_buf1) return AVERROR(ENOMEM);
        len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
        if (len2 < 0) {
            DEBUG_LOG("swr_convert() failed\n");
            return -1;
        }
        if (len2 == out_count) {
            DEBUG_LOG("audio buffer is probably too small\n");
            if (swr_init(is->swr_ctx) < 0) swr_free(&is->swr_ctx);
        }
        is->audio_buf = is->audio_buf1;
        resampled_data_size = len2 * is->audio_tgt.channels * av_get_bytes_per_sample(is->audio_tgt.fmt);
    } else {
        is->audio_buf = af->frame->data[0];
        resampled_data_size = data_size;
    }
    
    audio_clock0 = is->audio_clock;
    /* update the audio clock with the pts */
    if (!isnan(af->pts))
        is->audio_clock = af->pts + (double)af->frame->nb_samples / af->frame->sample_rate;  // pts + duration
    else
        is->audio_clock = NAN;
    is->audio_clock_serial = af->serial;
#ifdef DEBUG
    {
        static double last_clock;
        printf("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n", is->audio_clock - last_clock, is->audio_clock, audio_clock0);
        last_clock = is->audio_clock;
    }
#endif
    return resampled_data_size;
}

// 音频时间计算：audio_clock(frame pts + fram duration) - 缓冲区时间(当前缓冲区字节数/每秒播放的字节数)
/* prepare a new audio buffer */
static void sdl_audio_callback(void *opaque, Uint8 *stream, int len) {
    VideoState *is = opaque;
    int audio_size, len1;
    
    audio_callback_time = av_gettime_relative();
    
    while (len > 0) {
        // 数据不足，需要解码
        if (is->audio_buf_index >= is->audio_buf_size) {
            audio_size = audio_decode_frame(is);
            if (audio_size < 0) {
                /* if error, just output silence */
                is->audio_buf = NULL;
                is->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size * is->audio_tgt.frame_size;
            } else {
                is->audio_buf_size = audio_size;
            }
            is->audio_buf_index = 0;
        }
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len) len1 = len;
        memset(stream, 0, len1);
        if (is->audio_buf)
            SDL_MixAudioFormat(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1, SDL_MIX_MAXVOLUME);
        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
    }
    is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
    /* Let's assume the audio driver that is used by SDL has two periods. */
    if (!isnan(is->audio_clock)) {
        // 缓冲区可能有尚未播放的数据，需要减去这部分时间
        double pts = is->audio_clock - (double)(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / is->audio_tgt.bytes_per_sec;
        set_clock_at(&is->audclk, pts, is->audio_clock_serial, audio_callback_time / 1000000.0);
    }
}

static int audio_open(void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate,
                      AudioParams *audio_hw_params) {
    SDL_AudioSpec wanted_spec, spec;
    static const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
    static const int next_sample_rates[] = {0, 44100, 48000, 96000, 192000};
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;
    
    if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    wanted_spec.channels = wanted_nb_channels;
    wanted_spec.freq = wanted_sample_rate;
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        DEBUG_LOG("Invalid sample rate or channel count!\n");
        return -1;
    }
    
    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq) {
        next_sample_rate_idx--;
    }
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.silence = 0;
    wanted_spec.samples =
    FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    wanted_spec.callback = sdl_audio_callback;
    wanted_spec.userdata = opaque;
    while (!(audio_dev = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &spec,
                                             SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE))) {
        DEBUG_LOG("SDL_OpenAudio (%d channels, %d Hz): %s\n", wanted_spec.channels, wanted_spec.freq,
               SDL_GetError());
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if (!wanted_spec.channels) {
            wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
            wanted_spec.channels = wanted_nb_channels;
            if (!wanted_spec.freq) {
                DEBUG_LOG("No more combinations to try, audio open failed\n");
                return -1;
            }
        }
        wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
    }
    
    if (spec.format != AUDIO_S16SYS) {
        DEBUG_LOG("SDL advised audio format %d is not supported!\n", spec.format);
        return -1;
    }
    if (spec.channels != wanted_spec.channels) {
        wanted_channel_layout = av_get_default_channel_layout(spec.channels);
        if (!wanted_channel_layout) {
            DEBUG_LOG("SDL advised channel count %d is not supported!\n", spec.channels);
            return -1;
        }
    }
    
    audio_hw_params->fmt = AV_SAMPLE_FMT_S16;
    audio_hw_params->freq = spec.freq;
    audio_hw_params->channel_layout = wanted_channel_layout;
    audio_hw_params->channels = spec.channels;
    audio_hw_params->frame_size =
    av_samples_get_buffer_size(NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
    audio_hw_params->bytes_per_sec =
    av_samples_get_buffer_size(NULL, audio_hw_params->channels, audio_hw_params->freq, audio_hw_params->fmt, 1);
    if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
        DEBUG_LOG("av_samples_get_buffer_size failed\n");
        return -1;
    }
    return spec.size;
}

/* open a given stream. Return 0 if OK */
static int stream_component_open(VideoState *is, int stream_index) {
    AVFormatContext *ic = is->ic;
    AVCodecContext *avctx;
    AVCodec *codec;
    int ret = 0;
    
    if (stream_index < 0 || stream_index >= ic->nb_streams) return -1;
    
    // 0 - codec context
    avctx = avcodec_alloc_context3(NULL);
    if (!avctx) return AVERROR(ENOMEM);
    
    // 1 - set parameters
    ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
    if (ret < 0) goto fail;
    avctx->pkt_timebase = ic->streams[stream_index]->time_base;
    
    // 2 - find decoder
    codec = avcodec_find_decoder(avctx->codec_id);
    if (!codec) {
        DEBUG_LOG("No decoder could be found for codec %s\n", avcodec_get_name(avctx->codec_id));
        ret = AVERROR(EINVAL);
        goto fail;
    }
    
    // 3 - open codec
    if ((ret = avcodec_open2(avctx, codec, NULL)) < 0) goto fail;
    
    is->eof = 0;
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    switch (avctx->codec_type) {
        case AVMEDIA_TYPE_AUDIO:  // 音频流
            /* prepare audio output */
            if ((ret = audio_open(is, avctx->channel_layout, avctx->channels, avctx->sample_rate, &is->audio_tgt)) < 0)
                goto fail;
            is->audio_hw_buf_size = ret;
            is->audio_src = is->audio_tgt;
            is->audio_buf_size = 0;
            is->audio_buf_index = 0;
            
            is->audio_stream = stream_index;
            is->audio_st = ic->streams[stream_index];
            
            decoder_init(&is->auddec, avctx, &is->audioq, is->continue_read_thread);
            if ((is->ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) &&
                !is->ic->iformat->read_seek) {
                is->auddec.start_pts = is->audio_st->start_time;
                is->auddec.start_pts_tb = is->audio_st->time_base;
            }
            // 启动音频解码线程
            if ((ret = decoder_start(&is->auddec, audio_thread, is)) < 0) goto out;
            // 播放音频
            SDL_PauseAudioDevice(audio_dev, 0);
            break;
            
        case AVMEDIA_TYPE_VIDEO:  // 视频流
            is->video_stream = stream_index;
            is->video_st = ic->streams[stream_index];
            
            decoder_init(&is->viddec, avctx, &is->videoq, is->continue_read_thread);
            // 启动视频解码线程
            if ((ret = decoder_start(&is->viddec, video_thread, is)) < 0) goto out;
            is->queue_attachments_req = 1;
            break;
            
        default:
            break;
    }
    goto out;
    
fail:
    avcodec_free_context(&avctx);
out:
    return ret;
}

// 处理解码过程中的异常终止
static int decode_interrupt_cb(void *ctx) {
    VideoState *is = ctx;
    return is->abort_request;
}

// 当前队列是否已有足够多的数据
static int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue) {
    return (stream_id < 0 || queue->abort_request || st->disposition & AV_DISPOSITION_ATTACHED_PIC || queue->nb_packets > MIN_FRAMES) &&
    (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0);
}

/* this thread gets the stream from the disk or the network */
static int read_thread(void *arg) {
    VideoState *is = arg;
    AVFormatContext *ic = NULL;
    int err, ret;
    int st_index[AVMEDIA_TYPE_NB];
    AVPacket pkt1, *pkt = &pkt1;
    SDL_mutex *wait_mutex = SDL_CreateMutex();
    
    if (!wait_mutex) {
        DEBUG_LOG("SDL_CreateMutex(): %s\n", SDL_GetError());
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    
    memset(st_index, -1, sizeof(st_index));
    is->video_stream = -1;
    is->audio_stream = -1;
    is->eof = 0;
    
    // 0 - alloc format context
    ic = avformat_alloc_context();
    if (!ic) {
        DEBUG_LOG("Could not allocate context.\n");
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    // 注册解码器异常终止回调
    ic->interrupt_callback.callback = decode_interrupt_cb;
    ic->interrupt_callback.opaque = is;
    
    // 1 - open input
    err = avformat_open_input(&ic, is->filename, NULL, NULL);
    if (err < 0) {
        //print_error(is->filename, err);
        ret = -1;
        goto fail;
    }
    is->ic = ic;
    
    av_format_inject_global_side_data(ic);
    
    // 2 - find stream info
    err = avformat_find_stream_info(ic, NULL);
    if (err < 0) {
        DEBUG_LOG("%s: could not find codec parameters\n", is->filename);
        ret = -1;
        goto fail;
    }
    
    if (ic->pb) ic->pb->eof_reached = 0;  // FIXME hack, ffplay maybe should not use avio_feof() to test for the end
    
    is->max_frame_duration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
    
    // 输出格式信息
    av_dump_format(ic, 0, is->filename, 0);
    
    st_index[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
    st_index[AVMEDIA_TYPE_AUDIO] = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, st_index[AVMEDIA_TYPE_AUDIO],
                                                       st_index[AVMEDIA_TYPE_VIDEO], NULL, 0);
    
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        AVStream *st = ic->streams[st_index[AVMEDIA_TYPE_VIDEO]];
        AVCodecParameters *codecpar = st->codecpar;
        AVRational sar = av_guess_sample_aspect_ratio(ic, st, NULL);
        if (codecpar->width) set_default_window_size(codecpar->width, codecpar->height, sar);
    }
    
    /* open the streams */
    if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
        stream_component_open(is, st_index[AVMEDIA_TYPE_AUDIO]);
    }
    
    ret = -1;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        ret = stream_component_open(is, st_index[AVMEDIA_TYPE_VIDEO]);
    }
    
    if (is->video_stream < 0 && is->audio_stream < 0) {
        DEBUG_LOG("Failed to open file '%s' or configure filtergraph\n", is->filename);
        ret = -1;
        goto fail;
    }
    
    for (;;) {
        if (is->abort_request) break;
        
        if (is->paused != is->last_paused) {
            is->last_paused = is->paused;
            if (is->paused)
                is->read_pause_return = av_read_pause(ic);
            else
                av_read_play(ic);
        }
        
        if (is->queue_attachments_req) {
            // 如mp3的专辑封面
            if (is->video_st && is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                AVPacket copy = {0};
                if ((ret = av_packet_ref(&copy, &is->video_st->attached_pic)) < 0) goto fail;
                packet_queue_put(&is->videoq, &copy);
                packet_queue_put_nullpacket(&is->videoq, is->video_stream);
            }
            is->queue_attachments_req = 0;
        }
        
        /* if the queue are full, no need to read more */
        if (is->audioq.size + is->videoq.size > MAX_QUEUE_SIZE ||
            (stream_has_enough_packets(is->audio_st, is->audio_stream, &is->audioq) &&
             stream_has_enough_packets(is->video_st, is->video_stream, &is->videoq))) {
                /* wait 10 ms */
                SDL_LockMutex(wait_mutex);
                SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
                SDL_UnlockMutex(wait_mutex);
                continue;
            }
        
        // 播放结束
        if (!is->paused &&
            (!is->audio_st || (is->auddec.finished == is->audioq.serial && frame_queue_nb_remaining(&is->sampq) == 0)) &&
            (!is->video_st || (is->viddec.finished == is->videoq.serial && frame_queue_nb_remaining(&is->pictq) == 0))) {
            ret = AVERROR_EOF;
            goto fail;
        }
        
        ret = av_read_frame(ic, pkt);
        if (ret < 0) {
            if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof) {
                if (is->video_stream >= 0) packet_queue_put_nullpacket(&is->videoq, is->video_stream);
                if (is->audio_stream >= 0) packet_queue_put_nullpacket(&is->audioq, is->audio_stream);
                is->eof = 1;
            }
            if (ic->pb && ic->pb->error) break;
            SDL_LockMutex(wait_mutex);
            SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
            SDL_UnlockMutex(wait_mutex);
            continue;
        } else {
            is->eof = 0;
        }
        
        if (pkt->stream_index == is->audio_stream) {
            packet_queue_put(&is->audioq, pkt);
        } else if (pkt->stream_index == is->video_stream && !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
            packet_queue_put(&is->videoq, pkt);
        } else {
            av_packet_unref(pkt);
        }
    }
    
    ret = 0;
fail:
    if (ic && !is->ic) avformat_close_input(&ic);
    
    if (ret != 0) {
        SDL_Event event;
        
        event.type = FF_QUIT_EVENT;
        event.user.data1 = is;
        SDL_PushEvent(&event);
    }
    SDL_DestroyMutex(wait_mutex);
    return 0;
}

static VideoState *stream_open(const char *filename) {
    VideoState *is;
    
    is = av_mallocz(sizeof(VideoState));
    if (!is) return NULL;
    is->filename = av_strdup(filename);
    if (!is->filename) goto fail;
    is->ytop = 0;
    is->xleft = 0;
    
    /* start video display */
    if (frame_queue_init(&is->pictq, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE) < 0) goto fail;
    if (frame_queue_init(&is->sampq, &is->audioq, SAMPLE_QUEUE_SIZE) < 0) goto fail;
    
    if (packet_queue_init(&is->videoq) < 0 || packet_queue_init(&is->audioq) < 0) goto fail;
    
    if (!(is->continue_read_thread = SDL_CreateCond())) {
        DEBUG_LOG("SDL_CreateCond(): %s\n", SDL_GetError());
        goto fail;
    }
    
    init_clock(&is->vidclk, &is->videoq.serial);
    init_clock(&is->audclk, &is->audioq.serial);
    is->audio_clock_serial = -1;
    // 开始数据流读取
    is->read_tid = SDL_CreateThread(read_thread, "read_thread", is);
    if (!is->read_tid) {
        DEBUG_LOG("SDL_CreateThread(): %s\n", SDL_GetError());
    fail:
        stream_close(is);
        return NULL;
    }
    return is;
}

static void refresh_loop_wait_event(VideoState *is, SDL_Event *event) {
    double remaining_time = 0.0;
    // Pumps the event loop, gathering events from the input devices.
    SDL_PumpEvents();
    // Checks the event queue for messages and optionally returns them.
    while (!SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) {
        if (remaining_time > 0.0) av_usleep((int64_t)(remaining_time * 1000000.0));
        remaining_time = REFRESH_RATE;
        if (!is->paused || is->force_refresh) video_refresh(is, &remaining_time);
        SDL_PumpEvents();
    }
}

/* handle an event sent by the GUI */
static void event_loop(VideoState *cur_stream) {
    SDL_Event event;
    
    while(appletMainLoop()) {
        refresh_loop_wait_event(cur_stream, &event);
        switch (event.type) {
            case SDL_JOYBUTTONDOWN:
                if (event.jbutton.which == 0 && event.jbutton.button == 1) {
                    do_exit(cur_stream);
                    goto exit;
                }
                
                // If we don't yet have a window, skip all key events, because read_thread might still be
                // initializing...
                if (!cur_stream->width) continue;
                
                if (event.jbutton.which == 0 && event.jbutton.button == 0)
                    toggle_pause(cur_stream);
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                        screen_width = cur_stream->width = event.window.data1;
                        screen_height = cur_stream->height = event.window.data2;
                    case SDL_WINDOWEVENT_EXPOSED:
                        cur_stream->force_refresh = 1;
                }
                break;
            case SDL_QUIT:
            case FF_QUIT_EVENT:
                do_exit(cur_stream);
                goto exit;
                break;
            default:
                break;
        }
    }

exit:
    return;
}

int Menu_PlayVideo(char *path) {
    for (int i = 0; i < 2; i++) {
        if (SDL_JoystickOpen(i) == NULL) {
            DEBUG_LOG("SDL_JoystickOpen: %s\n", SDL_GetError());
            return -1;
        }
    }

    VideoState *is;
    
    /* register all codecs, demux and protocols */
    avformat_network_init();
    input_filename = path;
    
    signal(SIGINT, sigterm_handler);  /* Interrupt (ANSI).    */
    signal(SIGTERM, sigterm_handler); /* Termination (ANSI).  */
    
    /*if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        DEBUG_LOG("Could not initialize SDL - %s\n", SDL_GetError());
        DEBUG_LOG("(Did you set the DISPLAY variable?)\n");
        exit(1);
    }*/
    
    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
    SDL_EventState(SDL_USEREVENT, SDL_IGNORE);
    
    av_init_packet(&flush_pkt);
    flush_pkt.data = (uint8_t *)&flush_pkt;
    
    window = SDL_CreateWindow("simple player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, default_width,
                              default_height, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    if (window) {
        // Create a 2D rendering context for a window.
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            DEBUG_LOG("Failed to initialize a hardware accelerated renderer: %s\n", SDL_GetError());
            renderer = SDL_CreateRenderer(window, -1, 0);
        }
        if (renderer) {
            if (!SDL_GetRendererInfo(renderer, &renderer_info))
                DEBUG_LOG("Initialized %s renderer.\n", renderer_info.name);
        }
    }
    if (!window || !renderer || !renderer_info.num_texture_formats) {
        DEBUG_LOG("Failed to create window or renderer: %s", SDL_GetError());
        do_exit(NULL);
    }
    
    // 打开文件流
    is = stream_open(input_filename);
    if (!is) {
        DEBUG_LOG("Failed to initialize VideoState!\n");
        do_exit(NULL);
    }
    
    event_loop(is);
    
    /* never returns */
    
    return 0;
}
