/*!
 * \brief
 * IDirect3DSwapChain9 interfaces used in conjunction with NvIFREncoderD3D9 and NvIFREncoderD3D9Ex
 *
 * \file
 *
 * This file defines all of the IDirect3DSwapChain9 interfaces (such as Present and Release)
 * that are overriden by this source file.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <d3d9.h>
#include <stdlib.h>
#include <sstream>
#include <iomanip>
#include "IDirect3DSwapChain9.h"
#include "NvIFREncoderD3D9.h"
#include "ReplaceVtbl.h"
#include "Logger.h"

extern simplelogger::Logger *logger;
extern AppParam *pAppParam;

extern NvIFREncoderD3D9 *pEncoderD3D9;

static IDirect3DSwapChain9Vtbl vtbl;

static HRESULT STDMETHODCALLTYPE IDirect3DSwapChain9_Present_Proxy(IDirect3DSwapChain9 *This, CONST RECT* pSourceRect,
	CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags) 
{
	IDirect3DDevice9 *pDevice;
	vtbl.GetDevice(This, &pDevice);
		
	IDirect3DSurface9 *pBackBuffer;
	vtbl.GetBackBuffer(This, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	D3DSURFACE_DESC desc;
	pBackBuffer->GetDesc(&desc);

	D3DPRESENT_PARAMETERS dp;
	vtbl.GetPresentParameters(This, &dp);

	if (pEncoderD3D9 && !pEncoderD3D9->CheckSize(desc.Width, desc.Height)) {
		delete pEncoderD3D9;
		pEncoderD3D9 = NULL;
	}
	HWND hwnd = hDestWindowOverride ? hDestWindowOverride : dp.hDeviceWindow;
	if (!pEncoderD3D9 && !(pAppParam && pAppParam->bDwm)
		&& !(pAppParam && pAppParam->bForceHwnd && (HWND)pAppParam->hwnd != hwnd)) {
		pEncoderD3D9 = new NvIFREncoderD3D9(This, desc.Width, desc.Height, desc.Format, pAppParam);
		if (!pEncoderD3D9->StartEncoder(0, desc.Width, desc.Height)) {
			LOG_WARN(logger, "failed to start sc_d3d9ex encoder");
			delete pEncoderD3D9;
			pEncoderD3D9 = NULL;
		}
	}
	if (pEncoderD3D9) {
		if (!pEncoderD3D9->UpdateSharedSurface(pDevice, pBackBuffer)) {
			LOG_ERROR(logger, "UpdateSharedSurface() failed");
		}
	}
	
	pBackBuffer->Release();
	pDevice->Release();
	return vtbl.Present(This, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
}

static ULONG STDMETHODCALLTYPE IDirect3DSwapChain9_Release_Proxy(IDirect3DSwapChain9 *This)
{
	LOG_TRACE(logger, __FUNCTION__);
	vtbl.AddRef(This);
	ULONG uRef = vtbl.Release(This) - 1;
	if (uRef == 0) {
		if (pEncoderD3D9 && pEncoderD3D9->CheckPresenter(This)) {
			LOG_DEBUG(logger, "delete pEncoderD3D9 in IDirect3DSwapChain9_Release_Proxy(), pEncoder=" << pEncoderD3D9);
			delete pEncoderD3D9;
			pEncoderD3D9 = NULL;
		}
		if (pEncoderD3D9 && pEncoderD3D9->CheckPresenter(This)) {
			LOG_DEBUG(logger, "delete pEncoderD3D9 in IDirect3DSwapChain9_Release_Proxy(), pEncoder=" << pEncoderD3D9);
			delete pEncoderD3D9;
			pEncoderD3D9 = NULL;
		}
	}
	return vtbl.Release(This);
}

static void SetProxy(IDirect3DSwapChain9Vtbl *pVtbl) 
{
	pVtbl->Present = IDirect3DSwapChain9_Present_Proxy;
	pVtbl->Release = IDirect3DSwapChain9_Release_Proxy;
}

BOOL IDirect3DSwapChain9_ReplaceVtbl(IDirect3DSwapChain9 *pDirect3DSwapChain9)
{
	return ReplaceVtblEntries<IDirect3DSwapChain9Vtbl>(pDirect3DSwapChain9);
}
