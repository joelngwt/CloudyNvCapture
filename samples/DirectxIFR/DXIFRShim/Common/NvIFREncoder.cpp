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

#include "../DXGI/NvEncoder.h"

#pragma comment(lib, "winmm.lib")

extern simplelogger::Logger *logger;

// Nvidia GRID capture variables
#define NUMFRAMESINFLIGHT 1 // Limit is 3? Putting 4 causes an invalid parameter error to be thrown.
#define MAX_PLAYERS 4
HANDLE gpuEvent[MAX_PLAYERS];
uint8_t *bufferArray[MAX_PLAYERS];

// Streaming constants
#define STREAM_FRAME_RATE 30 // Number of images per second

// Input and Output video size
int bufferWidth;
int bufferHeight;

// Bit rate switching variables
const int bandwidthPerPlayer = 2000000;
int totalBandwidthAvailable = 0;
int sumWeight = 0;
int playerInputArray[MAX_PLAYERS] = { 0 };

// Function to use to measure time elapsed
LONGLONG g_llBegin1 = 0;
LONGLONG g_llPerfFrequency1 = 0;
BOOL g_timeInitialized1 = FALSE;

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

    // Initialization of Nvidia Codec SDK parameters
    int currentBitrate = 2500000;
    int targetBitrate = currentBitrate;

    // To sleep if encoding is going faster than framerate of the game
    UINT uFrameCount = 0;
    DWORD dwTimeZero = timeGetTime();

    char c = '0';
    int timeBeforeIdle = 3;
    bool isIdling = false;
    time_t shootingStartTime = std::time(0);
    time_t idleStartTime = 0;

    // To read the player input data for adaptive bitrate
    TCHAR szPath[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, szPath);
    ostringstream oss;
    oss << szPath << "\\test" << index << ".txt";    
    ifstream fin;

    // Setup Nvidia Video Codec SDK
    CNvEncoder nvEncoder(index);
    nvEncoder.EncodeMain(index, bufferWidth, bufferHeight, STREAM_FRAME_RATE, currentBitrate);

    while (!bStopEncoder)
    {
        fin.open(oss.str());
        if (fin.is_open())
        {
            // Always read the value at the end of the file
            fin.seekg(-3, ios::end); // -1 and -2 gets \n on the last line
            fin.get(c);
            fin.close();
        
            // If we shoot, we should refresh the shooting start time
            if (c == '3')
            {
                shootingStartTime = std::time(0);
            }
        
            // We cannot let any other movement e.g. "c = 2" through if we have recently shot
            if ((std::time(0) - shootingStartTime) < timeBeforeIdle)
            {
                c = '3';
            }
            // No input from player - is idling
            else if (c == '1')
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

            // Adaptive bitrate - independent of other players
            //if (playerInputArray[index] == 3) { // shooting
            //    targetBitrate = 5000000;
            //}
            //else if (playerInputArray[index] == 2) { // mouse movement or any other keyboard key
            //    targetBitrate = 2500000;
            //}
            //else if (playerInputArray[index] == 1) { // no input
            //    targetBitrate = 500000;
            //}

            // Adaptive bitrate - depends on other players
            float weight = (float)playerInputArray[index] / (float)sumWeight;
            targetBitrate = (int)(weight * totalBandwidthAvailable);
            
            if (targetBitrate != currentBitrate)
            {
                nvEncoder.EncodeFrameLoop(bufferArray[index], true, index, targetBitrate);
                currentBitrate = targetBitrate;
            }
            else
            {
                nvEncoder.EncodeFrameLoop(bufferArray[index], false, index, targetBitrate);
            }
            //write_video_frame(ocArray[index], /*&ostArray[index], */bufferArray[index], index);
        }
        else
        {
            LOG_ERROR(logger, "NvIFRTransferRenderTargetToSys failed, res=" << res);
        }

        // This sleeps the thread if we are producing frames faster than the desired framerate
        int delta = (int)((dwTimeZero + ++uFrameCount * 1000 / STREAM_FRAME_RATE) - timeGetTime());
        if (delta > 0) {
            WaitForSingleObject(hevtStopEncoder, delta);
        }
    }
    LOG_DEBUG(logger, "Quit encoding loop");

    nvEncoder.ShutdownNvEncoder();
    CleanupNvIFR();
}

Streamer * NvIFREncoder::pSharedStreamer = NULL;
