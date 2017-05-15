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
#include <atomic>
#include <ctime>

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

// a wrapper around a single output AVStream
typedef struct OutputStream {
    AVStream *st;
    AVCodecContext *enc0;
    AVCodecContext *enc1;

    AVFrame *frame;
    AVFrame *tmp_frame;

} OutputStream;

typedef struct ST {
    AVFormatContext oc;
    int playerIndex;
} setupAVCodecStruct;

EncodeConfig encodeConfig;
int numFramesEncoded;
int numBytesRead;

// Nvidia GRID capture variables
#define NUMFRAMESINFLIGHT 1 // Limit is 3? Putting 4 causes an invalid parameter error to be thrown.
#define MAX_PLAYERS 4
HANDLE gpuEvent[MAX_PLAYERS];
uint8_t *bufferArray[MAX_PLAYERS];

// FFmpeg constants
#define STREAM_FRAME_RATE 30 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */
const char* encoderName = "h264_nvenc";

// Input and Output video size
int bufferWidth;
int bufferHeight;

// Host IP address
const std::string streamingIP = "http://magam001.d1.comp.nus.edu.sg:";
const int firstPort = 30000;

// Arrays used by FFmpeg
AVFormatContext *ocArray[MAX_PLAYERS] = {};
OutputStream ostArray[MAX_PLAYERS];

LONGLONG g_llBegin1 = 0;
LONGLONG g_llPerfFrequency1 = 0;
BOOL g_timeInitialized1 = FALSE;

// Bit rate switching variables
std::atomic_bool isThreadStarted[MAX_PLAYERS] = { false, false, false, false }; // to ensure only 1 thread is working to setup AVCodecContext at any point in time
std::atomic_bool isThreadComplete[MAX_PLAYERS] = { false, false, false, false }; // to ensure that AVCodecContext is fully set up by the thread before it is properly applied
AVCodecContext *codecContextArray[MAX_PLAYERS] = {}; // to store the new AVCodecContext created by the thread
setupAVCodecStruct st[MAX_PLAYERS];
const int bandwidthPerPlayer = 1500000;
int totalBandwidthAvailable = 0;
int sumWeight = 0;
int playerInputArray[MAX_PLAYERS] = { 0 };
int oldSumWeight[MAX_PLAYERS] = { 0 };
int contextToUse[MAX_PLAYERS] = { 0 };
int freeContextComplete[MAX_PLAYERS] = { true, true, true, true };
AVCodec* codecSetup;
int oldInput[MAX_PLAYERS] = { 0 };

#define QPC(Int64) QueryPerformanceCounter((LARGE_INTEGER*)&Int64)
#define QPF(Int64) QueryPerformanceFrequency((LARGE_INTEGER*)&Int64)

double GetFloatingDate1()
{
    LONGLONG llNow;

    if (!g_timeInitialized1)
    {
        QPC(g_llBegin1);
        QPF(g_llPerfFrequency1);
        g_timeInitialized1 = TRUE;
    }
    QPC(llNow);
    return(((double)(llNow - g_llBegin1) / (double)g_llPerfFrequency1));
}

static inline int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    /* Write the compressed frame to the media file. */
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

AVCodecContext *setupAVCodecContext(AVCodec **codec, AVFormatContext *oc, int bitrate, int contextToUse, int index)
{
    AVRational time_base = { 1, STREAM_FRAME_RATE };
    AVRational framerate = { STREAM_FRAME_RATE, 1 };

    if (contextToUse == 0)
    {
        ostArray[index].enc0 = avcodec_alloc_context3(*codec);
        if (!ostArray[index].enc0) {
            LOG_WARN(logger, "Could not alloc an encoding context");
            exit(1);
        }

        // This is in bits
        ostArray[index].enc0->bit_rate = bitrate;
        /* Resolution must be a multiple of two. */
        ostArray[index].enc0->width = bufferWidth;
        ostArray[index].enc0->height = bufferHeight;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
        * of which frame timestamps are represented. For fixed-fps content,
        * timebase should be 1/framerate and timestamp increments should be
        * identical to 1. */
        ostArray[index].st->time_base = time_base;
        ostArray[index].enc0->time_base = ostArray[index].st->time_base;
        ostArray[index].enc0->delay = 0;
        ostArray[index].enc0->framerate = framerate;
        ostArray[index].enc0->has_b_frames = 0;
        ostArray[index].enc0->max_b_frames = 0;
        ostArray[index].enc0->rc_min_vbv_overflow_use = (float)(ostArray[index].enc0->bit_rate / STREAM_FRAME_RATE);
        ostArray[index].enc0->refs = 1; // ref=1

        ostArray[index].enc0->gop_size = 30; // emit one intra frame every 30 frames at most. keyint=30
        ostArray[index].enc0->pix_fmt = STREAM_PIX_FMT;

        // GPU nvenc_h264 parameters
        av_opt_set(ostArray[index].enc0->priv_data, "preset", "llhp", 0);
        av_opt_set(ostArray[index].enc0->priv_data, "delay", "0", 0);

        // CPU encoding paramaters (h264)
        //av_opt_set(ostArray[index].enc0->priv_data, "preset", "ultrafast", 0);
        //av_opt_set(ostArray[index].enc0->priv_data, "tune", "zerolatency", 0);
        //av_opt_set(ostArray[index].enc0->priv_data, "x264opts", "crf=2:vbv-maxrate=4000:vbv-bufsize=160:intra-refresh=1:slice-max-size=2000:keyint=30:ref=1", 0);

        /* Some formats want stream headers to be separate. */
        if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
            ostArray[index].enc0->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        /* open the codec */
        int ret = avcodec_open2(ostArray[index].enc0, *codec, NULL);
        if (ret < 0) {
            LOG_WARN(logger, "Could not open video codec");
            exit(1);
        }

        return ostArray[index].enc0;
    }

    else if (contextToUse == 1)
    {
        ostArray[index].enc1 = avcodec_alloc_context3(*codec); // 0.00011456
        if (!ostArray[index].enc1) {
            LOG_WARN(logger, "Could not alloc an encoding context");
            exit(1);
        }

        // This is in bits
        ostArray[index].enc1->bit_rate = bitrate;
        /* Resolution must be a multiple of two. */
        ostArray[index].enc1->width = bufferWidth;
        ostArray[index].enc1->height = bufferHeight;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
        * of which frame timestamps are represented. For fixed-fps content,
        * timebase should be 1/framerate and timestamp increments should be
        * identical to 1. */
        ostArray[index].st->time_base = time_base;
        ostArray[index].enc1->time_base = ostArray[index].st->time_base;
        ostArray[index].enc1->delay = 0;
        ostArray[index].enc1->framerate = framerate;
        ostArray[index].enc1->has_b_frames = 0;
        ostArray[index].enc1->max_b_frames = 0;
        ostArray[index].enc1->rc_min_vbv_overflow_use = (float)(ostArray[index].enc1->bit_rate / STREAM_FRAME_RATE);
        ostArray[index].enc1->refs = 1; // ref=1

        ostArray[index].enc1->gop_size = 30; // emit one intra frame every 30 frames at most. keyint=30
        ostArray[index].enc1->pix_fmt = STREAM_PIX_FMT;

        // GPU nvenc_h264 parameters
        av_opt_set(ostArray[index].enc1->priv_data, "preset", "llhp", 0);
        av_opt_set(ostArray[index].enc1->priv_data, "delay", "0", 0);

        // CPU encoding paramaters (h264)
        //av_opt_set(ostArray[index].enc0->priv_data, "preset", "ultrafast", 0);
        //av_opt_set(ostArray[index].enc0->priv_data, "tune", "zerolatency", 0);
        //av_opt_set(ostArray[index].enc0->priv_data, "x264opts", "crf=2:vbv-maxrate=4000:vbv-bufsize=160:intra-refresh=1:slice-max-size=2000:keyint=30:ref=1", 0);

        /* Some formats want stream headers to be separate. */
        if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
            ostArray[index].enc1->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        /* open the codec */
        int ret = avcodec_open2(ostArray[index].enc1, *codec, NULL);
        if (ret < 0) {
            LOG_WARN(logger, "Could not open video codec");
            exit(1);
        }

        return ostArray[index].enc1;
    }
}

