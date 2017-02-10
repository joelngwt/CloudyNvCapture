/*!
 * \brief
 * Base class of NvIFREncoder for which the D3Dx Encoders are derived.
 * These classes are defined to expose an convenient NvIFR interface to the
 * standard D3D functions.
 *
 * \file
 *
 * This is a more advanced D3D sample showing how to use NVIFR to 
 * capture a render target to a file. It shows how to load the NvIFR dll, 
 * initialize the function pointers, and create the NvIFR object.
 * 
 * It then goes on to show how to set up the target buffers and events in 
 * the NvIFR object and then calls NvIFRTransferRenderTargetToH264HWEncoder to 
 * transfer the render target to the hardware encoder, and get the encoded 
 * stream.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

/* We must include d3d9.h here. NvIFRLibrary needs d3d9.h to be included before itself.*/
#include <d3d9.h>
#include <process.h>
#include <NvIFRLibrary.h>
#include "NvIFREncoder.h"
#include <thread>

extern "C"
{
	#include <libavutil/opt.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libswresample/swresample.h>
}

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "avutil.lib")

#pragma comment(lib, "winmm.lib")

extern simplelogger::Logger *logger;

// Nvidia GRID capture variables
#define NUMFRAMESINFLIGHT 1 // Limit is 3? Putting 4 causes an invalid parameter error to be thrown.
#define MAX_PLAYERS 12
HANDLE gpuEvent[MAX_PLAYERS];
uint8_t *bufferArray[MAX_PLAYERS];

// FFmpeg constants
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */
#define SCALE_FLAGS SWS_BICUBIC

// Output video size. Nvidia capture will produce frames of this size
const int bufferWidth = 1280;
const int bufferHeight = 720;

// Streaming IP address
const std::string streamingIP = "http://137.132.82.160:";
const int firstPort = 30000;

// Arrays used by FFmpeg
AVFormatContext *outCtxArray[MAX_PLAYERS] = {};
AVDictionary *optionsOutput[MAX_PLAYERS] = {};
int ret[MAX_PLAYERS] = {};
AVDictionary *opt[MAX_PLAYERS] = {};
AVOutputFormat *fmt[MAX_PLAYERS] = {};

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

OutputStream video_st[MAX_PLAYERS];

static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
	/* rescale output packet timestamp values from codec to stream timebase */
	av_packet_rescale_ts(pkt, *time_base, st->time_base);
	pkt->stream_index = st->index;

	/* Write the compressed frame to the media file. */
	return av_interleaved_write_frame(fmt_ctx, pkt);
}

/* Add an output stream. */
static void add_stream(OutputStream *ost, AVFormatContext *oc,
	AVCodec **codec,
enum AVCodecID codec_id)
{
	AVCodecContext *c;

	/* find the encoder */
	//*codec = avcodec_find_encoder(codec_id);
	*codec = avcodec_find_encoder_by_name("nvenc_h264");
	if (!(*codec)) {
		LOG_WARN(logger, "Could not find encoder for" << avcodec_get_name(codec_id));
		exit(1);
	}

	ost->st = avformat_new_stream(oc, NULL);
	if (!ost->st) {
		LOG_WARN(logger, "Could not allocate stream");
		exit(1);
	}
	ost->st->id = oc->nb_streams - 1;
	c = avcodec_alloc_context3(*codec);
	if (!c) {
		LOG_WARN(logger, "Could not alloc an encoding context");
		exit(1);
	}
	ost->enc = c;

    AVRational time_base = { 1, STREAM_FRAME_RATE };
	AVRational framerate = { STREAM_FRAME_RATE, 1 };

	c->codec_id = codec_id;
	c->bit_rate = 400000;
	/* Resolution must be a multiple of two. */
	c->width = bufferWidth;
	c->height = bufferHeight;
	/* timebase: This is the fundamental unit of time (in seconds) in terms
	* of which frame timestamps are represented. For fixed-fps content,
	* timebase should be 1/framerate and timestamp increments should be
	* identical to 1. */
	ost->st->time_base = time_base;
	c->time_base = ost->st->time_base;
	c->delay = 0;
	c->framerate = framerate;
	c->has_b_frames = 0;
	c->max_b_frames = 0;
	c->rc_min_vbv_overflow_use = 400000;
	c->thread_count = 1;

