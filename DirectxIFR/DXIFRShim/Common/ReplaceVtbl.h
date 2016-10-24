/* \brief
 * ReplaceVtblEntries template to replace the Virtual Function Table Entries
 *
 * \file
 *
 * When using ReplaceVtblEntries(), define a static function SetProxy() where proxy
 * function replacement is settled. SetProxy() will be called within ReplaceVtblEntries()
 * and take effect.
 *
 * \copyright
 * Copyright 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma once

#include "Logger.h"

template <class Vtbl>
bool ReplaceVtblEntries(void *pObject) {
	Vtbl *pVtbl = *(Vtbl **)pObject;
	if (!vtbl.QueryInterface) {
		vtbl = *pVtbl;
	}
	
	DWORD dwOldProtectFlag;
	if (!VirtualProtectEx(GetCurrentProcess(), pVtbl, sizeof(Vtbl), PAGE_READWRITE, &dwOldProtectFlag)) {
		return false;
	}
	SetProxy(pVtbl);
	VirtualProtectEx(GetCurrentProcess(), pVtbl, sizeof(Vtbl), dwOldProtectFlag, &dwOldProtectFlag);
	
	return true;
}

template <class Vtbl, class VtblEx, class Interface>
static ULONG STDMETHODCALLTYPE Interface_Release_Proxy(Interface *pObject) 
{
	vtbl.AddRef(pObject);
	if (vtbl.Release(pObject) > 1) {
		return vtbl.Release(pObject);
	}

	VtblEx *pVtblEx = *(VtblEx **)pObject;
	*(Vtbl **)pObject = pVtblEx->pOriginalVtbl;
	delete pVtblEx;
	return pObject->Release();
}

template <class Vtbl, class VtblEx, class Interface>
bool ReplaceVtbl(Interface *pObject) 
{
	Vtbl **ppVtbl = (Vtbl **)pObject;
	if ((*ppVtbl)->Release == Interface_Release_Proxy<Vtbl, VtblEx, Interface>) {
		return true;
	}

	if (!vtbl.QueryInterface) {
		vtbl = **ppVtbl;
	}

	VtblEx *pVtblEx = new VtblEx();
	if (!pVtblEx) {
		return false;
	}

	pVtblEx->pOriginalVtbl = *ppVtbl;
	pVtblEx->vtbl = **ppVtbl;
	SetProxy(&pVtblEx->vtbl);
	*ppVtbl = (Vtbl *)pVtblEx;

	return true;
}

#ifdef UNICODE
#define APPENDAW(NAME) NAME ## W
#else
#define APPENDAW(NAME) NAME ## A
#endif