/* Add an output stream. */
static void add_stream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id, int index)
{
    AVCodecContext *c;

    /* find the encoder */
    //*codec = avcodec_find_encoder(codec_id);
    *codec = avcodec_find_encoder_by_name(encoderName);
    if (!(*codec)) {
        LOG_WARN(logger, "Could not find encoder for" << avcodec_get_name(codec_id));
        exit(1);
    }

    ostArray[index].st = avformat_new_stream(oc, NULL);
    if (!ostArray[index].st) {
        LOG_WARN(logger, "Could not allocate stream");
        exit(1);
    }
    ostArray[index].st->id = oc->nb_streams - 1;

    c = setupAVCodecContext(codec, oc, 1250000, contextToUse[index], index);
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

static void open_video(AVFormatContext *oc, AVCodec *codec, int index)
{
    int ret;
    AVCodecContext *c;
    if (contextToUse[index] == 0)
    {
        c = ostArray[index].enc0;
    }
    else
    {
        c = ostArray[index].enc1;
    }

    /* allocate and init a re-usable frame */
    ostArray[index].frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!ostArray[index].frame) {
        LOG_WARN(logger, "Could not allocate video frame");
        exit(1);
    }

    /* If the output format is not YUV420P, then a temporary YUV420P
    * picture is needed too. It is then converted to the required
    * output format. */
    ostArray[index].tmp_frame = NULL;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ostArray[index].tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!ostArray[index].tmp_frame) {
            LOG_WARN(logger, "Could not allocate temporary picture");
            exit(1);
        }
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ostArray[index].st->codecpar, c);
    if (ret < 0) {
        LOG_WARN(logger, "Could not copy the stream parameters");
        exit(1);
    }
}

void avcodec_free_context_proc(void* args)
{
    int index = (int)args;

    if (contextToUse[index] == 1)
    {
        avcodec_free_context(&ostArray[index].enc0);
    }
    else
    {
        avcodec_free_context(&ostArray[index].enc1);
    }
    freeContextComplete[index] = true;
    _endthread();
}

void setupAVCodecContextProc(void* args)
{
    int index = (int)args;
    //float weight = (float)playerInputArray[index] / (float)sumWeight;
    int bitrate;// = (int)(weight * totalBandwidthAvailable);

    if (playerInputArray[index] == 3) { // shooting
        bitrate = 1500000;
    }
    else if (playerInputArray[index] == 2) { // mouse movement or any other keyboard key
        bitrate = 1200000;
    }
    else if (playerInputArray[index] == 1) { // no input
        bitrate = 700000;
    }

    while (freeContextComplete[index] == false)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // This will set up context for the other ost->enc (the one currently not in use)
    codecContextArray[st[index].playerIndex] = setupAVCodecContext(&codecSetup, &st[index].oc, bitrate, 1 - contextToUse[index], index);

    isThreadComplete[st[index].playerIndex] = true;
    _endthread();
}

