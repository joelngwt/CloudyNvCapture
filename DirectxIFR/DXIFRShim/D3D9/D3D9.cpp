/*!
 * \brief
 * Direct3DCreate9 and Direct3DCreate9Ex interfaces used by D3D9 and D3D9Ex
 *
 * \file
 *
 * This file defines all of the Direct3DCreate9/Direct3DCreate9Ex interfaces.
 *
 * \copyright
 * Copyright 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <d3d9.h>
#include "IDirect3D9.h"
#include "ReplaceVtbl.h"
#include "Logger.h"
#include "AppParam.h"

simplelogger::Logger *logger 
	= simplelogger::LoggerFactory::CreateFileLogger("D3D9.shim.log");
AppParamManager appParamManger;
AppParam *pAppParam = appParamManger.GetAppParam();

IDirect3D9 * WINAPI Direct3DCreate9_Proxy(UINT SDKVersion)
{
	LOG_DEBUG(logger, __FUNCTION__);
	IDirect3D9 * pDirect3D9 = Direct3DCreate9(SDKVersion);
	if (!pDirect3D9) {
		return NULL;
	}

	BOOL IDirect3D9_ReplaceVtbl(IDirect3D9 *pDirect3D9);
	IDirect3D9_ReplaceVtbl(pDirect3D9);

	LOG_DEBUG(logger, "pDirect3D9=0x" << pDirect3D9);
	return pDirect3D9;
}

HRESULT WINAPI Direct3DCreate9Ex_Proxy(UINT SDKVersion, IDirect3D9Ex **ppD3D)
{
	LOG_DEBUG(logger, __FUNCTION__);
	HRESULT hr = Direct3DCreate9Ex(SDKVersion, ppD3D);
	if (FAILED(hr) || !*ppD3D) {
		return hr;
	}

	BOOL IDirect3D9Ex_ReplaceVtbl(IDirect3D9Ex *pDirect3D9);
	IDirect3D9Ex_ReplaceVtbl(*ppD3D);

	return hr;
}
