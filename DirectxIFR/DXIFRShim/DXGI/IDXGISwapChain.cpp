/*!
 * \brief
 * IDXGISwapChainVtbl interfaces used by DX10 and DX11
 *
 * \file
 *
 * This file defines all of the IDXGISwapChain::Present, SetFullscreenState,
 * and Release interfaces that are overriden by this source file.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <windows.h>
#include <Psapi.h>
#include <d3d11.h>
#include <d3d10.h>
#include <stdio.h>
#include <string>
#include <time.h>
#include "IDXGISwapChain.h"
#include "NvIFREncoderDXGI.h"
#include "ReplaceVtbl.h"
#include "Logger.h"

extern simplelogger::Logger *logger;
extern AppParam *pAppParam;

static IDXGISwapChainVtbl vtbl;
static NvIFREncoder *pEncoder;

inline HWND GetOutputWindow(IDXGISwapChain * This) 
{
	DXGI_SWAP_CHAIN_DESC desc;
	vtbl.GetDesc(This, &desc);
	return desc.OutputWindow;
}

static HRESULT STDMETHODCALLTYPE IDXGISwapChain_Present_Proxy(IDXGISwapChain * This, UINT SyncInterval, UINT Flags) 
{
	IUnknown *pIUnkown;
	vtbl.GetDevice(This, __uuidof(pIUnkown), reinterpret_cast<void **>(&pIUnkown));
	ID3D10Device *pD3D10Device;
	ID3D11Device *pD3D11Device;
	if (pIUnkown->QueryInterface(__uuidof(pD3D10Device), (void **)&pD3D10Device) == S_OK) {
		ID3D10Texture2D *pBackBuffer;
		vtbl.GetBuffer(This, 0, __uuidof(pBackBuffer), reinterpret_cast<void**>(&pBackBuffer));
		D3D10_TEXTURE2D_DESC desc;
		pBackBuffer->GetDesc(&desc);

		if (pEncoder && !pEncoder->CheckSize(desc.Width, desc.Height)) {
			LOG_INFO(logger, "destroy d3d10 encoder, new size: " << desc.Width << "x" << desc.Height);
			delete pEncoder;
			pEncoder = NULL;
		}
		if (!pEncoder && !(pAppParam && pAppParam->bDwm) 
				&& !(pAppParam && pAppParam->bForceHwnd && (HWND)pAppParam->hwnd != GetOutputWindow(This))) {
			/* On Windows 7, we can QueryInterface() to tell whether this is a D3D10.1 device and 
			   prefer KeyedMutex resource when yes. But on Windows 8.1, any D3D10 device has D3D10.1
			   interface, but the queried interface still can't open KeyedMutex resource. So for
			   compatibility, we stick to non-KeyedMutex resource without checking D3D10.1.*/
			pEncoder = new NvIFREncoderDXGI<ID3D10Device, ID3D10Texture2D>(This, desc.Width, desc.Height, 
				desc.Format, FALSE, pAppParam);
			
			if (!pEncoder->StartEncoder()) {
				LOG_WARN(logger, "failed to start d3d10 encoder");
				delete pEncoder;
				pEncoder = NULL;
			}
		}
		if (pEncoder) {
			if (!((NvIFREncoderDXGI<ID3D10Device, ID3D10Texture2D> *)pEncoder)->UpdateSharedSurface(pD3D10Device, pBackBuffer)) {
				LOG_WARN(logger, "d3d10 UpdateSharedSurface failed");
			}
		}

		pBackBuffer->Release();
		pD3D10Device->Release();
	} else if (pIUnkown->QueryInterface(__uuidof(pD3D11Device), (void **)&pD3D11Device) == S_OK) {
		ID3D11Texture2D *pBackBuffer;
		vtbl.GetBuffer(This, 0, __uuidof(pBackBuffer), reinterpret_cast<void**>(&pBackBuffer));
		D3D11_TEXTURE2D_DESC desc;
		pBackBuffer->GetDesc(&desc);

		if (pEncoder && !pEncoder->CheckSize(desc.Width, desc.Height)) {
			LOG_INFO(logger, "destroy d3d11 encoder, new size: " << desc.Width << "x" << desc.Height);
			delete pEncoder;
			pEncoder = NULL;
		}
		if (!pEncoder && !(pAppParam && pAppParam->bDwm) 
				&& !(pAppParam && pAppParam->bForceHwnd && (HWND)pAppParam->hwnd != GetOutputWindow(This))) {
			pEncoder = new NvIFREncoderDXGI<ID3D11Device, ID3D11Texture2D>(This, desc.Width, desc.Height, 
				desc.Format, FALSE, pAppParam);
			if (!pEncoder->StartEncoder()) {
				LOG_WARN(logger, "failed to start d3d11 encoder");
				delete pEncoder;
				pEncoder = NULL;
			}
		}
		if (pEncoder) {
			if (!((NvIFREncoderDXGI<ID3D11Device, ID3D11Texture2D> *)pEncoder)->UpdateSharedSurface(pD3D11Device, pBackBuffer)) {
				LOG_WARN(logger, "d3d11 UpdateSharedSurface failed");
			}
		}

		pBackBuffer->Release();
		pD3D11Device->Release();
	} else {
		LOG_ERROR(logger, "D3DxDevice not supported");
		return vtbl.Present(This, SyncInterval, Flags);
	}
	pIUnkown->Release();

	return vtbl.Present(This, 0, Flags);
}

static HRESULT STDMETHODCALLTYPE IDXGISwapChain_SetFullscreenState_Proxy(IDXGISwapChain * This, 
	BOOL Fullscreen, IDXGIOutput *pTarget)
{
	return vtbl.SetFullscreenState(This, FALSE, pTarget);
}

static ULONG STDMETHODCALLTYPE IDXGISwapChain_Release_Proxy(IDXGISwapChain * This)
{
	LOG_TRACE(logger, __FUNCTION__);
	vtbl.AddRef(This);
	ULONG uRef = vtbl.Release(This) - 1;
	if (uRef == 0 && pEncoder && pEncoder->CheckPresenter(This)) {
		LOG_DEBUG(logger, "delete pEncoder in Release(), pEncoder=" << pEncoder);
		delete pEncoder;
		pEncoder = NULL;
	}
	return vtbl.Release(This);
}

static void SetProxy(IDXGISwapChainVtbl *pVtbl)
{
	pVtbl->Present = IDXGISwapChain_Present_Proxy;
	pVtbl->SetFullscreenState = IDXGISwapChain_SetFullscreenState_Proxy;
	pVtbl->Release = IDXGISwapChain_Release_Proxy;
}

BOOL IDXGISwapChain_ReplaceVtbl(IDXGISwapChain *pSwapChain) 
{
	return ReplaceVtblEntries<IDXGISwapChainVtbl>(pSwapChain);
}
