
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <process.h> // windows threads
#include <windows.h> // HANDLE
#include <math.h>

extern "C"
{
    #include <libavutil/avassert.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/opt.h>
    #include <libavutil/mathematics.h>
    //#include <libavutil/timestamp.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
	#include <libavutil/buffer.h>
}

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "postproc.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "avutil.lib")

#define STREAM_DURATION   1000.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

#define SCALE_FLAGS SWS_BICUBIC

AVFormatContext *oc;
AVDictionary *optionsOutput = NULL;
int ret;
int have_video = 0;
int encode_video = 0;
AVDictionary *opt = NULL;
AVOutputFormat *fmt;

AVFormatContext *oc2;
AVDictionary *optionsOutput2 = NULL;
int have_video2 = 0;
int encode_video2 = 0;
AVDictionary *opt2 = NULL;
AVOutputFormat *fmt2;

// a wrapper around a single output AVStream
typedef struct OutputStream {
    AVStream *st;
    AVCodecContext *enc;

    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;

    AVFrame *frame;
    AVFrame *tmp_frame;

    float t, tincr, tincr2;

    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
} OutputStream;

OutputStream video_st = { 0 };
OutputStream video_st2 = { 0 };

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    //printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
    //    av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
    //    av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
    //    av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
    //    pkt->stream_index);
}

static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    /* Write the compressed frame to the media file. */
    log_packet(fmt_ctx, pkt);
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

/* Add an output stream. */
static void add_stream(OutputStream *ost, AVFormatContext *outCtx,
    AVCodec **codec,
enum AVCodecID codec_id)
{
    AVCodecContext *c;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
            avcodec_get_name(codec_id));
        exit(1);
    }

	ost->st = avformat_new_stream(outCtx, NULL);
    if (!ost->st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
	ost->st->id = outCtx->nb_streams - 1;
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        exit(1);
    }
    ost->enc = c;

    AVRational time_base;
    c->codec_id = codec_id;

    c->bit_rate = 400000;
    /* Resolution must be a multiple of two. */
    c->width = 352;
    c->height = 288;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
    * of which frame timestamps are represented. For fixed-fps content,
    * timebase should be 1/framerate and timestamp increments should be
    * identical to 1. */
    time_base = { 1, STREAM_FRAME_RATE };
    ost->st->time_base = time_base;
    c->time_base = ost->st->time_base;

    c->gop_size = 12; /* emit one intra frame every twelve frames at most */
    c->pix_fmt = STREAM_PIX_FMT;
    if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B-frames */
        c->max_b_frames = 2;
    }
    if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
        * This does not happen with normal video, it just happens here as
        * the motion of the chroma plane does not match the luma plane. */
        c->mb_decision = 2;
    }

    /* Some formats want stream headers to be separate. */
	if (outCtx->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

/**************************************************************/
/* video output */

static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame data.\n");
        exit(1);
    }

    return picture;
}

static void open_video(AVFormatContext *outCtx, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    int ret;
    AVCodecContext *c = ost->enc;
    AVDictionary *opt = NULL;

    av_dict_copy(&opt, opt_arg, 0);

    /* open the codec */
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open video codec\n");
        exit(1);
    }

    /* allocate and init a re-usable frame */
    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!ost->frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    /* If the output format is not YUV420P, then a temporary YUV420P
    * picture is needed too. It is then converted to the required
    * output format. */
    ost->tmp_frame = NULL;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!ost->tmp_frame) {
            fprintf(stderr, "Could not allocate temporary picture\n");
            exit(1);
        }
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }
}

