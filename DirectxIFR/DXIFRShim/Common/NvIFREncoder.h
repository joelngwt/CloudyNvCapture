/*!
 * \brief
 * Base class of NvIFREncoder for which the D3D interfaces are derived.
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

#pragma once

#include <d3d9.h>
#include <d3d10_1.h>
#include <d3d11.h>
#include <windows.h>
#include <NvIFR/NvIFR.h>
#include <NvIFR/nvIFRHWEnc.h>
#include "Logger.h"
#include "AppParam.h"
#include "GridAdapter.h"
#include "Streamer.h"

class NvIFREncoder {
public:
	NvIFREncoder(void *pPresenter, int nWidth, int nHeight, DXGI_FORMAT dxgiFormat, 
		BOOL bKeyedMutex, AppParam *pAppParam, Streamer *pStreamer = NULL) :
		pPresenter(pPresenter),
		nWidth(nWidth), nHeight(nHeight), 
		dxgiFormat(dxgiFormat),
		bKeyedMutex(bKeyedMutex), 
		pAppParam(pAppParam), pStreamer(pStreamer),
		bStopEncoder(TRUE), pIFR(NULL), hSharedTexture(NULL),
		nFrameRate(50),
		szClassName("NvIFREncoder"),
		pBitStreamBuffer(NULL), hevtEncodeComplete(NULL),
		bInitEncoderSuccessful(FALSE), hevtInitEncoderDone(NULL), hthEncoder(NULL), hevtStopEncoder(NULL)
	{}
	virtual ~NvIFREncoder() 
	{
	/* Although every subclass calls StopEncoder() in its destructor, the common
	   code can't be placed here, because StopEncoder() will cause virtual function 
	   CleanupNvIFR() to get called in EncoderThreadProc(). That's an error.
	   See Effective C++, 3rd Edition, Item 9: Never Call Virtual Functions during 
	   Construction or Destruction*/
	}

public:
	virtual BOOL StartEncoder();
	virtual void StopEncoder();
	BOOL CheckSize(int nWidth, int nHeight) {
		return this->nWidth == nWidth && this->nHeight == nHeight;
	}
	BOOL CheckPresenter(void *pPresenter) {
		return this->pPresenter == pPresenter;
	}

protected:
	/*Whether successfull or not, invocation of SetupNvIFR() must be paired 
	  with CleanupNvIFR() to avoid memory leak.*/
	virtual BOOL SetupNvIFR() {
		WNDCLASSEX wc = {
			sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L,
			GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
			szClassName, NULL
		};
		RegisterClassEx(&wc);
		//Create the window needed by NvIFR encoding device
		hwndEncoder = CreateWindow(szClassName, szClassName,
			WS_OVERLAPPEDWINDOW, 0, 0, nWidth, nHeight,
			NULL, NULL, wc.hInstance, NULL);
		return TRUE;
	}
	virtual void CleanupNvIFR() {
		DestroyWindow(hwndEncoder);
		UnregisterClass(szClassName, GetModuleHandle(NULL));
	}
	virtual BOOL UpdateBackBuffer() = 0;

private:
	void EncoderThreadProc();

	static void EncoderThreadStartProc(void *args) 
	{
		((NvIFREncoder *)args)->EncoderThreadProc();
	}

