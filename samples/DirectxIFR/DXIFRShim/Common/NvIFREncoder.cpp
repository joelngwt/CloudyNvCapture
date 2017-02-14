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


#pragma comment(lib, "winmm.lib")

extern simplelogger::Logger *logger;

// Nvidia GRID capture variables
#define NUMFRAMESINFLIGHT 1 // Limit is 3? Putting 4 causes an invalid parameter error to be thrown.
#define MAX_PLAYERS 12
HANDLE gpuEvent[MAX_PLAYERS];
uint8_t *bufferArray[MAX_PLAYERS];

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

	LibavcodecEncoder* libavcodecEncoder;

	libavcodecEncoder->SetupFFMPEGServer(index);

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
   
			libavcodecEncoder->write_video_frame(index, bufferArray[index]);
            
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

	libavcodecEncoder->EndAndShutdown(index);
 
	CleanupNvIFR();
}

Streamer * NvIFREncoder::pSharedStreamer = NULL;
