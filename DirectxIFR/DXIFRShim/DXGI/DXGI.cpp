/*!
 * \brief
 * IDXGIFactory and IDXGIFactory1 interfaces used by DX10 and DX11
 *
 * \file
 *
 * This file defines all of the IDXGIFactory/IDXGIFactory1 interfaces for
 * CreateFactory and CreateFactory1 functions that are overriden.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <dxgi.h>
#include <iostream>
#include "Logger.h"
#include "AppParam.h"
#include "Util.h"

simplelogger::Logger *logger 
	= simplelogger::LoggerFactory::CreateFileLogger("DXGI.shim.log");
AppParamManager appParamManger;
AppParam *pAppParam = appParamManger.GetAppParam();

template<class Factory>
static HRESULT WINAPI CreateDXGIFactory1_Proxy_Template(REFIID riid, Factory **ppFactory, BOOL (*Factory_ReplaceVtbl)(Factory *))
{
	static Factory *pFactory;
	if (pFactory) {
		pFactory->AddRef();
		*ppFactory = pFactory;
		return S_OK;
	}

	HRESULT hr = CreateDXGIFactory1(riid, (void **)ppFactory);
	if (FAILED(hr) || !*ppFactory) {
		LOG_ERROR(logger, "CreateDXGIFactory1 failed, hr=" << hr << ", riid=" << IID2String(riid));
		return hr;
	}
	Factory_ReplaceVtbl(*ppFactory);

	pFactory = *ppFactory;
	pFactory->AddRef();

	return hr;
}

HRESULT WINAPI CreateDXGIFactory1_Proxy(REFIID riid, void **ppFactory)
{
	LOG_DEBUG(logger, __FUNCTION__);

	if (!memcmp(&riid, &__uuidof(IDXGIFactory), sizeof(GUID))) {
		BOOL IDXGIFactory_ReplaceVtbl(IDXGIFactory *);
		return CreateDXGIFactory1_Proxy_Template(riid, (IDXGIFactory **)ppFactory, IDXGIFactory_ReplaceVtbl);
	} else if (!memcmp(&riid, &__uuidof(IDXGIFactory1), sizeof(GUID))) {
		BOOL IDXGIFactory1_ReplaceVtbl(IDXGIFactory1 *);
		return CreateDXGIFactory1_Proxy_Template(riid, (IDXGIFactory1 **)ppFactory, IDXGIFactory1_ReplaceVtbl);
	} else {
		LOG_WARN(logger, "Unrecognized GUID in " << __FUNCTION__);
		return CreateDXGIFactory1(riid, ppFactory);
	}
}

HRESULT WINAPI CreateDXGIFactory_Proxy(REFIID riid, void **ppFactory)
{
	LOG_DEBUG(logger, __FUNCTION__);

	return CreateDXGIFactory1_Proxy(riid, ppFactory);
}

HRESULT WINAPI DXGIGetDebugInterface1_Proxy(UINT Flags, REFIID riid, void **ppOut)
{
	typedef HRESULT (WINAPI *DXGIGetDebugInterface1_Type)(UINT Flags, REFIID riid, void **ppOut);
	HMODULE hDll = GetModuleHandle("_dxgi");
	DXGIGetDebugInterface1_Type f = (DXGIGetDebugInterface1_Type)GetProcAddress(hDll, "DXGIGetDebugInterface1");
	if (!f) {
		*ppOut = NULL;
		return E_NOINTERFACE;
	}
	return f(Flags, riid, ppOut);
}

HRESULT WINAPI CreateDXGIFactory2_Proxy(UINT flags, const IID &riid, void **ppFactory)
{
	typedef HRESULT (WINAPI *CreateDXGIFactory2_Type)(UINT flags, const IID &riid, void **ppFactory);
	HMODULE hDll = GetModuleHandle("_dxgi");
	CreateDXGIFactory2_Type f = (CreateDXGIFactory2_Type)GetProcAddress(hDll, "CreateDXGIFactory2");
	if (!f) {
		*ppFactory = NULL;
		return DXGI_ERROR_INVALID_CALL;
	}
	return f(flags, riid, ppFactory);
}
