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
#include "Util4Streamer.h"
#include <chrono>
#include <thread>

#pragma comment(lib, "winmm.lib")

extern simplelogger::Logger *logger;

#define PIXEL_SIZE 3
#define NUMFRAMESINFLIGHT 1 // Limit is 3? Putting 4 causes an invalid parameter error to be thrown.

HANDLE gpuEvent[NUMFRAMESINFLIGHT] = { NULL };
unsigned char *buffer[NUMFRAMESINFLIGHT] = { NULL};

BOOL NvIFREncoder::StartEncoder() 
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

void NvIFREncoder::FFMPEGThreadProc(int playerIndex)
{
	// Frames are written here
	pStreamer->Stream(buffer[0], pAppParam->width*pAppParam->height * PIXEL_SIZE, playerIndex); // Memory leak here

	if (playerIndex == 0)
	{
		numThreads1--;
	}
	else if (playerIndex == 1)
	{
		numThreads2--;
	}
	else if (playerIndex == 2)
	{
		numThreads3--;
	}
	else if (playerIndex == 3)
	{
		numThreads4--;
	}
	else if (playerIndex == 4)
	{
		numThreads5--;
	}
	else if (playerIndex == 5)
	{
		numThreads6--;
	}
	else if (playerIndex == 6)
	{
		numThreads7--;
	}
	else if (playerIndex == 7)
	{
		numThreads8--;
	}
	else if (playerIndex == 8)
	{
		numThreads9--;
	}
	else if (playerIndex == 9)
	{
		numThreads10--;
	}
	else if (playerIndex == 10)
	{
		numThreads11--;
	}
	else if (playerIndex == 11)
	{
		numThreads12--;
	}

	_endthread();
}

void NvIFREncoder::EncoderThreadProc() 
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
	params.eFormat = NVIFR_FORMAT_RGB;
	params.eSysStereoFormat = NVIFR_SYS_STEREO_NONE; 
	params.dwNBuffers = NUMFRAMESINFLIGHT; 
	params.dwTargetWidth = pAppParam->width;
	params.dwTargetHeight = pAppParam->height;
	params.ppPageLockedSysmemBuffers = buffer;
	params.ppTransferCompletionEvents = gpuEvent; 

	NVIFRRESULT nr = pIFR->NvIFRSetUpTargetBufferToSys(&params);

	if (nr != NVIFR_SUCCESS) {
		LOG_ERROR(logger, "NvIFRSetUpTargetBufferToSys failed, nr=" << nr);
		SetEvent(hevtInitEncoderDone);
		CleanupNvIFR();
		return;
	}
	LOG_DEBUG(logger, "NvIFRSetUpTargetBufferToSys succeeded");

	if (!pStreamer) {
		if (!pSharedStreamer) {
			// FFMPEG is started up here
			pSharedStreamer = Util4Streamer::GetStreamer(pAppParam, pAppParam->width, pAppParam->height);
		}
		pStreamer = pSharedStreamer;
	}
	if (!pStreamer->IsReady()) {
		LOG_ERROR(logger, "Cannot create FFMPEG pipe for streaming.");
		SetEvent(hevtInitEncoderDone);
		CleanupNvIFR();
		return;
	}

	bInitEncoderSuccessful = TRUE;
	SetEvent(hevtInitEncoderDone);

	numThreads1 = 0;
	numThreads2 = 0;
	numThreads3 = 0;
	numThreads4 = 0;
	numThreads5 = 0;
	numThreads6 = 0;
	numThreads7 = 0;
	numThreads8 = 0;
	numThreads9 = 0;
	numThreads10 = 0;
	numThreads11 = 0;
	numThreads12 = 0;

	while (!bStopEncoder) 
	{
		if (!UpdateBackBuffer()) 
		{
			LOG_DEBUG(logger, "UpdateBackBuffer() failed");
		}
		
		NVIFRRESULT res = pIFR->NvIFRTransferRenderTargetToSys(0);
		
		if (res == NVIFR_SUCCESS) 
		{
			DWORD dwRet = WaitForSingleObject(gpuEvent[0], INFINITE);
			if (dwRet != WAIT_OBJECT_0)// If not signalled
			{ 
				if (dwRet != WAIT_OBJECT_0 + 1) 
				{
					LOG_WARN(logger, "Abnormally break from encoding loop, dwRet=" << dwRet);
				}
				return;
			}

			if (pAppParam->numPlayers > 0 && numThreads1 <= 25)
			{
				FFMPEGThread = (HANDLE)_beginthread(FFMPEGThreadStartProc0, 0, this);
				numThreads1++;
			}
			if (pAppParam->numPlayers > 1 && numThreads2 <= 25)
			{
				FFMPEGThread = (HANDLE)_beginthread(FFMPEGThreadStartProc1, 0, this);
				numThreads2++;
			}
			if (pAppParam->numPlayers > 2 && numThreads3 <= 25)
			{
				FFMPEGThread = (HANDLE)_beginthread(FFMPEGThreadStartProc2, 0, this);
				numThreads3++;
			}
			if (pAppParam->numPlayers > 3 && numThreads4 <= 25)
			{
				FFMPEGThread = (HANDLE)_beginthread(FFMPEGThreadStartProc3, 0, this);
				numThreads4++;
			}
			if (pAppParam->numPlayers > 4 && numThreads5 <= 25)
			{
				FFMPEGThread = (HANDLE)_beginthread(FFMPEGThreadStartProc4, 0, this);
				numThreads5++;
			}
			if (pAppParam->numPlayers > 5 && numThreads6 <= 25)
			{
				FFMPEGThread = (HANDLE)_beginthread(FFMPEGThreadStartProc5, 0, this);
				numThreads6++;
			}
			if (pAppParam->numPlayers > 6 && numThreads7 <= 25)
			{
				FFMPEGThread = (HANDLE)_beginthread(FFMPEGThreadStartProc6, 0, this);
				numThreads7++;
			}
			if (pAppParam->numPlayers > 7 && numThreads8 <= 25)
			{
				FFMPEGThread = (HANDLE)_beginthread(FFMPEGThreadStartProc7, 0, this);
				numThreads8++;
			}
			if (pAppParam->numPlayers > 8 && numThreads9 <= 25)
			{
				FFMPEGThread = (HANDLE)_beginthread(FFMPEGThreadStartProc8, 0, this);
				numThreads9++;
			}
			if (pAppParam->numPlayers > 9 && numThreads10 <= 25)
			{
				FFMPEGThread = (HANDLE)_beginthread(FFMPEGThreadStartProc9, 0, this);
				numThreads10++;
			}
			if (pAppParam->numPlayers > 10 && numThreads11 <= 25)
			{
				FFMPEGThread = (HANDLE)_beginthread(FFMPEGThreadStartProc10, 0, this);
				numThreads11++;
			}
			if (pAppParam->numPlayers > 11 && numThreads12 <= 25)
			{
				FFMPEGThread = (HANDLE)_beginthread(FFMPEGThreadStartProc11, 0, this);
				numThreads12++;
			}
			ResetEvent(gpuEvent[0]);
		}
		else 
		{
			LOG_ERROR(logger, "NvIFRTransferRenderTargetToSys failed, res=" << res);
		}
		

		// Prevent doing extra work (25 FPS)
		std::this_thread::sleep_for(std::chrono::milliseconds(40));

	}
	LOG_DEBUG(logger, "Quit encoding loop");

	CleanupNvIFR();
}

Streamer * NvIFREncoder::pSharedStreamer = NULL;
