/*!
 * \brief
 * NvIFREncoderDXGI (derived from NvIFREncoder) is the interface used in 
 * hooked D3D10/11 API calls.
 *
 * \file
 * NvIFREncoderDXGI is extended from NvIFREncoderDXGIBase.
 * UpdateSharedSurface() is specific to NvIFREncoderDXGI. If keyed mutex 
 * is not available, the shared surface is opened/closed every time 
 * UpdateSharedSurface() is called; otherwise, the shared resource is 
 * always opened, and the keyed mutex is used to improve performance.
 * 
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma once

#include <NvIFRLibrary.h>
#include "NvIFREncoder.h"
#include "Logger.h"

extern simplelogger::Logger *logger;

class NvIFREncoderDXGIBase : public NvIFREncoder {
public:
	NvIFREncoderDXGIBase(void *pPresenter, int nWidth, int nHeight, DXGI_FORMAT d3dFormat, BOOL bKeyedMutex, AppParam *pAppParam, Streamer *pStreamer = NULL) :
		NvIFREncoder(pPresenter, nWidth, nHeight, d3dFormat, bKeyedMutex, pAppParam, pStreamer),
		pDevice(NULL), pSwapChain(NULL), pBackBuffer(NULL), pRenderTargetView(NULL),
		pSharedTexture(NULL), pStagingTexture(NULL), pCommitTexture(NULL) {}
	~NvIFREncoderDXGIBase()
	{
		if (!bStopEncoder) {
			StopEncoder();
		}
	}

	virtual BOOL UpdateBackBuffer();

protected:
	virtual BOOL SetupNvIFR();
	virtual void CleanupNvIFR();

	BOOL SetBackBufferContent(BYTE *pData)
	{
		D3D10_MAPPED_TEXTURE2D mappedTexture;
		HRESULT hr = pStagingTexture->Map(0, D3D10_MAP_WRITE, 0, &mappedTexture);
		if (FAILED(hr)) {
			LOG_ERROR(logger, __FUNCTION__ << " Map hr=" << hr);
			return FALSE;
		}
		memcpy(mappedTexture.pData, pData, nWidth * nHeight * 4);
		pStagingTexture->Unmap(0);
		pDevice->CopyResource(pBackBuffer, pStagingTexture);
		return TRUE;
	}

private:
	ID3D10Device1 *pDevice;
	IDXGISwapChain *pSwapChain;
	ID3D10Texture2D *pBackBuffer;
	ID3D10RenderTargetView *pRenderTargetView;
	ID3D10Texture2D *pSharedTexture;
	ID3D10Texture2D *pStagingTexture;
	ID3D10Texture2D *pCommitTexture;
};
