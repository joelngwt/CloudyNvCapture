#include "LibavcodecEncoder.h"

extern simplelogger::Logger *logger;

int LibavcodecEncoder::write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
	/* rescale output packet timestamp values from codec to stream timebase */
	av_packet_rescale_ts(pkt, *time_base, st->time_base);
	pkt->stream_index = st->index;

	/* Write the compressed frame to the media file. */
	return av_interleaved_write_frame(fmt_ctx, pkt);
}

/* Add an output stream. */
void LibavcodecEncoder::add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id)
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

AVFrame* LibavcodecEncoder::alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
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

void LibavcodecEncoder::open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
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
void LibavcodecEncoder::fill_yuv_image(AVFrame *pict, uint8_t *buffer)
{
	pict->width = bufferWidth; // This has to be the original dimensions of the original frame buffer
	pict->height = bufferHeight;

	// width and height parameters are the width and height of actual output.
	// Function requires top right corner coordinates.

	pict->format = AV_PIX_FMT_YUV420P;

	avpicture_fill((AVPicture*)pict, buffer, AV_PIX_FMT_YUV420P, pict->width, pict->height);
}

AVFrame* LibavcodecEncoder::get_video_frame(OutputStream *ost, uint8_t *buffer)
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
int LibavcodecEncoder::write_video_frame(int index, uint8_t *buffer)
{
	int ret;
	AVCodecContext *c;
	AVFrame *frame;
	int got_packet = 0;
	AVPacket pkt = { 0 };

	c = video_st[index]->enc;

	frame = get_video_frame(video_st[index], buffer);

	av_init_packet(&pkt);

	/* encode the image */
	ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
	if (ret < 0) {
		LOG_WARN(logger, "Error encoding video frame");
		exit(1);
	}

	if (got_packet) {
		ret = write_frame(outCtxArray[index], &c->time_base, video_st[index]->st, &pkt);
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

void LibavcodecEncoder::close_stream(AVFormatContext *oc, OutputStream *ost)
{
	avcodec_free_context(&ost->enc);
	av_frame_free(&ost->frame);
	av_frame_free(&ost->tmp_frame);
	sws_freeContext(ost->sws_ctx);
	swr_free(&ost->swr_ctx);
}


void LibavcodecEncoder::SetupFFMPEGServer(int playerIndex)
{
	outCtxArray[MAX_PLAYERS] = {};
	optionsOutput[MAX_PLAYERS] = {};
	ret[MAX_PLAYERS] = {};
	opt[MAX_PLAYERS] = {};
	fmt[MAX_PLAYERS] = {};
	
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
		add_stream(video_st[playerIndex], outCtxArray[playerIndex], &video_codec, fmt[playerIndex]->video_codec);
	}

	if ((ret[playerIndex] = av_dict_set(&opt[playerIndex], "re", "", 0)) < 0) {
		LOG_WARN(logger, "Failed to set -re mode.");
		return;
	}

	/* Now that all the parameters are set, we can open the audio and
	* video codecs and allocate the necessary encode buffers. */
	open_video(outCtxArray[playerIndex], video_codec, video_st[playerIndex], opt[playerIndex]);
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

void LibavcodecEncoder::EndAndShutdown(int index)
{
	/* Write the trailer, if any. The trailer must be written before you
	* close the CodecContexts open when you wrote the header; otherwise
	* av_write_trailer() may try to use memory that was freed on
	* av_codec_close(). */
	av_write_trailer(outCtxArray[index]);

	/* Close each codec. */
	close_stream(outCtxArray[index], video_st[index]);

	if (!(fmt[index]->flags & AVFMT_NOFILE))
		/* Close the output file. */
		avio_closep(&outCtxArray[index]->pb);

	/* free the stream */
	avformat_free_context(outCtxArray[index]);
}