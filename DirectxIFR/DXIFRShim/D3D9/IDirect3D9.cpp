/*!
 * \brief
 * IDirect3D9 interfaces used by D3D9
 *
 * \file
 *
 * This file defines all of the IDXGIFactory interfaces (such as 
 * CreateDevice and GetAdapterCount) that are overriden by this 
 * source file.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <d3d9.h>
#include <stdlib.h>
#include "IDirect3D9.h"
#include "Logger.h"
#include "ReplaceVtbl.h"
#include "GridAdapter.h"
#include "AppParam.h"

extern simplelogger::Logger *logger;
extern AppParam *pAppParam;

static IDirect3D9Vtbl vtbl;

void ChangeToWindowedMode(D3DPRESENT_PARAMETERS* pPresentationParameters, HWND hFocusWindow) 
{
	pPresentationParameters->Windowed = TRUE;
	pPresentationParameters->FullScreen_RefreshRateInHz = 0;

	HWND hwnd = hFocusWindow != NULL ? hFocusWindow : pPresentationParameters->hDeviceWindow;
	DWORD dwStyle = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME;
	SetWindowLongPtr(hwnd, GWL_STYLE, dwStyle);
	SetWindowLongPtr(hwnd, GWL_EXSTYLE, 0);

	RECT rect = {0, 0, pPresentationParameters->BackBufferWidth, pPresentationParameters->BackBufferHeight};
	AdjustWindowRect(&rect, dwStyle, FALSE);
	SetWindowPos(hwnd, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW | SWP_NOMOVE);
}

static HRESULT STDMETHODCALLTYPE IDirect3D9_CreateDevice_Proxy(IDirect3D9 *This, UINT Adapter, D3DDEVTYPE DeviceType, 
	HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, 
	IDirect3DDevice9** ppReturnedDeviceInterface)
{
	LOG_DEBUG(logger, __FUNCTION__);

	if (!pPresentationParameters->Windowed) {
		ChangeToWindowedMode(pPresentationParameters, hFocusWindow);
	}

	int iAdapter = GetGridAdapterOrdinal_D3D9(pAppParam ? pAppParam->iGpu : -1,
		AdapterAccessor_D3D9<IDirect3D9>(This, vtbl.GetAdapterCount, vtbl.GetAdapterIdentifier, vtbl.GetAdapterMonitor));

	HRESULT hr = vtbl.CreateDevice(This, iAdapter, DeviceType, hFocusWindow, BehaviorFlags, 
		pPresentationParameters, ppReturnedDeviceInterface);
	LOG_DEBUG(logger, "This=0x" << This << ", *ppReturnedDeviceInterface=0x" << *ppReturnedDeviceInterface);
	if (FAILED(hr)) {
		return hr;
	}

	BOOL IDirect3DDevice9_ReplaceVtbl(IDirect3DDevice9 *pDirect3DDevice9);
	IDirect3DDevice9_ReplaceVtbl(*ppReturnedDeviceInterface);

	return hr;
}

static void SetProxy(IDirect3D9Vtbl *pVtbl) 
{
	pVtbl->CreateDevice = IDirect3D9_CreateDevice_Proxy;
}

BOOL IDirect3D9_ReplaceVtbl(IDirect3D9 *pDirect3D9) 
{
	return ReplaceVtblEntries<IDirect3D9Vtbl>(pDirect3D9);
}
