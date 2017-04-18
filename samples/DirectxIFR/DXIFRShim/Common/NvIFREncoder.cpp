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

// Nvidia GRID capture variables
#define NUMFRAMESINFLIGHT 1 // Limit is 3? Putting 4 causes an invalid parameter error to be thrown.
#define MAX_PLAYERS 4
HANDLE gpuEvent[MAX_PLAYERS];
uint8_t *bufferArray[MAX_PLAYERS];

// FFmpeg constants
#define STREAM_FRAME_RATE 30 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */
const char* encoderName = "nvenc_h264";

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
    AVCodecContext *c;

    c = avcodec_alloc_context3(*codec);
    if (!c) {
        LOG_WARN(logger, "Could not alloc an encoding context");
        exit(1);
    }
    if (contextToUse == 0)
    {
        ostArray[index].enc0 = c;
    }
    else
    {
        ostArray[index].enc1 = c;
    }    

    AVRational time_base = { 1, STREAM_FRAME_RATE };
    AVRational framerate = { STREAM_FRAME_RATE, 1 };

    //c->codec_id = codec_id;

    // This is in bits
    if (bufferHeight > 800) { // 1600x900, 1920x1080
      c->bit_rate = bitrate;
    }
    else { // 1280x720, 1366x768
        c->bit_rate = (int64_t)(bitrate * 0.75);
    }
    /* Resolution must be a multiple of two. */
    c->width = bufferWidth;
    c->height = bufferHeight;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
    * of which frame timestamps are represented. For fixed-fps content,
    * timebase should be 1/framerate and timestamp increments should be
    * identical to 1. */
    ostArray[index].st->time_base = time_base;
    c->time_base = ostArray[index].st->time_base;
    c->delay = 0;
    c->framerate = framerate;
    c->has_b_frames = 0;
    c->max_b_frames = 0;
    c->rc_min_vbv_overflow_use = (float)(c->bit_rate / STREAM_FRAME_RATE);
    c->refs = 1; // ref=1

    c->gop_size = 30; // emit one intra frame every 30 frames at most. keyint=30
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

    // GPU nvenc_h264 parameters
    av_opt_set(c->priv_data, "preset", "llhp", 0);
    av_opt_set(c->priv_data, "delay", "0", 0);

    // CPU encoding paramaters (h264)
    //av_opt_set(c->priv_data, "preset", "ultrafast", 0);
    //av_opt_set(c->priv_data, "tune", "zerolatency", 0);
    //av_opt_set(c->priv_data, "x264opts", "crf=2:vbv-maxrate=4000:vbv-bufsize=160:intra-refresh=1:slice-max-size=2000:keyint=30:ref=1", 0);

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    /* open the codec */
    int ret = avcodec_open2(c, *codec, NULL);
    if (ret < 0) {
        LOG_WARN(logger, "Could not open video codec");
        exit(1);
    }

    return c;
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
        LOG_WARN(logger, "Player " << index << ", enc0 freed");
    }
    else
    {
        avcodec_free_context(&ostArray[index].enc1);
        LOG_WARN(logger, "Player " << index << ", enc1 freed");
    }
    freeContextComplete[index] = true;
    _endthread();
}