/*
* encode one video frame and send it to the muxer
* return 1 when encoding is finished, 0 otherwise
*/
static inline int write_video_frame(AVFormatContext *oc, uint8_t *buffer, int index)
{
    int ret;
    int got_packet = 0;
    AVPacket pkt = { 0 };

    if (playerInputArray[index] != oldInput[index] && isThreadStarted[index] == false) {
        st[index].oc = *oc;
        st[index].playerIndex = index;

        HANDLE setupThread = (HANDLE)_beginthread(&setupAVCodecContextProc, 0, (void*)index);
        BOOL success = SetThreadPriority(setupThread, THREAD_PRIORITY_HIGHEST);

        if (success == FALSE)
        {
            LOG_WARN(logger, "Set priority of setupThread failed!");
        }

        oldInput[index] = playerInputArray[index];
        isThreadStarted[index] = true;
    }

    // Only swap in the new AVCodecContext when the thread is done
    if (isThreadComplete[index] == true)
    {
        // Context 0 is currently in use. Context 1 has been set up by the thread and is ready
        if (contextToUse[index] == 0)
        {
            ostArray[index].enc1 = codecContextArray[index];

        }
        // Context 1 is currently in use. Context 0 has been set up by the thread and is ready
        else
        {
            ostArray[index].enc0 = codecContextArray[index];
        }

        contextToUse[index] = 1 - contextToUse[index]; // other context is ready to be used

        freeContextComplete[index] = false;
        HANDLE closingThread = (HANDLE)_beginthread(&avcodec_free_context_proc, 0, (void*)index);
        BOOL success = SetThreadPriority(closingThread, THREAD_PRIORITY_HIGHEST);
        if (success == FALSE)
        {
            LOG_WARN(logger, "Set priority of closingThread failed!");
        }

        isThreadStarted[index] = false; // Ready to let thread start again if necessary
        isThreadComplete[index] = false; // Ready to let thread start again if necessary
    }

    ostArray[index].frame->width = bufferWidth;
    ostArray[index].frame->height = bufferHeight;
    ostArray[index].frame->format = STREAM_PIX_FMT;

    avpicture_fill((AVPicture*)ostArray[index].frame, buffer, AV_PIX_FMT_YUV420P, ostArray[index].frame->width, ostArray[index].frame->height);

    av_init_packet(&pkt);

    /* encode the image */
    if (contextToUse[index] == 0)
    {
        ret = avcodec_encode_video2(ostArray[index].enc0, &pkt, ostArray[index].frame, &got_packet);
    }
    else
    {
        ret = avcodec_encode_video2(ostArray[index].enc1, &pkt, ostArray[index].frame, &got_packet);
    }
    if (ret < 0) {
        LOG_WARN(logger, "Error encoding video frame");
        exit(1);
    }

    if (got_packet) {
        if (contextToUse[index] == 0)
        {
            ret = write_frame(oc, &ostArray[index].enc0->time_base, ostArray[index].st, &pkt);
        }
        else
        {
            ret = write_frame(oc, &ostArray[index].enc1->time_base, ostArray[index].st, &pkt);
        }
    }
    else {
        ret = 0;
        LOG_WARN(logger, "No packet");
    }

    if (ret < 0) {
        // This happens when the thin client is closed. This will close the game.
        LOG_WARN(logger, "Error while writing video frame. Thin client was probably closed.");
        //exit(1);
        totalBandwidthAvailable -= bandwidthPerPlayer;
        _endthread();
    }

    return (ostArray[index].frame || got_packet) ? 0 : 1;
}

static void close_stream(int index)
{
    avcodec_free_context(&ostArray[index].enc0);
    avcodec_free_context(&ostArray[index].enc1);
    av_frame_free(&ostArray[index].frame);
    av_frame_free(&ostArray[index].tmp_frame);
}

void SetupFFMPEGServer(int playerIndex)
{
    AVCodec *video_codec;
    int ret;

    /* Initialize libavcodec, and register all codecs and formats. */
    av_register_all();
    // Global initialization of network components
    avformat_network_init();

    /* allocate the output media context */
    avformat_alloc_output_context2(&ocArray[playerIndex], NULL, "h264", "output.h264");
    if (!ocArray[playerIndex]) {
        LOG_WARN(logger, "No output context.");
        return;
    }

    /* Add the video streams using the default format codecs
    * and initialize the codecs. */
    if (ocArray[playerIndex]->oformat->video_codec != AV_CODEC_ID_NONE) {
        add_stream(ocArray[playerIndex], &video_codec, ocArray[playerIndex]->oformat->video_codec, playerIndex);
    }

    /* Now that all the parameters are set, we can open the audio and
    * video codecs and allocate the necessary encode buffers. */
    open_video(ocArray[playerIndex], video_codec, playerIndex);
    av_dump_format(ocArray[playerIndex], 0, NULL, 1);

    AVDictionary *optionsOutput[MAX_PLAYERS] = {};

    if ((ret = av_dict_set(&optionsOutput[playerIndex], "re", "", 0)) < 0) {
        LOG_WARN(logger, "Failed to set -re mode.");
        return;
    }

    if ((ret = av_dict_set(&optionsOutput[playerIndex], "listen", "1", 0)) < 0) {
        LOG_WARN(logger, "Failed to set listen mode for server.");
        return;
    }

    if ((ret = av_dict_set(&optionsOutput[playerIndex], "an", "", 0)) < 0) {
        LOG_WARN(logger, "Failed to set -an mode.");
        return;
    }

    std::stringstream *HTTPUrl = new std::stringstream();
    *HTTPUrl << streamingIP << firstPort + playerIndex;

    // Open server
    if ((avio_open2(&ocArray[playerIndex]->pb, HTTPUrl->str().c_str(), AVIO_FLAG_WRITE, NULL, &optionsOutput[playerIndex])) < 0) {
        LOG_ERROR(logger, "Failed to open server " << playerIndex << ".");
        return;
    }
    LOG_DEBUG(logger, "Server " << playerIndex << " opened at " << HTTPUrl->str());

    /* Write the stream header, if any. */
    ret = avformat_write_header(ocArray[playerIndex], &optionsOutput[playerIndex]);
    if (ret < 0) {
        LOG_ERROR(logger, "Error occurred when opening output file.\n");
        return;
    }
}

void CleanupLibavCodec(int index)
{
    /* Write the trailer, if any. The trailer must be written before you
    * close the CodecContexts open when you wrote the header; otherwise
    * av_write_trailer() may try to use memory that was freed on
    * av_codec_close(). */
    av_write_trailer(ocArray[index]);

    /* Close each codec. */
    close_stream(index);

    if (!(ocArray[index]->oformat->flags & AVFMT_NOFILE)) {
        /* Close the output file. */
        avio_closep(&ocArray[index]->pb);
    }

    /* free the stream */
    avformat_free_context(ocArray[index]);
}

