#pragma once

#include <string>
#include <process.h>
#include "NvIFREncoder.h"


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

class LibavcodecEncoder
{
public:
	LibavcodecEncoder();
	~LibavcodecEncoder();

	
	int write_video_frame(int index, uint8_t *buffer);
	void SetupFFMPEGServer(int playerIndex);
	void EndAndShutdown(int index);

private:
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

	// FFmpeg constants
	#define STREAM_FRAME_RATE 25 /* 25 images/s */
	#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */
	#define SCALE_FLAGS SWS_BICUBIC
	#define MAX_PLAYERS 12

	// Output video size. Nvidia capture will produce frames of this size
	const int bufferWidth = 1280;
	const int bufferHeight = 720;

	// Streaming IP address
	const std::string streamingIP = "http://137.132.82.160:";
	const int firstPort = 30000;

	// Arrays used by FFmpeg
	AVFormatContext *outCtxArray[MAX_PLAYERS];// = {};
	AVDictionary *optionsOutput[MAX_PLAYERS];// = {};
	int ret[MAX_PLAYERS];// = {};
	AVDictionary *opt[MAX_PLAYERS];// = {};
	AVOutputFormat *fmt[MAX_PLAYERS];// = {};

	OutputStream* video_st[MAX_PLAYERS];

	int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt);
	void add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id);
	AVFrame* alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
	void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
	void fill_yuv_image(AVFrame *pict, uint8_t *buffer);
	AVFrame* get_video_frame(OutputStream *ost, uint8_t *buffer);
	void close_stream(AVFormatContext *oc, OutputStream *ost);
};