void setupAVCodecContextProc(void* args)
{
    int index = (int)args;
    float weight = (float)playerInputArray[index] / (float)sumWeight;
    int bitrate = (int)(weight * totalBandwidthAvailable);
    AVCodec* codec = avcodec_find_encoder_by_name(encoderName);

    //if (playerInputArray[index] == 3) { // shooting
    //    bitrate = 500000;
    //}
    //else if (playerInputArray[index] == 2) { // mouse movement or any other keyboard key
    //    bitrate = 1500000;
    //}
    //else if (playerInputArray[index] == 1) { // no input
    //    bitrate = 250000;
    //}

    while (freeContextComplete[index] == false)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // This will set up context for the other ost->enc (the one currently not in use)
    codecContextArray[st[index].playerIndex] = setupAVCodecContext(&codec, &st[index].oc, bitrate, 1 - contextToUse[index], index); 
   
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

    if (sumWeight != oldSumWeight[index] && isThreadStarted[index] == false) {
        LOG_WARN(logger, "===============");
        LOG_WARN(logger, "Context currently in use for player " << index << ": " << contextToUse[index] << ", weight = [" << playerInputArray[0] << ", " << playerInputArray[1] << "]");

        st[index].oc = *oc;
        st[index].playerIndex = index;
        
        HANDLE setupThread = (HANDLE)_beginthread(&setupAVCodecContextProc, 0, (void*)index);
        BOOL success = SetThreadPriority(setupThread, THREAD_PRIORITY_HIGHEST);

        if (success == FALSE)
        {
            LOG_WARN(logger, "Set priority of setupThread failed!");
        }

        oldSumWeight[index] = sumWeight;
        isThreadStarted[index] = true;
    }
    
    // Only swap in the new AVCodecContext when the thread is done
    if (isThreadComplete[index] == true)
    {
        // Context 0 is currently in use. Context 1 has been set up by the thread and is ready
        if (contextToUse[index] == 0)
        {
            ostArray[index].enc1 = codecContextArray[index];
            LOG_WARN(logger, "Player " << index << ", Using context 1");
            
        }
        // Context 1 is currently in use. Context 0 has been set up by the thread and is ready
        else
        {
            ostArray[index].enc0 = codecContextArray[index];
            LOG_WARN(logger, "Player " << index << ", Using context 0");
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
   
	SetupFFMPEGServer(index);

    UINT uFrameCount = 0;
    DWORD dwTimeZero = timeGetTime();

    char c = '0';
    std::streampos fileSize = 0;
    std::streampos prevFileSize = 0;
    int timeBeforeIdle = 3;
    bool isIdling = false;
    time_t shootingStartTime= std::time(0);
    time_t idleStartTime = 0;

    ostringstream oss;
    oss << "G:\\Packaged Games\\414 Shipping 1-2-3 Limited 2\\WindowsNoEditor\\MyProject414\\Binaries\\Win64\\test" << index << ".txt";
    ifstream fin;

    while (!bStopEncoder)
    {
        fin.open(oss.str());
        if (fin.is_open()) 
        {
            fin.seekg(-3, ios::end); // -1 and -2 gets \n on the last line
            fileSize = fin.tellg();
            fin.get(c);
            fin.close();

            // If we shoot, we should refresh the shooting start time
            if (c == '3' && fileSize != prevFileSize)
            {
                shootingStartTime = std::time(0);
            }

            // We cannot let any other movement e.g. "c = 2" through if we have recently shot
            if ((std::time(0) - shootingStartTime) < timeBeforeIdle)
            {
                c = '3';
            }
            // No input from player - is idling
            else if (fileSize == prevFileSize)
            {
                if (isIdling == false)
                {
                    // Start the countdown
                    idleStartTime = std::time(0);
                    isIdling = true;
                }
                // We have idled for more than 3 seconds
                else if ((std::time(0) - idleStartTime) >= timeBeforeIdle)
                {
                    c = '1';
                }
            }
            // If there is input, allow countdown to restart
            else if (c == '3' || c == '2')
            {
                isIdling = false; // There is input. 
            }
            
            prevFileSize = fileSize;

            playerInputArray[index] = (c - '0');
        }
        else
        {
            LOG_ERROR(logger, "Failed to open file " << index);
        }

        // Index 0 will do the summing of the array.
        // This will pose problems in the future if player 0 can 
        // just leave while the other players are playing.
        if (index == 0)
        {
            sumWeight = 0;
            for (int i = 0; i < MAX_PLAYERS; i++)
            {
                sumWeight += playerInputArray[i];
            }
        }

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

			write_video_frame(ocArray[index], /*&ostArray[index], */bufferArray[index], index);
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