BOOL NvIFREncoder::StartEncoder(int index, int windowWidth, int windowHeight)
{
    bufferWidth = windowWidth;
    bufferHeight = windowHeight;

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
    totalBandwidthAvailable += bandwidthPerPlayer;
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

    //SetupFFMPEGServer(index);

    UINT uFrameCount = 0;
    DWORD dwTimeZero = timeGetTime();

    char c = '0';
    std::streampos fileSize = 0;
    std::streampos prevFileSize = 0;
    int timeBeforeIdle = 3;
    bool isIdling = false;
    time_t shootingStartTime = std::time(0);
    time_t idleStartTime = 0;
    codecSetup = avcodec_find_encoder_by_name(encoderName);

    ostringstream oss;
    oss << "G:\\Packaged Games\\414 Shipping 1-2-3 Limited 2\\WindowsNoEditor\\MyProject414\\Binaries\\Win64\\test" << index << ".txt";
    ifstream fin;

    // Setup Nvidia Video Codec SDK
    char *argv[] = { "-i", "inputFile.yuv", "-o", "output.h264", "-size", "1280", "720", NULL };
    int argc = sizeof(argv) / sizeof(char*) - 1;

    CNvEncoder nvEncoder;
    nvEncoder.EncodeMain(argc, argv);

    while (!bStopEncoder)
    {
        //fin.open(oss.str());
        //if (fin.is_open())
        //{
        //    fin.seekg(-3, ios::end); // -1 and -2 gets \n on the last line
        //    fileSize = fin.tellg();
        //    fin.get(c);
        //    fin.close();

        //    // If we shoot, we should refresh the shooting start time
        //    if (c == '3' && fileSize != prevFileSize)
        //    {
        //        shootingStartTime = std::time(0);
        //    }

        //    // We cannot let any other movement e.g. "c = 2" through if we have recently shot
        //    if ((std::time(0) - shootingStartTime) < timeBeforeIdle)
        //    {
        //        c = '3';
        //    }
        //    // No input from player - is idling
        //    else if (fileSize == prevFileSize)
        //    {
        //        if (isIdling == false)
        //        {
        //            // Start the countdown
        //            idleStartTime = std::time(0);
        //            isIdling = true;
        //        }
        //        // We have idled for more than 3 seconds
        //        else if ((std::time(0) - idleStartTime) >= timeBeforeIdle)
        //        {
        //            c = '1';
        //        }
        //    }
        //    // If there is input, allow countdown to restart
        //    else if (c == '3' || c == '2')
        //    {
        //        isIdling = false; // There is input. 
        //    }

        //    prevFileSize = fileSize;

        //    playerInputArray[index] = (c - '0');
        //}
        //else
        //{
        //    LOG_ERROR(logger, "Failed to open file " << index);
        //}

        //// Index 0 will do the summing of the array.
        //// This will pose problems in the future if player 0 can 
        //// just leave while the other players are playing.
        //if (index == 0)
        //{
        //    sumWeight = 0;
        //    for (int i = 0; i < MAX_PLAYERS; i++)
        //    {
        //        sumWeight += playerInputArray[i];
        //    }
        //}

        if (!UpdateBackBuffer())
        {
            LOG_DEBUG(logger, "UpdateBackBuffer() failed");
        }

        NVIFRRESULT res = pIFR->NvIFRTransferRenderTargetToSys(0);

        if (res == NVIFR_SUCCESS)
        {
            HANDLE ahevt[] = { gpuEvent[index], hevtStopEncoder };
            //DWORD dwRet = WaitForSingleObject(gpuEvent[index], INFINITE);
            DWORD dwRet = WaitForMultipleObjects(sizeof(ahevt) / sizeof(ahevt[0]), ahevt, FALSE, INFINITE);
            if (dwRet != WAIT_OBJECT_0)// If not signalled
            {
                if (dwRet != WAIT_OBJECT_0 + 1)
                {
                    LOG_WARN(logger, "Abnormally break from encoding loop, dwRet=" << dwRet);
                }
                return;
            }
            ResetEvent(gpuEvent[index]);

            nvEncoder.EncodeFrameLoop(bufferArray[index], encodeConfig);
            //write_video_frame(ocArray[index], /*&ostArray[index], */bufferArray[index], index);
        }
        else
        {
            LOG_ERROR(logger, "NvIFRTransferRenderTargetToSys failed, res=" << res);
        }

        // This improves FPS but thin client suffers from slightly jittery video
        //std::this_thread::sleep_for(std::chrono::milliseconds(33));

        // This sleeps the thread if we are producing frames faster than the desired framerate
        int delta = (int)((dwTimeZero + ++uFrameCount * 1000 / STREAM_FRAME_RATE) - timeGetTime());
        if (delta > 0) {
            WaitForSingleObject(hevtStopEncoder, delta);
        }
    }
    LOG_DEBUG(logger, "Quit encoding loop");

    CleanupLibavCodec(index);
    CleanupNvIFR();
}

Streamer * NvIFREncoder::pSharedStreamer = NULL;


////////////////////////////////////////////////////////////////////////////
//
// Copyright 1993-2014 NVIDIA Corporation.  All rights reserved.
//
// Please refer to the NVIDIA end user license agreement (EULA) associated
// with this source code for terms and conditions that govern your use of
// this software. Any use, reproduction, disclosure, or distribution of
// this software and related documentation outside the terms of the EULA
// is strictly prohibited.
//
////////////////////////////////////////////////////////////////////////////

#include "../common/inc/nvCPUOPSys.h"
#include "../common/inc/nvEncodeAPI.h"
#include "../common/inc/nvUtils.h"
#include "../common/inc/nvFileIO.h"
#include <new>

#define BITSTREAM_BUFFER_SIZE 2 * 1024 * 1024

void convertYUVpitchtoNV12(unsigned char *yuv_luma, unsigned char *yuv_cb, unsigned char *yuv_cr,
    unsigned char *nv12_luma, unsigned char *nv12_chroma,
    int width, int height, int srcStride, int dstStride)
{
    int y;
    int x;
    if (srcStride == 0)
        srcStride = width;
    if (dstStride == 0)
        dstStride = width;

    for (y = 0; y < height; y++)
    {
        memcpy(nv12_luma + (dstStride*y), yuv_luma + (srcStride*y), width);
    }

    for (y = 0; y < height / 2; y++)
    {
        for (x = 0; x < width; x = x + 2)
        {
            nv12_chroma[(y*dstStride) + x] = yuv_cb[((srcStride / 2)*y) + (x >> 1)];
            nv12_chroma[(y*dstStride) + (x + 1)] = yuv_cr[((srcStride / 2)*y) + (x >> 1)];
        }
    }
}