	c->gop_size = 30; /* emit one intra frame every twelve frames at most */
	c->pix_fmt = STREAM_PIX_FMT;
	if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
		/* just for testing, we also add B-frames */
		c->max_b_frames = 0; // original: 2
	}
	if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
		/* Needed to avoid using macroblocks in which some coeffs overflow.
		* This does not happen with normal video, it just happens here as
		* the motion of the chroma plane does not match the luma plane. */
		c->mb_decision = 2;
	}

	av_opt_set(c->priv_data, "preset", "ll", 0);
	//av_opt_set(c->priv_data, "tune", "zerolatency", 0);
	av_opt_set(c->priv_data, "delay", "0", 0);
	//av_opt_set(c->priv_data, "x264opts", "crf=2:vbv-maxrate=4000:vbv-bufsize=160:intra-refresh=1:slice-max-size=2000:keyint=30:ref=1", 0);

	/* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
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
		LOG_WARN(logger, "Could not allocate frame data");
		exit(1);
	}

	return picture;
}

static void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
	int ret;
	AVCodecContext *c = ost->enc;
	AVDictionary *opt = NULL;

	av_dict_copy(&opt, opt_arg, 0);

	/* open the codec */
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		LOG_WARN(logger, "Could not open video codec");
		exit(1);
	}

	/* allocate and init a re-usable frame */
	ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
	if (!ost->frame) {
		LOG_WARN(logger, "Could not allocate video frame");
		exit(1);
	}

	/* If the output format is not YUV420P, then a temporary YUV420P
	* picture is needed too. It is then converted to the required
	* output format. */
	ost->tmp_frame = NULL;
	if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
		ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
		if (!ost->tmp_frame) {
			LOG_WARN(logger, "Could not allocate temporary picture");
			exit(1);
		}
	}

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0) {
		LOG_WARN(logger, "Could not copy the stream parameters");
		exit(1);
	}
}

/* Prepare a dummy image. */
static void fill_yuv_image(AVFrame *pict, uint8_t *buffer)
{
	pict->width = bufferWidth; // This has to be the original dimensions of the original frame buffer
	pict->height = bufferHeight;

    // width and height parameters are the width and height of actual output.
    // Function requires top right corner coordinates.

	pict->format = AV_PIX_FMT_YUV420P; 

    avpicture_fill((AVPicture*)pict, buffer, AV_PIX_FMT_YUV420P, pict->width, pict->height);
}

static AVFrame* get_video_frame(OutputStream *ost, uint8_t *buffer)
{
	AVCodecContext *c = ost->enc;

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
				LOG_WARN(logger, "Could not initialize the conversion context");
				exit(1);
			}
		}
        fill_yuv_image(ost->tmp_frame, buffer);
		sws_scale(ost->sws_ctx,
			(const uint8_t * const *)ost->tmp_frame->data, ost->tmp_frame->linesize,
			0, c->height, ost->frame->data, ost->frame->linesize);
	}
	else {
        fill_yuv_image(ost->frame, buffer);
	}

	ost->frame->pts = ost->next_pts++;

	return ost->frame;
}

/*
* encode one video frame and send it to the muxer
* return 1 when encoding is finished, 0 otherwise
*/
static int write_video_frame(AVFormatContext *oc, OutputStream *ost, uint8_t *buffer)
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
		LOG_WARN(logger, "Error encoding video frame");
		exit(1);
	}

	if (got_packet) {
		ret = write_frame(oc, &c->time_base, ost->st, &pkt);
	}
	else {
		ret = 0;
		LOG_WARN(logger, "No packet");
	}

	if (ret < 0) {
		// This happens when the thin client is closed. This will close the game.
		LOG_WARN(logger, "Error while writing video frame. Thin client was probably closed.");
		_endthread();
	}

	return (frame || got_packet) ? 0 : 1;
}