/* Prepare a dummy image. */
static void fill_yuv_image(AVFrame *pict, int frame_index, int width, int height, uint8_t *buffer)
//static void fill_yuv_image(AVFrame *pict, int frame_index, int width, int height)
{
	pict->width = width;
	pict->height = height;
	pict->format = AV_PIX_FMT_YUV420P;
	
	//avpicture_fill((AVPicture*)pict, buffer, AV_PIX_FMT_YUV420P, pict->width, pict->height);
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

static AVFrame *get_video_frame(OutputStream *ost, uint8_t *buffer)
{
    AVCodecContext *c = ost->enc;
    AVRational tb_b = { 1, 1 };

    /* check if we want to generate more frames */
    if (av_compare_ts(ost->next_pts, c->time_base,
        STREAM_DURATION, tb_b) >= 0)
        return NULL;

    /* when we pass a frame to the encoder, it may keep a reference to it
    * internally; make sure we do not overwrite it here */
    if (av_frame_make_writable(ost->frame) < 0)
        exit(1);

    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        /* as we only generate a YUV420P picture, we must convert it
        * to the codec pixel format if needed */
        if (!ost->sws_ctx) {
            ost->sws_ctx = sws_getContext(c->width, c->height,
                AV_PIX_FMT_YUV420P,
                c->width, c->height,
                c->pix_fmt,
                SCALE_FLAGS, NULL, NULL, NULL);
            if (!ost->sws_ctx) {
                fprintf(stderr,
                    "Could not initialize the conversion context\n");
                exit(1);
            }
        }
		fill_yuv_image(ost->tmp_frame, ost->next_pts, c->width, c->height, buffer);
        sws_scale(ost->sws_ctx,
            (const uint8_t * const *)ost->tmp_frame->data, ost->tmp_frame->linesize,
            0, c->height, ost->frame->data, ost->frame->linesize);
    }
    else {
		fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height, buffer);
    }

    ost->frame->pts = ost->next_pts++;

    return ost->frame;
}

/*
* encode one video frame and send it to the muxer
* return 1 when encoding is finished, 0 otherwise
*/
static int write_video_frame(AVFormatContext *outCtx, OutputStream *ost, uint8_t *buffer)
{
    int ret;
    AVCodecContext *c;
    AVFrame *frame;
    int got_packet = 0;
    AVPacket pkt = { 0 };

    c = ost->enc;

    frame = get_video_frame(ost, buffer);

    av_init_packet(&pkt);

    /* encode the image */
    ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        fprintf(stderr, "Error encoding video frame\n");
        exit(1);
    }

    if (got_packet) {
		ret = write_frame(outCtx, &c->time_base, ost->st, &pkt);
    }
    else {
        ret = 0;
    }

    if (ret < 0) {
        fprintf(stderr, "Error while writing video frame\n");
        exit(1);
    }

    return (frame || got_packet) ? 0 : 1;
}

static void close_stream(AVFormatContext *outCtx, OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}

void outputThread0(void * ignored)
{
	av_dump_format(oc, 0, "http://172.26.186.80:30000", 1);
	// Open server
	if ((avio_open2(&oc->pb, "http://172.26.186.80:30000", AVIO_FLAG_WRITE, NULL, &optionsOutput)) < 0) {
		fprintf(stderr, "Failed to open server 0.\n");
		return;
	}
	fprintf(stderr, "Server 0 opened.\n");

	/* Write the stream header, if any. */
	ret = avformat_write_header(oc, &opt);
	if (ret < 0) {
		fprintf(stderr, "Error occurred when opening output file.\n");
		return;
	}

	uint8_t *buffer;

	while (encode_video) {
		/* select the stream to encode */
		encode_video = !write_video_frame(oc, &video_st, buffer);
	}

	/* Write the trailer, if any. The trailer must be written before you
	* close the CodecContexts open when you wrote the header; otherwise
	* av_write_trailer() may try to use memory that was freed on
	* av_codec_close(). */
	av_write_trailer(oc);

	/* Close each codec. */
	if (have_video)
		close_stream(oc, &video_st);

	if (!(fmt->flags & AVFMT_NOFILE))
		/* Close the output file. */
		avio_closep(&oc->pb);

	/* free the stream */
	avformat_free_context(oc);
}

