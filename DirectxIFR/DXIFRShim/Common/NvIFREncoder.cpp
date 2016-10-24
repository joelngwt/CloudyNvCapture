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

#pragma comment(lib, "winmm.lib")

extern simplelogger::Logger *logger;

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

	UINT cxEncoding = nWidth, cyEncoding = nHeight;
	if (pAppParam && pAppParam->cxEncoding && pAppParam->cyEncoding) {
		cxEncoding = pAppParam->cxEncoding;
		cyEncoding = pAppParam->cyEncoding;
	}

	NVIFR_HW_ENC_SETUP_PARAMS params = { 0 };
	params.dwVersion = NVIFR_HW_ENC_SETUP_PARAMS_VER;
	NV_HW_ENC_CONFIG_PARAMS &encodeConfig = params.configParams;

	encodeConfig.dwVersion = NV_HW_ENC_CONFIG_PARAMS_VER;
	encodeConfig.eCodec = pAppParam && pAppParam->bHEVC ? NV_HW_ENC_HEVC : NV_HW_ENC_H264;
	LOG_DEBUG(logger, "Codec (H264=" << NV_HW_ENC_H264 << ", HEVC=" << NV_HW_ENC_HEVC << "): " << encodeConfig.eCodec);

	DWORD dwAvgBitRate = pAppParam && pAppParam->dwAvgBitRate ?
		pAppParam->dwAvgBitRate : (unsigned long long)5000000 * (cxEncoding * cyEncoding) / (1280 * 720);
	encodeConfig.dwProfile = encodeConfig.eCodec == NV_HW_ENC_HEVC ? 1 : 100;
	encodeConfig.dwFrameRateNum = nFrameRate;
	encodeConfig.dwFrameRateDen = 1;
	encodeConfig.dwAvgBitRate = dwAvgBitRate;
	encodeConfig.dwPeakBitRate = dwAvgBitRate * 12 / 10;
	encodeConfig.dwGOPLength = pAppParam && pAppParam->dwGOPLength ? pAppParam->dwGOPLength : encodeConfig.dwFrameRateNum * 5;
	encodeConfig.dwVBVBufferSize = encodeConfig.dwAvgBitRate / encodeConfig.dwFrameRateNum * 8;
	encodeConfig.dwVBVInitialDelay = encodeConfig.dwAvgBitRate / encodeConfig.dwFrameRateNum;
	encodeConfig.bRepeatSPSPPSHeader = 1;
	encodeConfig.bEnableYUV444Encoding = pAppParam ? pAppParam->bEnableYUV444Encoding : FALSE;
	encodeConfig.eRateControl = NV_HW_ENC_PARAMS_RC_CBR;
	encodeConfig.ePresetConfig = NV_HW_ENC_PRESET_LOW_LATENCY_HP;

	params.dwNBuffers = 1;
	params.dwBSMaxSize = 2048 * 1024;
	params.ppPageLockedBitStreamBuffers = &pBitStreamBuffer;
	params.ppEncodeCompletionEvents = &hevtEncodeComplete;
	params.dwTargetWidth = cxEncoding / 16 * 16;
	params.dwTargetHeight = cyEncoding / 8 * 8;

	LOG_DEBUG(logger, "Encoding Parameters:" << endl
		<< "Average Bit Rate: " << encodeConfig.dwAvgBitRate / 1024.0 / 1024.0 << "MB" << endl
		<< "Peak Bit Rate: " << encodeConfig.dwAvgBitRate / 1024.0 / 1024.0 << "MB" << endl
		<< "Frame Rate: " << encodeConfig.dwFrameRateNum << "/" << encodeConfig.dwFrameRateDen << endl
		<< "GOP Length: " << encodeConfig.dwGOPLength << endl
		<< "VBV Buffer Size: " << encodeConfig.dwVBVBufferSize << "B" << endl
		<< "VBV Initial Delay: " << encodeConfig.dwVBVInitialDelay << "B" << endl
		<< "Enable YUV444 Encoding: " << encodeConfig.bEnableYUV444Encoding << endl
		<< "Rate Control: " << encodeConfig.eRateControl << endl
		<< "Preset: " << encodeConfig.ePresetConfig
	);

	NVIFRRESULT nr = pIFR->NvIFRSetUpHWEncoder(&params);
	if (nr != NVIFR_SUCCESS) {
		LOG_ERROR(logger, "NvIFRSetUpH264HWEncoder failed, nr=" << nr);
		SetEvent(hevtInitEncoderDone);
		CleanupNvIFR();
		return;
	}
	LOG_DEBUG(logger, "NvIFRSetUpH264HWEncoder succeeded, target size: " 
		<< params.dwTargetWidth << "x" << params.dwTargetHeight);

	if (!pStreamer) {
		if (!pSharedStreamer) {
			pSharedStreamer = Util4Streamer::GetStreamer(pAppParam);
		}
		pStreamer = pSharedStreamer;
	}
	if (!pStreamer->IsReady()) {
		LOG_ERROR(logger, "Cannot create H264 file. Please check file writing permission.");
		SetEvent(hevtInitEncoderDone);
		CleanupNvIFR();
		return;
	}

	bInitEncoderSuccessful = TRUE;
	SetEvent(hevtInitEncoderDone);

	DWORD dwTimeZero = timeGetTime();
	UINT uFrameCount = 0;
	while (!bStopEncoder) {
		if (!UpdateBackBuffer()) {
			LOG_DEBUG(logger, "UpdateBackBuffer() failed");
		}

		NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS params = { 0 };
		params.dwVersion = NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS_VER;
		params.encodePicParams.dwVersion = NV_HW_ENC_PIC_PARAMS_VER;

		NVIFRRESULT res = pIFR->NvIFRTransferRenderTargetToHWEncoder(&params);
		if (res == NVIFR_SUCCESS) {
			HANDLE ahevt[] = {hevtEncodeComplete, hevtStopEncoder};
			DWORD dwRet = WaitForMultipleObjects(sizeof(ahevt)/sizeof(ahevt[0]), ahevt, FALSE, INFINITE);
			if (dwRet != WAIT_OBJECT_0) {
				if (dwRet != WAIT_OBJECT_0 + 1) {
					LOG_WARN(logger, "Abnormally break from encoding loop, dwRet=" << dwRet);
				}
				break;
			}
			ResetEvent(hevtEncodeComplete);

			NVIFR_HW_ENC_GET_BITSTREAM_PARAMS params = { 0 };
			params.dwVersion = NVIFR_HW_ENC_GET_BITSTREAM_PARAMS_VER;
			params.bitStreamParams.dwVersion = NV_HW_ENC_GET_BIT_STREAM_PARAMS_VER;
			NV_HW_ENC_GET_BIT_STREAM_PARAMS &streamParam = params.bitStreamParams;

			res = pIFR->NvIFRGetStatsFromHWEncoder(&params);

			if (res == NVIFR_SUCCESS) {
				pStreamer->Stream(pBitStreamBuffer, streamParam.dwByteSize);
			} else {
				LOG_ERROR(logger, "NvIFRGetStatsFromH264HWEncoder failed, res=" << res);
			}
		} else {
			LOG_ERROR(logger, "NvIFRTransferRenderTargetToH264HWEncoder failed, res=" << res);
		}

		int delta = (int)((dwTimeZero + ++uFrameCount * 1000 / nFrameRate) - timeGetTime());
		if (delta > 0) {
			WaitForSingleObject(hevtStopEncoder, delta);
		} else {
			LOG_DEBUG(logger, "No sleep to catch up frame rate, delta=" << delta);
		}
	}
	LOG_DEBUG(logger, "Quit encoding loop");

	CleanupNvIFR();
}

Streamer * NvIFREncoder::pSharedStreamer = NULL;