protected:
	int nWidth, nHeight;
	DXGI_FORMAT dxgiFormat;
	HWND hwndEncoder;
	BOOL bStopEncoder;

	INvIFRToHWEncoder_v1 *pIFR;
	HANDLE hSharedTexture;
	BOOL bKeyedMutex;

	AppParam *pAppParam;

	static inline HRESULT CreateCommitSurface(IDirect3DDevice9 *pDeviceX, D3DFORMAT format, IDirect3DSurface9 **ppCommitSurfaceX)
	{
		return pDeviceX->CreateRenderTarget(1, 1, format, D3DMULTISAMPLE_NONE, 0, TRUE, ppCommitSurfaceX, NULL);
	}
	static inline HRESULT CreateCommitTexture(ID3D10Device *pDeviceX, DXGI_FORMAT format, ID3D10Texture2D **ppCommitTextureX) 
	{
		D3D10_TEXTURE2D_DESC desc = {
			1, 1, 1, 1, format, {1, 0}, 
			D3D10_USAGE_STAGING, 0, 
			D3D10_CPU_ACCESS_READ, 0
		};
		return pDeviceX->CreateTexture2D(&desc, NULL, ppCommitTextureX);
	}
	static inline HRESULT CreateCommitTexture(ID3D11Device *pDeviceX, DXGI_FORMAT format, ID3D11Texture2D **ppCommitTextureX) 
	{
		D3D11_TEXTURE2D_DESC desc = {
			1, 1, 1, 1, format, {1, 0}, 
			D3D11_USAGE_STAGING, 0, 
			D3D11_CPU_ACCESS_READ, 0
		};
		return pDeviceX->CreateTexture2D(&desc, NULL, ppCommitTextureX);
	}

	static inline BOOL CommitUpdate(IDirect3DDevice9 *pDeviceX, IDirect3DSurface9 *pCommitSurfaceX, IDirect3DSurface9 *pSurfaceX)
	{
		RECT rect = {0, 0, 1, 1};
		HRESULT hr = pDeviceX->StretchRect(pSurfaceX, &rect, pCommitSurfaceX, &rect, D3DTEXF_NONE);
		if (FAILED(hr)) {
			LOG_ERROR(logger, __FUNCTION__ << " " << __LINE__ << " StretchRect(), hr=" << hr);
			return hr;
		}
		D3DLOCKED_RECT lockedRect;
		hr = pCommitSurfaceX->LockRect(&lockedRect, NULL, D3DLOCK_READONLY);
		pCommitSurfaceX->UnlockRect();
		return hr;
	}
	static inline BOOL CommitUpdate(ID3D10Device *pDeviceX, ID3D10Texture2D *pCommitTextureX, ID3D10Texture2D *pTextureX) 
	{
		static D3D10_BOX box = {0, 0, 0, 1, 1, 1};
		pDeviceX->CopySubresourceRegion(pCommitTextureX, 0, 0, 0, 0, pTextureX, 0, &box);
		D3D10_MAPPED_TEXTURE2D mappedTexture;
		HRESULT hr = pCommitTextureX->Map(0, D3D10_MAP_READ, 0, &mappedTexture);
		if (FAILED(hr)) {
			LOG_ERROR(logger, "Map hr=" << hr);
			return FALSE;
		}
		BYTE c = *(BYTE*)mappedTexture.pData;
		pCommitTextureX->Unmap(0);
		return TRUE;
	}
	static inline BOOL CommitUpdate(ID3D11Device *pDeviceX, ID3D11Texture2D *pCommitTextureX, ID3D11Texture2D *pTextureX) 
	{
		static D3D11_BOX box = {0, 0, 0, 1, 1, 1};
		ID3D11DeviceContext *pContextX = NULL;
		pDeviceX->GetImmediateContext(&pContextX);
		pContextX->CopySubresourceRegion(pCommitTextureX, 0, 0, 0, 0, pTextureX, 0, &box);
		D3D11_MAPPED_SUBRESOURCE mappedTexture;
		HRESULT hr = pContextX->Map(pCommitTextureX, 0, D3D11_MAP_READ, 0, &mappedTexture);
		if (FAILED(hr)) {
			LOG_ERROR(logger, "Map hr=" << hr);
			return FALSE;
		}
		BYTE c = *(BYTE*)mappedTexture.pData;
		pContextX->Unmap(pCommitTextureX, 0);
		pContextX->Release();
		return TRUE;
	}

private:
	const int nFrameRate;
	const char *szClassName;
	const void *pPresenter;

	BYTE *pBitStreamBuffer;
	HANDLE hevtEncodeComplete;

	BOOL bInitEncoderSuccessful;
	HANDLE hevtInitEncoderDone;
	HANDLE hthEncoder;
	HANDLE hevtStopEncoder;

	Streamer *pStreamer;
	static Streamer *pSharedStreamer;
};
