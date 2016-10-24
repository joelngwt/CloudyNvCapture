/*!
 * \brief
 * IDXGIFactoryVtbl interfaces used by DX10 and DX11
 *
 * \file
 *
 * This file defines all of the IDXGIFactory CreateSwapChain and EnumAdapters 
 * interfaces that are overriden by this source file.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <vector>
#include "IDXGIFactory.h"
#include "ReplaceVtbl.h"
#include "Logger.h"
#include "GridAdapter.h"
#include "AppParam.h"

extern simplelogger::Logger *logger;
extern AppParam *pAppParam;

static IDXGIFactoryVtbl vtbl;

using namespace std;

HRESULT STDMETHODCALLTYPE IDXGIFactory_CreateSwapChain_Proxy( 
	IDXGIFactory * This,
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
		LOG_WARN(logger, "IDXGIFactory_CreateSwapChain failed. hr=" << hr);
		return hr;
	}

	BOOL IDXGISwapChain_ReplaceVtbl(IDXGISwapChain *);
	IDXGISwapChain_ReplaceVtbl(reinterpret_cast<IDXGISwapChain *>(*ppSwapChain));

	return hr;
}

HRESULT STDMETHODCALLTYPE IDXGIFactory_EnumAdapters_Proxy( 
	IDXGIFactory * This,
	UINT Adapter,
	IDXGIAdapter **ppAdapter) 
{
	LOG_DEBUG(logger, __FUNCTION__);

	if (Adapter != 0) {
		*ppAdapter = NULL;
		return DXGI_ERROR_NOT_FOUND;
	}

	return vtbl.EnumAdapters(
		This, 
		GetGridAdapterOrdinal_DXGI(
			pAppParam ? pAppParam->iGpu : -1, 
			(IDXGIAdapter **)NULL, 
			AdapterAccessor_DXGI<IDXGIAdapter, IDXGIFactory>(This, vtbl.EnumAdapters)
		), 
		ppAdapter);
}

static void SetProxy(IDXGIFactoryVtbl *pVtbl) 
{
	pVtbl->CreateSwapChain = IDXGIFactory_CreateSwapChain_Proxy;
	pVtbl->EnumAdapters = IDXGIFactory_EnumAdapters_Proxy;	
}

BOOL IDXGIFactory_ReplaceVtbl(IDXGIFactory *pFactory) 
{
	return ReplaceVtblEntries<IDXGIFactoryVtbl>(pFactory);
}