static void close_stream(AVFormatContext *oc, OutputStream *ost)
{
	avcodec_free_context(&ost->enc);
	av_frame_free(&ost->frame);
	av_frame_free(&ost->tmp_frame);
	sws_freeContext(ost->sws_ctx);
	swr_free(&ost->swr_ctx);
}


BOOL NvIFREncoder::StartEncoder(int index) 
{
	hevtStopEncoder = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!hevtStopEncoder) {
		LOG_ERROR(logger, "Failed to create hevtStopEncoder");
		return FALSE;
	}
	bStopEncoder = FALSE;

	hevtInitEncoderDone = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!hevtInitEncoderDone) {
		LOG_ERROR(logger, "Failed to create hevtInitEncoderDone");
		return FALSE;
	}
	bInitEncoderSuccessful = FALSE;

	indexToUse = index;
	hthEncoder = (HANDLE)_beginthread(EncoderThreadStartProc, 0, this);

	if (!hthEncoder) {
		return FALSE;
	}

	WaitForSingleObject(hevtInitEncoderDone, INFINITE);
	CloseHandle(hevtInitEncoderDone);
	hevtInitEncoderDone = NULL;

	return bInitEncoderSuccessful;
}

void NvIFREncoder::StopEncoder() 
{
	if (bStopEncoder || !hevtStopEncoder || !hthEncoder) {
		return;
	}

	bStopEncoder = TRUE;
	SetEvent(hevtStopEncoder);
	WaitForSingleObject(hthEncoder, INFINITE);
	CloseHandle(hevtStopEncoder);
	hevtStopEncoder = NULL;
}

void NvIFREncoder::SetupFFMPEGServer(int playerIndex)
{
	const char *filename = NULL;
	AVCodec *video_codec;

	/* Initialize libavcodec, and register all codecs and formats. */
	av_register_all();
	// Global initialization of network components
	avformat_network_init();

	/* allocate the output media context */
	avformat_alloc_output_context2(&outCtxArray[playerIndex], NULL, NULL, "output.h264");
	if (!outCtxArray[playerIndex]) {
		LOG_WARN(logger, "Could not deduce output format from file extension: using h264.");
		avformat_alloc_output_context2(&outCtxArray[playerIndex], NULL, "h264", filename);
	}
	if (!outCtxArray[playerIndex]) {
		LOG_WARN(logger, "No output context.");
		return;
	}

	fmt[playerIndex] = outCtxArray[playerIndex]->oformat;

	/* Add the audio and video streams using the default format codecs
	* and initialize the codecs. */
	if (fmt[playerIndex]->video_codec != AV_CODEC_ID_NONE) {
		add_stream(&video_st[playerIndex], outCtxArray[playerIndex], &video_codec, fmt[playerIndex]->video_codec);
	}

	if ((ret[playerIndex] = av_dict_set(&opt[playerIndex], "re", "", 0)) < 0) {
		LOG_WARN(logger, "Failed to set -re mode.");
		return;
	}

	/* Now that all the parameters are set, we can open the audio and
	* video codecs and allocate the necessary encode buffers. */
	open_video(outCtxArray[playerIndex], video_codec, &video_st[playerIndex], opt[playerIndex]);
	av_dump_format(outCtxArray[playerIndex], 0, filename, 1);

	AVDictionary *optionsOutput[MAX_PLAYERS] = {};

	if ((ret[playerIndex] = av_dict_set(&optionsOutput[playerIndex], "listen", "1", 0)) < 0) {
		LOG_WARN(logger, "Failed to set listen mode for server.");
		return;
	}

	if ((ret[playerIndex] = av_dict_set(&optionsOutput[playerIndex], "an", "", 0)) < 0) {
		LOG_WARN(logger, "Failed to set -an mode.");
		return;
	}

	std::stringstream *HTTPUrl = new std::stringstream();
	*HTTPUrl << streamingIP << firstPort + playerIndex;

	// Open server
	if ((avio_open2(&outCtxArray[playerIndex]->pb, HTTPUrl->str().c_str(), AVIO_FLAG_WRITE, NULL, &optionsOutput[playerIndex])) < 0) {
		LOG_ERROR(logger, "Failed to open server " << playerIndex << ".");
		return;
	}
	LOG_DEBUG(logger, "Server " << playerIndex << " opened at " << HTTPUrl->str());

	/* Write the stream header, if any. */
	ret[playerIndex] = avformat_write_header(outCtxArray[playerIndex], &opt[playerIndex]);
	if (ret[playerIndex] < 0) {
		LOG_ERROR(logger, "Error occurred when opening output file.\n");
		return;
	}
}

