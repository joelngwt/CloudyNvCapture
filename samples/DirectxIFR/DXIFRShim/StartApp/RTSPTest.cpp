#include <chrono>
#include <thread>
#include <tchar.h>

extern "C"
{
#include <libavcodec\avcodec.h>
#include <libavformat\avformat.h>
#include <libavformat\avio.h>
#include <libswscale\swscale.h>
#include <libavutil\time.h>
}

#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"swresample.lib")

using namespace std;

static int video_is_eof;

#define STREAM_DURATION   20
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT   AV_PIX_FMT_YUV420P /* default pix_fmt */ //AV_PIX_FMT_NV12;

/* video output */
static AVFrame *frame;
static AVPicture src_picture, dst_picture;

/* Add an output stream. */
static AVStream *add_stream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id)
{
    AVCodecContext *c;
    AVStream *st;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        av_log(NULL, AV_LOG_ERROR, "Could not find encoder for '%s'.\n", avcodec_get_name(codec_id));
    }
    else {
        st = avformat_new_stream(oc, *codec);
        if (!st) {
            av_log(NULL, AV_LOG_ERROR, "Could not allocate stream.\n");
        }
        else {
            st->id = oc->nb_streams - 1;
            st->time_base.den = st->pts.den = 90000;
            st->time_base.num = st->pts.num = 1;

            c = st->codec;
            c->codec_id = codec_id;
            c->bit_rate = 400000;
            c->width = 352;
            c->height = 288;
            c->time_base.den = STREAM_FRAME_RATE;
            c->time_base.num = 1;
            c->gop_size = 12; /* emit one intra frame every twelve frames at most */
            c->pix_fmt = STREAM_PIX_FMT;
        }
    }

    return st;
}

static int open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st)
{
    int ret;
    AVCodecContext *c = st->codec;

    /* open the codec */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open video codec.\n", avcodec_get_name(c->codec_id));
    }
    else {

        /* allocate and init a re-usable frame */
        frame = av_frame_alloc();
        if (!frame) {
            av_log(NULL, AV_LOG_ERROR, "Could not allocate video frame.\n");
            ret = -1;
        }
        else {
            frame->format = c->pix_fmt;
            frame->width = c->width;
            frame->height = c->height;

            /* Allocate the encoded raw picture. */
            ret = avpicture_alloc(&dst_picture, c->pix_fmt, c->width, c->height);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Could not allocate picture.\n");
            }
            else {
                /* copy data and linesize picture pointers to frame */
                *((AVPicture *)frame) = dst_picture;
            }
        }
    }

    return ret;
}

/* Prepare a dummy image. */
static void fill_yuv_image(AVPicture *pict, int frame_index, int width, int height)
{
    int x, y, i;

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}

static int write_video_frame(AVFormatContext *oc, AVStream *st, int frameCount)
{
    int ret = 0;
    AVCodecContext *c = st->codec;

    fill_yuv_image(&dst_picture, frameCount, c->width, c->height);

    AVPacket pkt = { 0 };
    int got_packet;
    av_init_packet(&pkt);

    /* encode the image */
    frame->pts = frameCount;
    ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error encoding video frame.\n");
    }
    else {
        if (got_packet) {
            pkt.stream_index = st->index;
            pkt.pts = av_rescale_q_rnd(pkt.pts, c->time_base, st->time_base, AVRounding(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            ret = av_write_frame(oc, &pkt);

            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error while writing video frame.\n");
            }
        }
    }

    return ret;
}

int _tmainRTSP(int argc, _TCHAR* argv[])
{
    printf("starting...\n");

    const char *url = "http://137.132.83.206:30000";
    //const char *url = "rtsp://192.168.33.19:1935/ffmpeg/0";

    AVFormatContext *outContext;
    AVStream *video_st;
    AVCodec *video_codec;
    int ret = 0, frameCount = 0;

    //av_log_set_level(AV_LOG_DEBUG);
    av_log_set_level(AV_LOG_TRACE);

    av_register_all();
    avformat_network_init();

    avformat_alloc_output_context2(&outContext, NULL, "h264", url);
    fprintf(stderr, "avformat_alloc_output_context2.\n");

    if (!outContext) {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate an output context for '%s'.\n", url);
        goto end;
    }

    if (!outContext->oformat) {
        av_log(NULL, AV_LOG_FATAL, "Could not create the output format for '%s'.\n", url);
        goto end;
    }

    video_st = add_stream(outContext, &video_codec, AV_CODEC_ID_H264);
    fprintf(stderr, "Stream added.\n");

    /* Now that all the parameters are set, we can open the video codec and allocate the necessary encode buffers. */
    if (video_st) {
        av_log(NULL, AV_LOG_DEBUG, "Video stream codec %s.\n ", avcodec_get_name(video_st->codec->codec_id));

        ret = open_video(outContext, video_codec, video_st);
        if (ret < 0) {
            av_log(NULL, AV_LOG_FATAL, "Open video stream failed.\n");
            goto end;
        }
    }
    else {
        av_log(NULL, AV_LOG_FATAL, "Add video stream for the codec '%s' failed.\n", avcodec_get_name(AV_CODEC_ID_H264));
        goto end;
    }
    fprintf(stderr, "Video opened.\n");

    av_dump_format(outContext, 0, url, 1);
    fprintf(stderr, "Format dumped.\n");

    AVDictionary *options = NULL;
    if ((ret = av_dict_set(&options, "listen", "1", 0)) < 0) {
        fprintf(stderr, "Failed to set listen mode for server.\n");
        return ret;
    }

    ret = avformat_write_header(outContext, &options);
    fprintf(stderr, "Header written.\n");
    if (ret != 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to connect to RTSP server for '%s'.\n", url);
        goto end;
    }

    fprintf(stderr, "Start streaming...\n");

    while (video_st) {
        frameCount++;

        ret = write_video_frame(outContext, video_st, frameCount);

        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Write video frame failed.\n", url);
            goto end;
        }
    }

    if (video_st) {
        avcodec_close(video_st->codec);
        av_free(src_picture.data[0]);
        av_free(dst_picture.data[0]);
        av_frame_free(&frame);
    }

    avformat_free_context(outContext);

end:
    printf("finished.\n");

    getchar();

    return 0;
}