void outputThread1(void * ignored)
{
	av_dump_format(oc2, 0, "http://172.26.186.80:30001", 1);
	// Open server
	if ((avio_open2(&oc2->pb, "http://172.26.186.80:30001", AVIO_FLAG_WRITE, NULL, &optionsOutput2)) < 0) {
		fprintf(stderr, "Failed to open server 1.\n");
		return;
	}
	fprintf(stderr, "Server 1 opened.\n");

	/* Write the stream header, if any. */
	ret = avformat_write_header(oc2, &opt2);
	if (ret < 0) {
		fprintf(stderr, "Error occurred when opening output file.\n");
		return;
	}

	uint8_t *buffer;

	while (encode_video2) {
		/* select the stream to encode */
		encode_video2 = !write_video_frame(oc2, &video_st2, buffer);
	}

	/* Write the trailer, if any. The trailer must be written before you
	* close the CodecContexts open when you wrote the header; otherwise
	* av_write_trailer() may try to use memory that was freed on
	* av_codec_close(). */
	av_write_trailer(oc2);

	/* Close each codec. */
	if (have_video2)
		close_stream(oc2, &video_st2);

	if (!(fmt2->flags & AVFMT_NOFILE))
		/* Close the output file. */
		avio_closep(&oc2->pb);

	/* free the stream */
	avformat_free_context(oc2);
}

/**************************************************************/
/* media file output */

int main(int argc, char **argv)
{
	const char *filename;
	AVCodec *video_codec;
    int i;

	const char *filename2;
	AVCodec *video_codec2;

    /* Initialize libavcodec, and register all codecs and formats. */
    av_register_all();
    // Global initialization of network components
    avformat_network_init();

    if (argc < 2) {
        printf("usage: %s output_file\n"
            "API example program to output a media file with libavformat.\n"
            "This program generates a synthetic audio and video stream, encodes and\n"
            "muxes them into a file named output_file.\n"
            "The output format is automatically guessed according to the file extension.\n"
            "Raw images can also be output by using '%%d' in the filename.\n"
            "\n", argv[0]);
        return 1;
    }

    filename = argv[1];
	filename2 = argv[1];
    for (i = 2; i + 1 < argc; i += 2) {
        if (!strcmp(argv[i], "-flags") || !strcmp(argv[i], "-fflags"))
            av_dict_set(&opt, argv[i] + 1, argv[i + 1], 0);
    }

    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    if (!oc) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    }
    if (!oc)
        return 1;

    fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
    * and initialize the codecs. */
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        add_stream(&video_st, oc, &video_codec, fmt->video_codec);
        have_video = 1;
        encode_video = 1;
    }

    /* Now that all the parameters are set, we can open the audio and
    * video codecs and allocate the necessary encode buffers. */
    if (have_video)
        open_video(oc, video_codec, &video_st, opt);

    // options. equivalent to "-listen 1"
    if ((ret = av_dict_set(&optionsOutput, "listen", "1", 0)) < 0) {
        fprintf(stderr, "Failed to set listen mode for server.\n");
        return ret;
    }

	// ------------- copy 2 -----------------
	/* allocate the output media context */
	avformat_alloc_output_context2(&oc2, NULL, NULL, filename2);
	if (!oc2) {
		printf("Could not deduce output format from file extension: using MPEG.\n");
		avformat_alloc_output_context2(&oc2, NULL, "mpeg", filename2);
	}
	if (!oc2)
		return 1;

	fmt2 = oc2->oformat;

	/* Add the audio and video streams using the default format codecs
	* and initialize the codecs. */
	if (fmt2->video_codec != AV_CODEC_ID_NONE) {
		add_stream(&video_st2, oc2, &video_codec2, fmt2->video_codec);
		have_video2 = 1;
		encode_video2 = 1;
	}

	/* Now that all the parameters are set, we can open the audio and
	* video codecs and allocate the necessary encode buffers. */
	if (have_video2)
		open_video(oc2, video_codec2, &video_st2, opt2);

	// options. equivalent to "-listen 1"
	if ((ret = av_dict_set(&optionsOutput2, "listen", "1", 0)) < 0) {
		fprintf(stderr, "Failed to set listen mode for server.\n");
		return ret;
	}

    // Single threaded open server

	HANDLE Thread0 = (HANDLE)_beginthread(outputThread0, 0, NULL);
	HANDLE Thread1 = (HANDLE)_beginthread(outputThread1, 0, NULL);
	WaitForSingleObject(Thread0, INFINITE);
	WaitForSingleObject(Thread1, INFINITE);

  // /* open the output file, if needed */
  // if (!(fmt->flags & AVFMT_NOFILE)) {
  //     ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
  //     if (ret < 0) {
  //         fprintf(stderr, "Could not open '%s'.\n", filename);
  //         return 1;
  //     }
  // }

	

    return 0;
}