void NvIFREncoder::EncoderThreadProc(int index) 
{
	/*Note: 
	1. The D3D device for encoding must be create on a seperate thread other than the game rendering thread. 
	Otherwise, some games (such as Mass Effect 2) will run abnormally. That's why SetupNvIFR()
	is called here instead of inside the subclass constructor.
	2. The D3D device (or swapchain) and the window bound with it must be created in 
	the same thread, or you get D3DERR_INVALIDCALL.*/
	if (!SetupNvIFR()) {
		LOG_ERROR(logger, "Failed to setup NvIFR.");
		SetEvent(hevtInitEncoderDone);
		CleanupNvIFR();
		return;
	}
    
	NVIFR_TOSYS_SETUP_PARAMS params = { 0 }; 
	params.dwVersion = NVIFR_TOSYS_SETUP_PARAMS_VER; 
	params.eFormat = NVIFR_FORMAT_YUV_420;
	params.eSysStereoFormat = NVIFR_SYS_STEREO_NONE; 
	params.dwNBuffers = NUMFRAMESINFLIGHT; 
	//params.dwTargetWidth = bufferWidth; // this is just scaling. No point wasting work on this.
	//params.dwTargetHeight = bufferHeight;
	params.ppPageLockedSysmemBuffers = &bufferArray[index];
	params.ppTransferCompletionEvents = &gpuEvent[index];
	 
	NVIFRRESULT nr = pIFR->NvIFRSetUpTargetBufferToSys(&params);
    
	if (nr != NVIFR_SUCCESS) {
		LOG_ERROR(logger, "NvIFRSetUpTargetBufferToSys failed, nr=" << nr);
		SetEvent(hevtInitEncoderDone);
		CleanupNvIFR();
		return;
	}
	LOG_DEBUG(logger, "NvIFRSetUpTargetBufferToSys succeeded");

    bInitEncoderSuccessful = TRUE;
    SetEvent(hevtInitEncoderDone);

	SetupFFMPEGServer(index);

    while (!bStopEncoder)
    {
        if (!UpdateBackBuffer())
        {
            LOG_DEBUG(logger, "UpdateBackBuffer() failed");
        }
   
        NVIFRRESULT res = pIFR->NvIFRTransferRenderTargetToSys(0);
   
        if (res == NVIFR_SUCCESS)
        {
            DWORD dwRet = WaitForSingleObject(gpuEvent[index], INFINITE);
            if (dwRet != WAIT_OBJECT_0)// If not signalled
            {
                if (dwRet != WAIT_OBJECT_0 + 1)
                {
                    LOG_WARN(logger, "Abnormally break from encoding loop, dwRet=" << dwRet);
                }
                return;
            }
   
			write_video_frame(outCtxArray[index], &video_st[index], bufferArray[index]);
            
            ResetEvent(gpuEvent[index]);
        }
        else
        {
            LOG_ERROR(logger, "NvIFRTransferRenderTargetToSys failed, res=" << res);
        }
        // Prevent doing extra work (25 FPS)
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    LOG_DEBUG(logger, "Quit encoding loop");

    /* Write the trailer, if any. The trailer must be written before you
        * close the CodecContexts open when you wrote the header; otherwise
        * av_write_trailer() may try to use memory that was freed on
        * av_codec_close(). */
    av_write_trailer(outCtxArray[index]);

    /* Close each codec. */
	close_stream(outCtxArray[index], &video_st[index]);

	if (!(fmt[index]->flags & AVFMT_NOFILE))
        /* Close the output file. */
		avio_closep(&outCtxArray[index]->pb);

    /* free the stream */
	avformat_free_context(outCtxArray[index]);
 
	CleanupNvIFR();
}

Streamer * NvIFREncoder::pSharedStreamer = NULL;
