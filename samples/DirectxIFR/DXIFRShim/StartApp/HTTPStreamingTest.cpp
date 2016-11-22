
extern "C"
{
    // NOTE: Additional directory ..\zeranoe.com\dev\include gets to the files
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

// NOTE: Additional directory ..\zeranoe.com\dev\lib gets to the files
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avutil.lib")

int main(int argc, char *argv[]) {
    av_log_set_level(AV_LOG_TRACE);

    const char * kInputFileName = "C:/Users/stuadmin/Desktop/Repositories/CloudyNvFBCToSys/samples/DirectxIFR/DXIFRShim/x64/Release/BigBuckBunny.avi";
    //const char * kOutputFileName = "C:/Users/stuadmin/Desktop/Repositories/CloudyNvFBCToSys/samples/DirectxIFR/DXIFRShim/x64/Release/output.avi";
    const char * kOutputFileName = "http://137.132.83.206:30000";
    const char * kOutputFileType = "avi";

    // Global initialization of network components
    avformat_network_init();
    // Register all codecs
    av_register_all();

    AVFormatContext * inCtx = NULL;

    AVDictionary *optionsInput = NULL;
    int retIn;

    // options. equivalent to "-re"
    if ((retIn = av_dict_set(&optionsInput, "re", NULL, 0)) < 0) {
        fprintf(stderr, "Failed to set re mode.\n");
        return retIn;
    }

    // Open file, read header
    int err = avformat_open_input(&inCtx, kInputFileName, NULL, &optionsInput);
    if (err < 0)
        exit(1);


    // Read packets, get stream information
    err = avformat_find_stream_info(inCtx, &optionsInput);
    if (err < 0)
        exit(1);

    int inputVideoStreamIndex = -1;
    // Find the first video stream
    for (unsigned int s = 0; s < inCtx->nb_streams; ++s)
    {
        if (inCtx->streams[s]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            inputVideoStreamIndex = s;
            break;
        }
    }

    if (inputVideoStreamIndex == -1)
    {
        exit(1);
    }

    // Print info about input 
    av_dump_format(inCtx, inputVideoStreamIndex, kInputFileName, 0);

    // Get output format object
    AVOutputFormat * outFmt = av_guess_format(kOutputFileType, NULL, NULL);
    if (!outFmt)
    {
        exit(1);
    }

    AVFormatContext *outCtx = NULL;
    // Make a context for output
    err = avformat_alloc_output_context2(&outCtx, outFmt, NULL, NULL);

    if (err < 0 || !outCtx)
        exit(1);

    // Make output stream
    AVStream * outStrm = avformat_new_stream(outCtx, NULL);
    AVStream const * const inStrm = inCtx->streams[inputVideoStreamIndex];
    AVCodec * codec = NULL;
    avcodec_get_context_defaults3(outStrm->codec, codec);
    outStrm->codec->thread_count = 1;

#if (LIBAVFORMAT_VERSION_MAJOR == 53)
    outStrm->stream_copy = 1;
#endif

    outStrm->codec->coder_type = AVMEDIA_TYPE_VIDEO;
    if (outCtx->oformat->flags & AVFMT_GLOBALHEADER)
    {
        outStrm->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    // out = in (aspect ratio)
    outStrm->codec->sample_aspect_ratio = outStrm->sample_aspect_ratio = inStrm->sample_aspect_ratio;

#if (LIBAVFORMAT_VERSION_MAJOR == 53)
    outCtx->timestamp = 0;
#endif

    AVDictionary *optionsOutput = NULL;
    int ret;

    // options. equivalent to "-listen 1"
    if ((ret = av_dict_set(&optionsOutput, "listen", "1", 0)) < 0) {
        fprintf(stderr, "Failed to set listen mode for server.\n");
        return ret;
    }

    // Open server
    if ((ret = avio_open2(&outCtx->pb, kOutputFileName, AVIO_FLAG_WRITE, NULL, &optionsOutput)) < 0) {
        fprintf(stderr, "Failed to open server.\n");
        return ret;
    }

    // Open file
    //err = avio_open(&outCtx->pb, kOutputFileName, AVIO_FLAG_WRITE); // does not work with network
    //if (err < 0)
    //    exit(1);

#if (LIBAVFORMAT_VERSION_MAJOR == 53)
    AVFormatParameters params = { 0 };
    err = av_set_parameters(outCtx, &params);
    if (err < 0)
        exit(1);
#endif

    // Direct copy of "in"
    outStrm->disposition = inStrm->disposition;
    outStrm->codec->bits_per_raw_sample = inStrm->codec->bits_per_raw_sample;
    outStrm->codec->chroma_sample_location = inStrm->codec->chroma_sample_location;
    outStrm->codec->codec_id = inStrm->codec->codec_id;
    outStrm->codec->codec_type = inStrm->codec->codec_type;
    outStrm->codec->bit_rate = inStrm->codec->bit_rate;
    outStrm->codec->rc_max_rate = inStrm->codec->rc_max_rate;
    outStrm->codec->rc_buffer_size = inStrm->codec->rc_buffer_size;
    outStrm->codec->time_base = inStrm->codec->time_base;
    outStrm->codec->pix_fmt = inStrm->codec->pix_fmt;
    outStrm->codec->width = inStrm->codec->width;
    outStrm->codec->height = inStrm->codec->height;
    outStrm->codec->has_b_frames = inStrm->codec->has_b_frames;

    // Copy codec tag
    if (!outStrm->codec->codec_tag)
    {
        if (!outCtx->oformat->codec_tag
            || av_codec_get_id(outCtx->oformat->codec_tag, inStrm->codec->codec_tag) == outStrm->codec->codec_id
            || av_codec_get_tag(outCtx->oformat->codec_tag, inStrm->codec->codec_id) <= 0)
        {
            outStrm->codec->codec_tag = inStrm->codec->codec_tag;
        }
    }

    // Copy extradata_size
    const size_t extra_size_alloc = (inStrm->codec->extradata_size > 0) ?
        (inStrm->codec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE) : 0;

    if (extra_size_alloc)
    {
        outStrm->codec->extradata = (uint8_t*)av_mallocz(extra_size_alloc);
        memcpy(outStrm->codec->extradata, inStrm->codec->extradata, inStrm->codec->extradata_size);
    }
    outStrm->codec->extradata_size = inStrm->codec->extradata_size;

    // Copy frame rate
    AVRational input_time_base = inStrm->time_base;
    AVRational frameRate = { 25, 1 };
    if (inStrm->r_frame_rate.num && inStrm->r_frame_rate.den
        && (1.0 * inStrm->r_frame_rate.num / inStrm->r_frame_rate.den < 1000.0))
    {
        frameRate.num = inStrm->r_frame_rate.num;
        frameRate.den = inStrm->r_frame_rate.den;
    }
    outStrm->r_frame_rate = frameRate;

    // Copy aspect ratio
    if (!outStrm->codec->sample_aspect_ratio.num) {
        AVRational r0 = { 0, 1 };
        outStrm->codec->sample_aspect_ratio =
            outStrm->sample_aspect_ratio =
            inStrm->sample_aspect_ratio.num ? inStrm->sample_aspect_ratio :
            inStrm->codec->sample_aspect_ratio.num ?
            inStrm->codec->sample_aspect_ratio : r0;
    }
    // done copying

#if LIBAVFORMAT_VERSION_MAJOR == 53
    av_write_header(outFmtCtx);
#else
    avformat_write_header(outCtx, NULL);
#endif
    av_dump_format(outCtx, 0, kOutputFileName, 1);

    // Read packets and frames, write to file
    for (;;)
    {
        AVPacket packet = { 0 };
        av_init_packet(&packet);

        err = AVERROR(EAGAIN);
        while (AVERROR(EAGAIN) == err)
            err = av_read_frame(inCtx, &packet);

        if (err < 0)
        {
            if (AVERROR_EOF != err && AVERROR(EIO) != err)
            {
                // error
                exit(1);
            }
            else
            {
                // end of file
                break;
            }
        }


        if (packet.stream_index == inputVideoStreamIndex)
        {
            err = av_interleaved_write_frame(outCtx, &packet);
            if (err < 0)
                exit(1);
        }

        av_packet_unref(&packet);
    }

    // Write trailer and close and cleanup
    av_write_trailer(outCtx);
    if (!(outCtx->oformat->flags & AVFMT_NOFILE) && outCtx->pb)
    {
        avio_close(outCtx->pb);
    }

    avformat_free_context(outCtx);
    avformat_close_input(&inCtx);
    return 0;
}