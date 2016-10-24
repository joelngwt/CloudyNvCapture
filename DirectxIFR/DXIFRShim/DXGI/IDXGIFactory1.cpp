/*!
 * \brief
 * DXGIFactory1 interfaces used by DX10 and DX11
 *
 * \file
 *
 * This file defines all of the IDXGIFactory1::CreateSwapChain, 
 * and EnumAdapters1 interfaces that are overriden by this source file.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <vector>
#include "IDXGIFactory1.h"
#include "ReplaceVtbl.h"
#include "Logger.h"
#include "GridAdapter.h"
#include "AppParam.h"

extern simplelogger::Logger *logger;
extern AppParam *pAppParam;

static IDXGIFactory1Vtbl vtbl;

using namespace std;

HRESULT STDMETHODCALLTYPE IDXGIFactory1_CreateSwapChain_Proxy( 
	IDXGIFactory1 * This,
	IUnknown *pDevice,
	DXGI_SWAP_CHAIN_DESC *pDesc,
	IDXGISwapChain **ppSwapChain)
{
	LOG_INFO(logger, __FUNCTION__ << ": " << pDesc->BufferDesc.Width << "x" << pDesc->BufferDesc.Height
		<< ", " << (pDesc->Windowed ? "windowed" : "fullscreen"));

	pDesc->Windowed = TRUE;

	LOG_DEBUG(logger, __FUNCTION__ << ": "
		<< "This=" << This
		<< ", pDevice=" << pDevice
		<< ", *pDesc=[" 
		<< "BufferDesc=[" 
		<< "Width=" << pDesc->BufferDesc.Width
		<< ", Height=" << pDesc->BufferDesc.Height
		<< ", RefreshRate=" << pDesc->BufferDesc.RefreshRate.Numerator << "/" << pDesc->BufferDesc.RefreshRate.Denominator
		<< ", Format=" << pDesc->BufferDesc.Format
		<< ", ScanlineOrdering=" << pDesc->BufferDesc.ScanlineOrdering
		<< ", Scaling=" << pDesc->BufferDesc.Scaling
		<< "]"
		<< ", SampleDesc=[" 
		<< "Width=" << pDesc->SampleDesc.Count
		<< ", Height=" << pDesc->SampleDesc.Quality
		<< "]"
		<< ", BufferUsage=" << pDesc->BufferUsage
		<< ", BufferCount=" << pDesc->BufferCount
		<< ", OutputWindow=" << pDesc->OutputWindow
		<< ", Windowed=" << pDesc->Windowed
		<< ", SwapEffect=" << pDesc->SwapEffect
		<< ", Flags=" << pDesc->Flags
		<< "]");
	HRESULT hr = vtbl.CreateSwapChain(This, pDevice, pDesc, ppSwapChain);
	if (FAILED(hr) || !*ppSwapChain) {
		LOG_WARN(logger, "IDXGIFactory1_CreateSwapChain failed. hr=" << hr);
		return hr;
	}

	BOOL IDXGISwapChain_ReplaceVtbl(IDXGISwapChain *);
	IDXGISwapChain_ReplaceVtbl(reinterpret_cast<IDXGISwapChain *>(*ppSwapChain));

	return hr;
}

HRESULT STDMETHODCALLTYPE IDXGIFactory1_EnumAdapters1_Proxy( 
	IDXGIFactory1 * This,
	UINT Adapter,
	IDXGIAdapter1 **ppAdapter) 
{
	/* Never call IDXGIFactory1::EnumAdapters within this function body, otherwise an endless recursion happens.
	   In its original implementation, IDXGIFactory1::EnumAdapters calls IDXGIFactory1::EnumAdapters1.*/
	LOG_DEBUG(logger, __FUNCTION__ << "Adapter=" << Adapter << ", ppAdapter=" << ppAdapter);

	if (Adapter != 0) {
		*ppAdapter = NULL;
		return DXGI_ERROR_NOT_FOUND;
	}

	return vtbl.EnumAdapters1(
		This,
		GetGridAdapterOrdinal_DXGI(
			pAppParam ? pAppParam->iGpu : -1,
			(IDXGIAdapter1 **)NULL,
			AdapterAccessor_DXGI<IDXGIAdapter1, IDXGIFactory1>(This, vtbl.EnumAdapters1)
		),
		ppAdapter);
}

static void SetProxy(IDXGIFactory1Vtbl *pVtbl) 
{
	pVtbl->CreateSwapChain = IDXGIFactory1_CreateSwapChain_Proxy;
	pVtbl->EnumAdapters1 = IDXGIFactory1_EnumAdapters1_Proxy;
	/* Note: EnumAdapters is not needed to override, because it calls EnumAdapters1() internally.*/
}

BOOL IDXGIFactory1_ReplaceVtbl(IDXGIFactory1 *pFactory) 
{
	return ReplaceVtblEntries<IDXGIFactory1Vtbl>(pFactory);
}
