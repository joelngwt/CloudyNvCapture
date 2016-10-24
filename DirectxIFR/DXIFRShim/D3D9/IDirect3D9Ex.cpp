/*!
 * \brief
 * IDirect3D9Ex interfaces used by D3D9Ex
 *
 * \file
 *
 * This file defines all of the IDirect3D9Ex interfaces (such as 
 * CreateDevice, CreateDeviceEx, and GetAdapterCount) that are 
 * overriden by this source file.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */


#include <d3d9.h>
#include <stdlib.h>
#include "IDirect3D9Ex.h"
#include "Logger.h"
#include "ReplaceVtbl.h"
#include "GridAdapter.h"
#include "AppParam.h"

extern simplelogger::Logger *logger;
extern AppParam *pAppParam;

static IDirect3D9ExVtbl vtbl;

void ChangeToWindowedMode(D3DPRESENT_PARAMETERS* pPresentationParameters, HWND hFocusWindow);

static HRESULT STDMETHODCALLTYPE IDirect3D9Ex_CreateDevice_Proxy(IDirect3D9Ex *This, UINT Adapter, D3DDEVTYPE DeviceType, 
	HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, 
	IDirect3DDevice9** ppReturnedDeviceInterface) 
{
	LOG_DEBUG(logger, __FUNCTION__);

	if (!pPresentationParameters->Windowed) {
		ChangeToWindowedMode(pPresentationParameters, hFocusWindow);
	}

	int iAdapter = GetGridAdapterOrdinal_D3D9(pAppParam ? pAppParam->iGpu : -1,
		AdapterAccessor_D3D9<IDirect3D9Ex>(This, vtbl.GetAdapterCount, vtbl.GetAdapterIdentifier, vtbl.GetAdapterMonitor));

	HRESULT hr = vtbl.CreateDevice(This, iAdapter, DeviceType, hFocusWindow, BehaviorFlags, 
		pPresentationParameters, ppReturnedDeviceInterface);
	if (FAILED(hr)) {
		return hr;
	}

	BOOL IDirect3DDevice9_ReplaceVtbl(IDirect3DDevice9 *pDirect3DDevice9);
	IDirect3DDevice9_ReplaceVtbl(*ppReturnedDeviceInterface);

	return hr;
}

static HRESULT STDMETHODCALLTYPE IDirect3D9Ex_CreateDeviceEx_Proxy(IDirect3D9Ex *This, UINT Adapter, 
	D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters,
	D3DDISPLAYMODEEX* pFullscreenDisplayMode, IDirect3DDevice9Ex** ppReturnedDeviceInterface) 
{
	LOG_DEBUG(logger, __FUNCTION__);

	if (!pPresentationParameters->Windowed) {
		ChangeToWindowedMode(pPresentationParameters, hFocusWindow);
	}

	int iAdapter = GetGridAdapterOrdinal_D3D9(pAppParam ? pAppParam->iGpu : -1,
		AdapterAccessor_D3D9<IDirect3D9Ex>(This, vtbl.GetAdapterCount, vtbl.GetAdapterIdentifier, vtbl.GetAdapterMonitor));

	// Replace pFullscreenDisplayMode with NULL
	HRESULT hr = vtbl.CreateDeviceEx(This, iAdapter, DeviceType, hFocusWindow, BehaviorFlags, 
		pPresentationParameters, NULL, ppReturnedDeviceInterface);
	if (FAILED(hr)) {
		return hr;
	}

	BOOL IDirect3DDevice9Ex_ReplaceVtbl(IDirect3DDevice9Ex *pDirect3DDevice9);
	IDirect3DDevice9Ex_ReplaceVtbl(*ppReturnedDeviceInterface);

	return hr;
}

static void SetProxy(IDirect3D9ExVtbl *pVtbl) 
{
	pVtbl->CreateDevice = IDirect3D9Ex_CreateDevice_Proxy;
	pVtbl->CreateDeviceEx = IDirect3D9Ex_CreateDeviceEx_Proxy;
}

BOOL IDirect3D9Ex_ReplaceVtbl(IDirect3D9Ex *pDirect3D9Ex) 
{
	return ReplaceVtblEntries<IDirect3D9ExVtbl>(pDirect3D9Ex);
}
