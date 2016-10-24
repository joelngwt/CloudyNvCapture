/* \brief
 * IDirect3DSwapChain9Vtbl interfaces used by IDirect3DSwapChain9
 *
 * \file
 *
 * This file defines vtable structure of IDirect3DSwapChain9.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <d3d9.h>

typedef struct IDirect3DSwapChain9Vtbl
{
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(IDirect3DSwapChain9 *This, REFIID riid, void** ppvObj);
	ULONG (STDMETHODCALLTYPE *AddRef)(IDirect3DSwapChain9 *This);
	ULONG (STDMETHODCALLTYPE *Release)(IDirect3DSwapChain9 *This);
	HRESULT (STDMETHODCALLTYPE *Present)(IDirect3DSwapChain9 *This, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags);
	HRESULT (STDMETHODCALLTYPE *GetFrontBufferData)(IDirect3DSwapChain9 *This, IDirect3DSurface9* pDestSurface);
	HRESULT (STDMETHODCALLTYPE *GetBackBuffer)(IDirect3DSwapChain9 *This, UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer);
	HRESULT (STDMETHODCALLTYPE *GetRasterStatus)(IDirect3DSwapChain9 *This, D3DRASTER_STATUS* pRasterStatus);
	HRESULT (STDMETHODCALLTYPE *GetDisplayMode)(IDirect3DSwapChain9 *This, D3DDISPLAYMODE* pMode);
	HRESULT (STDMETHODCALLTYPE *GetDevice)(IDirect3DSwapChain9 *This, IDirect3DDevice9** ppDevice);
	HRESULT (STDMETHODCALLTYPE *GetPresentParameters)(IDirect3DSwapChain9 *This, D3DPRESENT_PARAMETERS* pPresentationParameters);
} IDirect3DSwapChain9Vtbl;