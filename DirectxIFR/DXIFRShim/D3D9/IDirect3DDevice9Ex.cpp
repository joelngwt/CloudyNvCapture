/*!
 * \brief
 * IDirect3DDevice9Ex interfaces used by D3D9Ex
 *
 * \file
 *
 * This file defines all of the IDirect3DDevice9Ex interfaces (such as 
 * CreateAdditionalSwapChain, Present, Reset and Release)
 * that are overriden by this source file.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <d3d9.h>
#include "NvIFREncoderD3D9.h"
#include "IDirect3DDevice9Ex.h"
#include "ReplaceVtbl.h"
#include "Logger.h"
#include "Util.h"

extern simplelogger::Logger *logger;
extern AppParam *pAppParam;

extern NvIFREncoderD3D9 *pEncoderD3D9;
static IDirect3DDevice9ExVtbl vtbl;

BOOL IDirect3DDevice9Ex_ReplaceVtbl(IDirect3DDevice9Ex *pDirect3DDevice9);
void ChangeToWindowedMode(D3DPRESENT_PARAMETERS* pPresentationParameters, HWND hFocusWindow);

static HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateAdditionalSwapChain_Proxy(IDirect3DDevice9Ex * This, 
	D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** ppSwapChain) 
{
	LOG_DEBUG(logger, __FUNCTION__);

	if (!pPresentationParameters->Windowed) {
		D3DDEVICE_CREATION_PARAMETERS dcp;
		vtbl.GetCreationParameters(This, &dcp);
		ChangeToWindowedMode(pPresentationParameters, dcp.hFocusWindow);
	}

	HRESULT hr = vtbl.CreateAdditionalSwapChain(This, pPresentationParameters, ppSwapChain);
	if (*ppSwapChain) {
		BOOL IDirect3DSwapChain9_ReplaceVtbl(IDirect3DSwapChain9* ppSwapChain);
		IDirect3DSwapChain9_ReplaceVtbl(*ppSwapChain);
	}
	return hr;
}

static void IDirect3DDevice9Ex_Present_Proxy_Common(IDirect3DDevice9Ex * This, HWND hDestWindowOverride) 
{
	IDirect3DSurface9 *pBackBuffer;
	vtbl.GetBackBuffer(This, 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	D3DSURFACE_DESC desc;
	pBackBuffer->GetDesc(&desc);

	if (pEncoderD3D9 && !pEncoderD3D9->CheckSize(desc.Width, desc.Height)) {
		delete pEncoderD3D9;
		pEncoderD3D9 = NULL;
	}
	if (!pEncoderD3D9 && !(pAppParam && pAppParam->bDwm) && !(pAppParam && pAppParam->bForceHwnd 
			&& (HWND)pAppParam->hwnd != (hDestWindowOverride ? hDestWindowOverride : GetDeviceWindow(vtbl, This)))) {
		pEncoderD3D9 = new NvIFREncoderD3D9(This, desc.Width, desc.Height, desc.Format, pAppParam);
		if (!pEncoderD3D9->StartEncoder()) {
			LOG_WARN(logger, "failed to start d3d9ex encoder");
			delete pEncoderD3D9;
			pEncoderD3D9 = NULL;
		}
	}
	if (pEncoderD3D9) {
		if (!pEncoderD3D9->UpdateSharedSurface(This, pBackBuffer)) {
			LOG_ERROR(logger, "pEncoderD3D9Ex->UpdateSharedSurface() failed");
		}
	}

	pBackBuffer->Release();
}

static HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_Present_Proxy(IDirect3DDevice9Ex * This, 
	CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) 
{
	LOG_TRACE(logger, __FUNCTION__);
	IDirect3DDevice9Ex_Present_Proxy_Common(This, hDestWindowOverride);
	return vtbl.Present(This, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

static HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_PresentEx_Proxy(IDirect3DDevice9Ex * This, 
	CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags) 
{
	LOG_TRACE(logger, __FUNCTION__);
	IDirect3DDevice9Ex_Present_Proxy_Common(This, hDestWindowOverride);
	return vtbl.PresentEx(This, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
}

static HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_Reset_Proxy(IDirect3DDevice9Ex * This, D3DPRESENT_PARAMETERS* pPresentationParameters) 
{
	if (!pPresentationParameters->Windowed) {
		D3DDEVICE_CREATION_PARAMETERS dcp;
		vtbl.GetCreationParameters(This, &dcp);
		ChangeToWindowedMode(pPresentationParameters, dcp.hFocusWindow);
	}

	return vtbl.Reset(This, pPresentationParameters);
}

static ULONG STDMETHODCALLTYPE IDirect3DDevice9Ex_Release_Proxy(IDirect3DDevice9Ex * This) {
	LOG_TRACE(logger, __FUNCTION__);
	vtbl.AddRef(This);
	ULONG uRef = vtbl.Release(This) - 1;
	if (uRef == 0 && pEncoderD3D9 && pEncoderD3D9->CheckPresenter(This)) {
		LOG_DEBUG(logger, "delete pEncoderD3D9Ex in IDirect3DDevice9Ex_Release_Proxy(), pEncoder=" << pEncoderD3D9);
		delete pEncoderD3D9;
		pEncoderD3D9 = NULL;
	}
	return vtbl.Release(This);
}

static void SetProxy(IDirect3DDevice9ExVtbl *pVtbl) 
{
	pVtbl->CreateAdditionalSwapChain = IDirect3DDevice9Ex_CreateAdditionalSwapChain_Proxy;
	pVtbl->Present = IDirect3DDevice9Ex_Present_Proxy;
	pVtbl->PresentEx = IDirect3DDevice9Ex_PresentEx_Proxy;
	pVtbl->Reset = IDirect3DDevice9Ex_Reset_Proxy;
	pVtbl->Release = IDirect3DDevice9Ex_Release_Proxy;
}

BOOL IDirect3DDevice9Ex_ReplaceVtbl(IDirect3DDevice9Ex *pDirect3DDevice9) 
{
	return ReplaceVtblEntries<IDirect3DDevice9ExVtbl>(pDirect3DDevice9);
}
