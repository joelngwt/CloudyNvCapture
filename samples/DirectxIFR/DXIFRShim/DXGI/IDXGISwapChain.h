/*!
 * \brief
 * IDXGISwapChainVtbl interfaces used by DX10 and DX11
 *
 * \file
 *
 * This file defines all of the IDXGISwapChainVtbl interfaces used.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma once

#include <dxgi.h>

typedef struct IDXGISwapChainVtbl
{
    BEGIN_INTERFACE
    
    HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        IDXGISwapChain * This,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ 
        __RPC__deref_out  void **ppvObject);
    
    ULONG ( STDMETHODCALLTYPE *AddRef )( 
        IDXGISwapChain * This);
    
    ULONG ( STDMETHODCALLTYPE *Release )( 
        IDXGISwapChain * This);
    
    HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
        IDXGISwapChain * This,
        /* [in] */ REFGUID Name,
        /* [in] */ UINT DataSize,
        /* [in] */ const void *pData);
    
    HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
        IDXGISwapChain * This,
        /* [in] */ REFGUID Name,
        /* [in] */ const IUnknown *pUnknown);
    
    HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
        IDXGISwapChain * This,
        /* [in] */ REFGUID Name,
        /* [out][in] */ UINT *pDataSize,
        /* [out] */ void *pData);
    
    HRESULT ( STDMETHODCALLTYPE *GetParent )( 
        IDXGISwapChain * This,
        /* [in] */ REFIID riid,
        /* [retval][out] */ void **ppParent);
    
    HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
        IDXGISwapChain * This,
        /* [in] */ REFIID riid,
        /* [retval][out] */ void **ppDevice);
    
    HRESULT ( STDMETHODCALLTYPE *Present )( 
        IDXGISwapChain * This,
        /* [in] */ UINT SyncInterval,
        /* [in] */ UINT Flags);
    
    HRESULT ( STDMETHODCALLTYPE *GetBuffer )( 
        IDXGISwapChain * This,
        /* [in] */ UINT Buffer,
        /* [in] */ REFIID riid,
        /* [out][in] */ void **ppSurface);
    
    HRESULT ( STDMETHODCALLTYPE *SetFullscreenState )( 
        IDXGISwapChain * This,
        /* [in] */ BOOL Fullscreen,
        /* [in] */ IDXGIOutput *pTarget);
    
    HRESULT ( STDMETHODCALLTYPE *GetFullscreenState )( 
        IDXGISwapChain * This,
        /* [out] */ BOOL *pFullscreen,
        /* [out] */ IDXGIOutput **ppTarget);
    
    HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
        IDXGISwapChain * This,
        /* [out] */ DXGI_SWAP_CHAIN_DESC *pDesc);
    
    HRESULT ( STDMETHODCALLTYPE *ResizeBuffers )( 
        IDXGISwapChain * This,
        /* [in] */ UINT BufferCount,
        /* [in] */ UINT Width,
        /* [in] */ UINT Height,
        /* [in] */ DXGI_FORMAT NewFormat,
        /* [in] */ UINT SwapChainFlags);
    
    HRESULT ( STDMETHODCALLTYPE *ResizeTarget )( 
        IDXGISwapChain * This,
        /* [in] */ const DXGI_MODE_DESC *pNewTargetParameters);
    
    HRESULT ( STDMETHODCALLTYPE *GetContainingOutput )( 
        IDXGISwapChain * This,
        IDXGIOutput **ppOutput);
    
    HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
        IDXGISwapChain * This,
        /* [out] */ DXGI_FRAME_STATISTICS *pStats);
    
    HRESULT ( STDMETHODCALLTYPE *GetLastPresentCount )( 
        IDXGISwapChain * This,
        /* [out] */ UINT *pLastPresentCount);
    
    END_INTERFACE
} IDXGISwapChainVtbl;