void convertYUVpitchtoYUV444(unsigned char *yuv_luma, unsigned char *yuv_cb, unsigned char *yuv_cr,
    unsigned char *surf_luma, unsigned char *surf_cb, unsigned char *surf_cr, int width, int height, int srcStride, int dstStride)
{
    int h;

    for (h = 0; h < height; h++)
    {
        memcpy(surf_luma + dstStride * h, yuv_luma + srcStride * h, width);
        memcpy(surf_cb + dstStride * h, yuv_cb + srcStride * h, width);
        memcpy(surf_cr + dstStride * h, yuv_cr + srcStride * h, width);
    }
}

CNvEncoder::CNvEncoder()
{
    m_pNvHWEncoder = new CNvHWEncoder;
    m_pDevice = NULL;
#if defined (NV_WINDOWS)
    m_pD3D = NULL;
#endif
    m_cuContext = NULL;

    m_uEncodeBufferCount = 0;
    memset(&m_stEncoderInput, 0, sizeof(m_stEncoderInput));
    memset(&m_stEOSOutputBfr, 0, sizeof(m_stEOSOutputBfr));

    memset(&m_stEncodeBuffer, 0, sizeof(m_stEncodeBuffer));
}

CNvEncoder::~CNvEncoder()
{
    if (m_pNvHWEncoder)
    {
        delete m_pNvHWEncoder;
        m_pNvHWEncoder = NULL;
    }
}

