/*!
 * \brief
 * IDXGIFactoryVtbl interfaces used by DX10 and DX11
 *
 * \file
 *
 * This file defines all of the IDXGIFactoryVtbl interfaces used.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma once

#include <dxgi.h>

typedef struct IDXGIFactoryVtbl
{
    BEGIN_INTERFACE
    
    HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGIFactory * This,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ 
        __RPC__deref_out  void **ppvObject);
    
    ULONG ( STDMETHODCALLTYPE *AddRef )( 
        IDXGIFactory * This);
    
    ULONG ( STDMETHODCALLTYPE *Release )( 
        IDXGIFactory * This);
    
    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
        IDXGIFactory * This,
        /* [in] */ REFGUID Name,
        /* [in] */ UINT DataSize,
        /* [in] */ const void *pData);
    
    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
        IDXGIFactory * This,
        /* [in] */ REFGUID Name,
        /* [in] */ const IUnknown *pUnknown);
    
    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
        IDXGIFactory * This,
        /* [in] */ REFGUID Name,
        /* [out][in] */ UINT *pDataSize,
        /* [out] */ void *pData);
    
    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
        IDXGIFactory * This,
        /* [in] */ REFIID riid,
        /* [retval][out] */ void **ppParent);
    
    HRESULT ( STDMETHODCALLTYPE *EnumAdapters )( 
        IDXGIFactory * This,
        /* [in] */ UINT Adapter,
        /* [out] */ IDXGIAdapter **ppAdapter);
    
    HRESULT ( STDMETHODCALLTYPE *MakeWindowAssociation )( 
        IDXGIFactory * This,
        HWND WindowHandle,
        UINT Flags);
    
    HRESULT ( STDMETHODCALLTYPE *GetWindowAssociation )( 
        IDXGIFactory * This,
        HWND *pWindowHandle);
    
    HRESULT ( STDMETHODCALLTYPE *CreateSwapChain )( 
        IDXGIFactory * This,
        IUnknown *pDevice,
        DXGI_SWAP_CHAIN_DESC *pDesc,
        IDXGISwapChain **ppSwapChain);
    
    HRESULT ( STDMETHODCALLTYPE *CreateSoftwareAdapter )( 
        IDXGIFactory * This,
        /* [in] */ HMODULE Module,
        /* [out] */ IDXGIAdapter **ppAdapter);
    
    END_INTERFACE
} IDXGIFactoryVtbl;