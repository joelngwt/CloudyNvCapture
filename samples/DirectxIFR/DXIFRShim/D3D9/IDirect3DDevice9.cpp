/*!
 * \brief
 * IDirect3DDevice9 interfaces used by D3D9
 *
 * \file
 *
 * This file defines all of the IDirect3DDevice9 interfaces such as 
 * (CreateAdditionalSwapChain, Present, Reset and Release) that are 
 * overriden by this source file.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <d3d9.h>
#include "NvIFREncoderD3D9.h"
#include "IDirect3DDevice9.h"
#include "ReplaceVtbl.h"
#include "Logger.h"
#include "Util.h"

extern simplelogger::Logger *logger;
extern AppParam *pAppParam;

NvIFREncoderD3D9 *pEncoderD3D9;
static IDirect3DDevice9Vtbl vtbl;

BOOL IDirect3DDevice9_ReplaceVtbl(IDirect3DDevice9 *pIDirect3DDevice9);
void ChangeToWindowedMode(D3DPRESENT_PARAMETERS* pPresentationParameters, HWND hFocusWindow);

static HRESULT STDMETHODCALLTYPE IDirect3DDevice9_CreateAdditionalSwapChain_Proxy(IDirect3DDevice9 * This, 
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

static HRESULT STDMETHODCALLTYPE IDirect3DDevice9_GetSwapChain_Proxy(IDirect3DDevice9 * This, UINT iSwapChain, IDirect3DSwapChain9** ppSwapChain) 
{
	LOG_DEBUG(logger, __FUNCTION__);

	HRESULT hr = vtbl.GetSwapChain(This, iSwapChain, ppSwapChain);
	if (*ppSwapChain) {
		BOOL IDirect3DSwapChain9_ReplaceVtbl(IDirect3DSwapChain9* ppSwapChain);
		IDirect3DSwapChain9_ReplaceVtbl(*ppSwapChain);
	}
	return hr;
}

static HRESULT STDMETHODCALLTYPE IDirect3DDevice9_Present_Proxy(IDirect3DDevice9 * This, 
	CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) 
{
	LOG_TRACE(logger, __FUNCTION__);

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
			LOG_WARN(logger, "failed to start d3d9 encoder");
			delete pEncoderD3D9;
			pEncoderD3D9 = NULL;
		}
	}
	if (pEncoderD3D9) {
		if (!pEncoderD3D9->UpdateSharedSurface(This, pBackBuffer)) {
			LOG_ERROR(logger, "pEncoderD3D9->UpdateSharedSurface() failed");
		}
	}
	
	pBackBuffer->Release();
	return vtbl.Present(This, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

static HRESULT STDMETHODCALLTYPE IDirect3DDevice9_Reset_Proxy(IDirect3DDevice9 * This, D3DPRESENT_PARAMETERS* pPresentationParameters) 
{
	LOG_DEBUG(logger, __FUNCTION__);
	if (!pPresentationParameters->Windowed) {
		D3DDEVICE_CREATION_PARAMETERS dcp;
		vtbl.GetCreationParameters(This, &dcp);
		ChangeToWindowedMode(pPresentationParameters, dcp.hFocusWindow);
	}

	if (pEncoderD3D9) {
		pEncoderD3D9->DestroySharedSurface();
	}

	HRESULT hr = vtbl.Reset(This, pPresentationParameters);
	/*If ReplaceVtbl not called here, hooking will be lost on some games (such as "AaaaaAAaaaAAAaaaAAaAAA!!! For the awesome").*/
	IDirect3DDevice9_ReplaceVtbl(This);
	return hr;
}

static ULONG STDMETHODCALLTYPE IDirect3DDevice9_Release_Proxy(IDirect3DDevice9 * This) {
	LOG_TRACE(logger, __FUNCTION__);
	vtbl.AddRef(This);
	ULONG uRef = vtbl.Release(This) - 1;
	if (uRef == 0 && pEncoderD3D9 && pEncoderD3D9->CheckPresenter(This)) {
		LOG_DEBUG(logger, "delete pEncoderD3D9 in IDirect3DDevice9_Release_Proxy(), pEncoder=" << pEncoderD3D9);
		delete pEncoderD3D9;
		pEncoderD3D9 = NULL;
	}
	return vtbl.Release(This);
}

/* This method rewrites vtable, so must be hooked.*/
static HRESULT STDMETHODCALLTYPE IDirect3DDevice9_BeginStateBlock_Proxy(IDirect3DDevice9 * This) 
{
	HRESULT hr = vtbl.BeginStateBlock(This);
	IDirect3DDevice9_ReplaceVtbl(This);
	return hr;
}

static void SetProxy(IDirect3DDevice9Vtbl *pVtbl) 
{
	pVtbl->CreateAdditionalSwapChain = IDirect3DDevice9_CreateAdditionalSwapChain_Proxy;
	pVtbl->GetSwapChain = IDirect3DDevice9_GetSwapChain_Proxy;
	pVtbl->Present = IDirect3DDevice9_Present_Proxy;
	pVtbl->Reset = IDirect3DDevice9_Reset_Proxy;
	pVtbl->Release = IDirect3DDevice9_Release_Proxy;
	pVtbl->BeginStateBlock = IDirect3DDevice9_BeginStateBlock_Proxy;
}

BOOL IDirect3DDevice9_ReplaceVtbl(IDirect3DDevice9 *pDirect3DDevice9) 
{
	return ReplaceVtblEntries<IDirect3DDevice9Vtbl>(pDirect3DDevice9);
}