NVENCSTATUS CNvEncoder::InitCuda(uint32_t deviceID)
{
    CUresult cuResult;
    CUdevice device;
    CUcontext cuContextCurr;
    int  deviceCount = 0;
    int  SMminor = 0, SMmajor = 0;

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    typedef HMODULE CUDADRIVER;
#else
    typedef void *CUDADRIVER;
#endif
    CUDADRIVER hHandleDriver = 0;
    cuResult = cuInit(0, __CUDA_API_VERSION, hHandleDriver);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuInit error:0x%x\n", cuResult);
        assert(0);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuDeviceGetCount(&deviceCount);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuDeviceGetCount error:0x%x\n", cuResult);
        assert(0);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    // If dev is negative value, we clamp to 0
    if ((int)deviceID < 0)
        deviceID = 0;

    if (deviceID >(unsigned int)deviceCount - 1)
    {
        PRINTERR("Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    cuResult = cuDeviceGet(&device, deviceID);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuDeviceGet error:0x%x\n", cuResult);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuDeviceComputeCapability(&SMmajor, &SMminor, deviceID);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuDeviceComputeCapability error:0x%x\n", cuResult);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    if (((SMmajor << 4) + SMminor) < 0x30)
    {
        PRINTERR("GPU %d does not have NVENC capabilities exiting\n", deviceID);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuCtxCreate((CUcontext*)(&m_pDevice), 0, device);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuCtxCreate error:0x%x\n", cuResult);
        assert(0);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuCtxPopCurrent(&cuContextCurr);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuCtxPopCurrent error:0x%x\n", cuResult);
        assert(0);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }
    return NV_ENC_SUCCESS;
}

#if defined(NV_WINDOWS)
NVENCSTATUS CNvEncoder::InitD3D10(uint32_t deviceID)
{
    HRESULT hr;
    IDXGIFactory * pFactory = NULL;
    IDXGIAdapter * pAdapter;

    if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory) != S_OK)
    {
        return NV_ENC_ERR_GENERIC;
    }

    if (pFactory->EnumAdapters(deviceID, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        hr = D3D10CreateDevice(pAdapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
            D3D10_SDK_VERSION, (ID3D10Device**)(&m_pDevice));
        if (FAILED(hr))
        {
            PRINTERR("Problem while creating %d D3d10 device \n", deviceID);
            return NV_ENC_ERR_OUT_OF_MEMORY;
        }
    }
    else
    {
        PRINTERR("Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    return  NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::InitD3D11(uint32_t deviceID)
{
    HRESULT hr;
    IDXGIFactory * pFactory = NULL;
    IDXGIAdapter * pAdapter;

    if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory) != S_OK)
    {
        return NV_ENC_ERR_GENERIC;
    }

    if (pFactory->EnumAdapters(deviceID, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        hr = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0,
            NULL, 0, D3D11_SDK_VERSION, (ID3D11Device**)(&m_pDevice), NULL, NULL);
        if (FAILED(hr))
        {
            PRINTERR("Problem while creating %d D3d11 device \n", deviceID);
            return NV_ENC_ERR_OUT_OF_MEMORY;
        }
    }
    else
    {
        PRINTERR("Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    return  NV_ENC_SUCCESS;
}
#endif

NVENCSTATUS CNvEncoder::AllocateIOBuffers(uint32_t uInputWidth, uint32_t uInputHeight, uint32_t isYuv444)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    m_EncodeBufferQueue.Initialize(m_stEncodeBuffer, m_uEncodeBufferCount);
    for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
    {
        nvStatus = m_pNvHWEncoder->NvEncCreateInputBuffer(uInputWidth, uInputHeight, &m_stEncodeBuffer[i].stInputBfr.hInputSurface, isYuv444);
        if (nvStatus != NV_ENC_SUCCESS)
            return nvStatus;

        m_stEncodeBuffer[i].stInputBfr.bufferFmt = isYuv444 ? NV_ENC_BUFFER_FORMAT_YUV444_PL : NV_ENC_BUFFER_FORMAT_NV12_PL;
        m_stEncodeBuffer[i].stInputBfr.dwWidth = uInputWidth;
        m_stEncodeBuffer[i].stInputBfr.dwHeight = uInputHeight;

        //Allocate output surface
        nvStatus = m_pNvHWEncoder->NvEncCreateBitstreamBuffer(BITSTREAM_BUFFER_SIZE, &m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
        if (nvStatus != NV_ENC_SUCCESS)
            return nvStatus;
        m_stEncodeBuffer[i].stOutputBfr.dwBitstreamBufferSize = BITSTREAM_BUFFER_SIZE;

#if defined (NV_WINDOWS)
        nvStatus = m_pNvHWEncoder->NvEncRegisterAsyncEvent(&m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
        if (nvStatus != NV_ENC_SUCCESS)
            return nvStatus;
        if (m_stEncoderInput.enableMEOnly)
        {
            m_stEncodeBuffer[i].stOutputBfr.bWaitOnEvent = false;
        }
        else
            m_stEncodeBuffer[i].stOutputBfr.bWaitOnEvent = true;
#else
        m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = NULL;
#endif
    }

    m_stEOSOutputBfr.bEOSFlag = TRUE;

#if defined (NV_WINDOWS)
    nvStatus = m_pNvHWEncoder->NvEncRegisterAsyncEvent(&m_stEOSOutputBfr.hOutputEvent);
    if (nvStatus != NV_ENC_SUCCESS)
        return nvStatus;
#else
    m_stEOSOutputBfr.hOutputEvent = NULL;
#endif

    return NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::ReleaseIOBuffers()
{
    for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
    {
        m_pNvHWEncoder->NvEncDestroyInputBuffer(m_stEncodeBuffer[i].stInputBfr.hInputSurface);
        m_stEncodeBuffer[i].stInputBfr.hInputSurface = NULL;

        if (m_stEncoderInput.enableMEOnly)
        {
            m_pNvHWEncoder->NvEncDestroyMVBuffer(m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
            m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer = NULL;
        }
        else
        {
            m_pNvHWEncoder->NvEncDestroyBitstreamBuffer(m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
            m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer = NULL;
        }

#if defined(NV_WINDOWS)
        m_pNvHWEncoder->NvEncUnregisterAsyncEvent(m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
        nvCloseFile(m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
        m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = NULL;
#endif
    }

    if (m_stEOSOutputBfr.hOutputEvent)
    {
#if defined(NV_WINDOWS)
        m_pNvHWEncoder->NvEncUnregisterAsyncEvent(m_stEOSOutputBfr.hOutputEvent);
        nvCloseFile(m_stEOSOutputBfr.hOutputEvent);
        m_stEOSOutputBfr.hOutputEvent = NULL;
#endif
    }

    return NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::FlushEncoder()
{
    NVENCSTATUS nvStatus = m_pNvHWEncoder->NvEncFlushEncoderQueue(m_stEOSOutputBfr.hOutputEvent);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
        return nvStatus;
    }

    EncodeBuffer *pEncodeBufer = m_EncodeBufferQueue.GetPending();
    while (pEncodeBufer)
    {
        m_pNvHWEncoder->ProcessOutput(pEncodeBufer);
        pEncodeBufer = m_EncodeBufferQueue.GetPending();
    }

#if defined(NV_WINDOWS)
    if (WaitForSingleObject(m_stEOSOutputBfr.hOutputEvent, 500) != WAIT_OBJECT_0)
    {
        assert(0);
        nvStatus = NV_ENC_ERR_GENERIC;
    }
#endif

    return nvStatus;
}

NVENCSTATUS CNvEncoder::Deinitialize(uint32_t devicetype)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    ReleaseIOBuffers();

    nvStatus = m_pNvHWEncoder->NvEncDestroyEncoder();

    if (m_pDevice)
    {
        switch (devicetype)
        {
#if defined(NV_WINDOWS)
        case NV_ENC_DX9:
            ((IDirect3DDevice9*)(m_pDevice))->Release();
            break;

        case NV_ENC_DX10:
            ((ID3D10Device*)(m_pDevice))->Release();
            break;

        case NV_ENC_DX11:
            ((ID3D11Device*)(m_pDevice))->Release();
            break;
#endif

        case NV_ENC_CUDA:
            CUresult cuResult = CUDA_SUCCESS;
            cuResult = cuCtxDestroy((CUcontext)m_pDevice);
            if (cuResult != CUDA_SUCCESS)
                PRINTERR("cuCtxDestroy error:0x%x\n", cuResult);
        }

        m_pDevice = NULL;
    }

#if defined (NV_WINDOWS)
    if (m_pD3D)
    {
        m_pD3D->Release();
        m_pD3D = NULL;
    }
#endif

    return nvStatus;
}

NVENCSTATUS loadframe(uint8_t *yuvInput[3], HANDLE hInputYUVFile, uint32_t frmIdx, uint32_t width, uint32_t height, uint32_t &numBytesRead, uint32_t isYuv444)
{
    uint64_t fileOffset;
    uint32_t result;
    //Set size depending on whether it is YUV 444 or YUV 420
    uint32_t dwInFrameSize = isYuv444 ? width * height * 3 : width*height + (width*height) / 2;
    fileOffset = (uint64_t)dwInFrameSize * frmIdx;
    result = nvSetFilePointer64(hInputYUVFile, fileOffset, NULL, FILE_BEGIN);
    if (result == INVALID_SET_FILE_POINTER)
    {
        return NV_ENC_ERR_INVALID_PARAM;
    }
    if (isYuv444)
    {
        nvReadFile(hInputYUVFile, yuvInput[0], width * height, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[1], width * height, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[2], width * height, &numBytesRead, NULL);
    }
    else
    {
        nvReadFile(hInputYUVFile, yuvInput[0], width * height, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[1], width * height / 4, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[2], width * height / 4, &numBytesRead, NULL);
    }
    return NV_ENC_SUCCESS;
}


int CNvEncoder::EncodeMain(int argc, char *argv[])
{
    HANDLE hInput;
    DWORD fileSize;
    uint32_t numBytesRead = 0;
    uint8_t *yuv[3];
    int lumaPlaneSize, chromaPlaneSize;
    unsigned long long lStart, lEnd, lFreq;
    int numFramesEncoded = 0;
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    bool bError = false;
    
    unsigned int preloadedFrameCount = FRAME_QUEUE;

    memset(&encodeConfig, 0, sizeof(EncodeConfig));

    encodeConfig.endFrameIdx = INT_MAX;
    encodeConfig.bitrate = 5000000;
    encodeConfig.rcMode = NV_ENC_PARAMS_RC_CONSTQP;
    encodeConfig.gopLength = NVENC_INFINITE_GOPLENGTH;
    encodeConfig.deviceType = NV_ENC_CUDA;
    encodeConfig.codec = NV_ENC_H264;
    encodeConfig.fps = 30;
    encodeConfig.qp = 28;
    encodeConfig.i_quant_factor = DEFAULT_I_QFACTOR;
    encodeConfig.b_quant_factor = DEFAULT_B_QFACTOR;
    encodeConfig.i_quant_offset = DEFAULT_I_QOFFSET;
    encodeConfig.b_quant_offset = DEFAULT_B_QOFFSET;
    encodeConfig.presetGUID = NV_ENC_PRESET_DEFAULT_GUID;
    encodeConfig.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    encodeConfig.isYuv444 = 0;

    nvStatus = m_pNvHWEncoder->ParseArguments(&encodeConfig, argc, argv);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        return 1;
    }

    if (!encodeConfig.inputFileName || !encodeConfig.outputFileName || encodeConfig.width == 0 || encodeConfig.height == 0)
    {
        return 1;
    }

    encodeConfig.fOutput = fopen(encodeConfig.outputFileName, "wb");
    if (encodeConfig.fOutput == NULL)
    {
        PRINTERR("Failed to create \"%s\"\n", encodeConfig.outputFileName);
        return 1;
    }
    
    hInput = nvOpenFile(encodeConfig.inputFileName);
    if (hInput == INVALID_HANDLE_VALUE)
    {
        PRINTERR("Failed to open \"%s\"\n", encodeConfig.inputFileName);
        return 1;
    }

    switch (encodeConfig.deviceType)
    {
#if defined(NV_WINDOWS)
    case NV_ENC_DX10:
        InitD3D10(encodeConfig.deviceID);
        break;

    case NV_ENC_DX11:
        InitD3D11(encodeConfig.deviceID);
        break;
#endif
    case NV_ENC_CUDA:
        InitCuda(encodeConfig.deviceID);
        break;
    }

    if (encodeConfig.deviceType != NV_ENC_CUDA)
        nvStatus = m_pNvHWEncoder->Initialize(m_pDevice, NV_ENC_DEVICE_TYPE_DIRECTX);
    else
        nvStatus = m_pNvHWEncoder->Initialize(m_pDevice, NV_ENC_DEVICE_TYPE_CUDA);

    if (nvStatus != NV_ENC_SUCCESS)
        return 1;

    encodeConfig.presetGUID = m_pNvHWEncoder->GetPresetGUID(encodeConfig.encoderPreset, encodeConfig.codec);

    nvStatus = m_pNvHWEncoder->CreateEncoder(&encodeConfig);
    if (nvStatus != NV_ENC_SUCCESS)
        return 1;
    encodeConfig.maxWidth = encodeConfig.maxWidth ? encodeConfig.maxWidth : encodeConfig.width;
    encodeConfig.maxHeight = encodeConfig.maxHeight ? encodeConfig.maxHeight : encodeConfig.height;

    if (encodeConfig.numB > 0)
    {
        m_uEncodeBufferCount = encodeConfig.numB + 4; // min buffers is numb + 1 + 3 pipelining
    }
    else
    {
        int numMBs = ((encodeConfig.maxHeight + 15) >> 4) * ((encodeConfig.maxWidth + 15) >> 4);
        int NumIOBuffers;
        if (numMBs >= 32768) //4kx2k
            NumIOBuffers = MAX_ENCODE_QUEUE / 8;
        else if (numMBs >= 16384) // 2kx2k
            NumIOBuffers = MAX_ENCODE_QUEUE / 4;
        else if (numMBs >= 8160) // 1920x1080
            NumIOBuffers = MAX_ENCODE_QUEUE / 2;
        else
            NumIOBuffers = MAX_ENCODE_QUEUE;
        m_uEncodeBufferCount = NumIOBuffers;
    }
    m_uPicStruct = encodeConfig.pictureStruct;
    nvStatus = AllocateIOBuffers(encodeConfig.width, encodeConfig.height, encodeConfig.isYuv444);
    if (nvStatus != NV_ENC_SUCCESS)
        return 1;

    if (encodeConfig.preloadedFrameCount >= 2)
    {
        preloadedFrameCount = encodeConfig.preloadedFrameCount;
    }

    uint32_t  chromaFormatIDC = (encodeConfig.isYuv444 ? 3 : 1);
    lumaPlaneSize = encodeConfig.maxWidth * encodeConfig.maxHeight;
    chromaPlaneSize = (chromaFormatIDC == 3) ? lumaPlaneSize : (lumaPlaneSize >> 2);
    nvGetFileSize(hInput, &fileSize);
    int totalFrames = fileSize / (lumaPlaneSize + chromaPlaneSize + chromaPlaneSize);
    if (encodeConfig.endFrameIdx < 0) {
        encodeConfig.endFrameIdx = totalFrames - 1;
    }
    else if (encodeConfig.endFrameIdx > totalFrames) {
        PRINTERR("nvEncoder.exe Warning: -endf %d exceeds total video frame %d, using %d instead\n", encodeConfig.endFrameIdx, totalFrames, totalFrames);
        encodeConfig.endFrameIdx = totalFrames - 1;
    }

    yuv[0] = new(std::nothrow) uint8_t[lumaPlaneSize];
    yuv[1] = new(std::nothrow) uint8_t[chromaPlaneSize];
    yuv[2] = new(std::nothrow) uint8_t[chromaPlaneSize];
    NvQueryPerformanceCounter(&lStart);

    if (yuv[0] == NULL || yuv[1] == NULL || yuv[2] == NULL)
    {
        PRINTERR("\nvEncoder.exe Error: Failed to allocate memory for yuv array!\n");
        return 1;
    }

//
//    nvStatus = EncodeFrame(NULL, true, encodeConfig.width, encodeConfig.height);
//    if (nvStatus != NV_ENC_SUCCESS)
//    {
//        bError = true;
//        goto exit;
//    }
//
//    if (numFramesEncoded > 0)
//    {
//        NvQueryPerformanceCounter(&lEnd);
//        NvQueryPerformanceFrequency(&lFreq);
//        double elapsedTime = (double)(lEnd - lStart);
//        printf("Encoded %d frames in %6.2fms\n", numFramesEncoded, (elapsedTime*1000.0) / lFreq);
//        printf("Avergage Encode Time : %6.2fms\n", ((elapsedTime*1000.0) / numFramesEncoded) / lFreq);
//    }
//
//exit:
//    if (encodeConfig.fOutput)
//    {
//        fclose(encodeConfig.fOutput);
//    }
//
//    if (hInput)
//    {
//        nvCloseFile(hInput);
//    }
//
//    Deinitialize(encodeConfig.deviceType);
//
//    for (int i = 0; i < 3; i++)
//    {
//        if (yuv[i])
//        {
//            delete[] yuv[i];
//        }
//    }
//
//    return bError ? 1 : 0;
}

void CNvEncoder::EncodeFrameLoop(uint8_t *buffer, EncodeConfig encodeConfig)
{
    //numBytesRead = 0;
    //loadframe(yuv, hInput, frm, encodeConfig.width, encodeConfig.height, numBytesRead, encodeConfig.isYuv444);
    //if (numBytesRead == 0)
    //    break;

    EncodeFrameConfig stEncodeFrame;
    memset(&stEncodeFrame, 0, sizeof(stEncodeFrame));
    stEncodeFrame.yuv[0] = buffer;//yuv[0];
    stEncodeFrame.yuv[1] = buffer;//yuv[1];
    stEncodeFrame.yuv[2] = buffer;//yuv[2];

    stEncodeFrame.stride[0] = encodeConfig.width;
    stEncodeFrame.stride[1] = (encodeConfig.isYuv444) ? encodeConfig.width : encodeConfig.width / 2;
    stEncodeFrame.stride[2] = (encodeConfig.isYuv444) ? encodeConfig.width : encodeConfig.width / 2;
    stEncodeFrame.width = encodeConfig.width;
    stEncodeFrame.height = encodeConfig.height;

    EncodeFrame(&stEncodeFrame, false, encodeConfig.width, encodeConfig.height);
    numFramesEncoded++;

    //if (frm == 500)//encodeConfig.endFrameIdx / 2)
    //{
    //    NvEncPictureCommand encPicCommand;
    //
    //    encPicCommand.bBitrateChangePending = true;
    //    encPicCommand.newVBVSize = 0;
    //    encPicCommand.newBitrate = 250000;
    //
    //    //encPicCommand.bForceIDR = false;
    //
    //    //encPicCommand.bForceIntraRefresh = false;
    //    //encPicCommand.intraRefreshDuration = 0;
    //
    //    //encPicCommand.bInvalidateRefFrames = false;
    //    //encPicCommand.numRefFramesToInvalidate = 0;
    //    //encPicCommand.refFrameNumbers[0] = 0;    
    //
    //    encPicCommand.bResolutionChangePending = false;
    //    //encPicCommand.newHeight = 640;
    //    //encPicCommand.newWidth = 480;
    //
    //    NVENCSTATUS status = m_pNvHWEncoder->NvEncReconfigureEncoder(&encPicCommand);
    //    if (status == NV_ENC_SUCCESS)
    //    {
    //        printf("bitrate changed!\n");
    //    }
    //    else
    //    {
    //        // Getting NV_ENC_ERR_INVALID_PARAM (== 8)
    //        printf("Bitrate changing failed! Error is %d\n", status);
    //    }
    //}
}

NVENCSTATUS CNvEncoder::EncodeFrame(EncodeFrameConfig *pEncodeFrame, bool bFlush, uint32_t width, uint32_t height)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    uint32_t lockedPitch = 0;
    EncodeBuffer *pEncodeBuffer = NULL;

    if (bFlush)
    {
        FlushEncoder();
        return NV_ENC_SUCCESS;
    }

    if (!pEncodeFrame)
    {
        return NV_ENC_ERR_INVALID_PARAM;
    }

    pEncodeBuffer = m_EncodeBufferQueue.GetAvailable();
    if (!pEncodeBuffer)
    {
        m_pNvHWEncoder->ProcessOutput(m_EncodeBufferQueue.GetPending());
        pEncodeBuffer = m_EncodeBufferQueue.GetAvailable();
    }
    
    unsigned char *pInputSurface;
    
    nvStatus = m_pNvHWEncoder->NvEncLockInputBuffer(pEncodeBuffer->stInputBfr.hInputSurface, (void**)&pInputSurface, &lockedPitch);
    if (nvStatus != NV_ENC_SUCCESS)
        return nvStatus;

    if (pEncodeBuffer->stInputBfr.bufferFmt == NV_ENC_BUFFER_FORMAT_NV12_PL)
    {
        unsigned char *pInputSurfaceCh = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight*lockedPitch);
        convertYUVpitchtoNV12(pEncodeFrame->yuv[0], pEncodeFrame->yuv[1], pEncodeFrame->yuv[2], pInputSurface, pInputSurfaceCh, width, height, width, lockedPitch);
    }
    else
    {
        unsigned char *pInputSurfaceCb = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
        unsigned char *pInputSurfaceCr = pInputSurfaceCb + (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
        convertYUVpitchtoYUV444(pEncodeFrame->yuv[0], pEncodeFrame->yuv[1], pEncodeFrame->yuv[2], pInputSurface, pInputSurfaceCb, pInputSurfaceCr, width, height, width, lockedPitch);
    }
    nvStatus = m_pNvHWEncoder->NvEncUnlockInputBuffer(pEncodeBuffer->stInputBfr.hInputSurface);
    if (nvStatus != NV_ENC_SUCCESS)
        return nvStatus;

    nvStatus = m_pNvHWEncoder->NvEncEncodeFrame(pEncodeBuffer, NULL, width, height, (NV_ENC_PIC_STRUCT)m_uPicStruct);
    return nvStatus;
}