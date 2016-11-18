/*!
 * \brief
 * IDXGIFactory1Vtbl interfaces used by DX10 and DX11
 *
 * \file
 *
 * This file defines all of the IDXGIFactory1Vtbl interfaces that used.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma once

#include <dxgi.h>

typedef struct IDXGIFactory1Vtbl
{
	BEGIN_INTERFACE

	HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
		IDXGIFactory1 * This,
		/* [in] */ REFIID riid,
		/* [annotation][iid_is][out] */ 
		__RPC__deref_out  void **ppvObject);

	ULONG ( STDMETHODCALLTYPE *AddRef )( 
		IDXGIFactory1 * This);

	ULONG ( STDMETHODCALLTYPE *Release )( 
		IDXGIFactory1 * This);

	HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
		IDXGIFactory1 * This,
		/* [in] */ REFGUID Name,
		/* [in] */ UINT DataSize,
		/* [in] */ const void *pData);

	HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
		IDXGIFactory1 * This,
		/* [in] */ REFGUID Name,
		/* [in] */ const IUnknown *pUnknown);

	HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
		IDXGIFactory1 * This,
		/* [in] */ REFGUID Name,
		/* [out][in] */ UINT *pDataSize,
		/* [out] */ void *pData);

	HRESULT ( STDMETHODCALLTYPE *GetParent )( 
		IDXGIFactory1 * This,
		/* [in] */ REFIID riid,
		/* [retval][out] */ void **ppParent);

	HRESULT ( STDMETHODCALLTYPE *EnumAdapters )( 
		IDXGIFactory1 * This,
		/* [in] */ UINT Adapter,
		/* [out] */ IDXGIAdapter **ppAdapter);

	HRESULT ( STDMETHODCALLTYPE *MakeWindowAssociation )( 
		IDXGIFactory1 * This,
		HWND WindowHandle,
		UINT Flags);

	HRESULT ( STDMETHODCALLTYPE *GetWindowAssociation )( 
		IDXGIFactory1 * This,
		HWND *pWindowHandle);

	HRESULT ( STDMETHODCALLTYPE *CreateSwapChain )( 
		IDXGIFactory1 * This,
		IUnknown *pDevice,
		DXGI_SWAP_CHAIN_DESC *pDesc,
		IDXGISwapChain **ppSwapChain);

	HRESULT ( STDMETHODCALLTYPE *CreateSoftwareAdapter )( 
		IDXGIFactory1 * This,
		/* [in] */ HMODULE Module,
		/* [out] */ IDXGIAdapter **ppAdapter);

	HRESULT ( STDMETHODCALLTYPE *EnumAdapters1 )( 
		IDXGIFactory1 * This,
		/* [in] */ UINT Adapter,
		/* [out] */ IDXGIAdapter1 **ppAdapter);

	BOOL ( STDMETHODCALLTYPE *IsCurrent )( 
		IDXGIFactory1 * This);

	END_INTERFACE
} IDXGIFactory1Vtbl;
