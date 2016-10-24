/* \brief
 * IDirect3DDevice9Vtbl interfaces used by D3D9
 *
 * \file
 *
 * This file defines vtable structure of IDirect3DDevice9.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <d3d9.h>

typedef struct IDirect3DDevice9Vtbl
{
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(IDirect3DDevice9 * This, REFIID riid, void** ppvObj);
	ULONG (STDMETHODCALLTYPE *AddRef)(IDirect3DDevice9 * This);
	ULONG (STDMETHODCALLTYPE *Release)(IDirect3DDevice9 * This);
	HRESULT (STDMETHODCALLTYPE *TestCooperativeLevel)(IDirect3DDevice9 * This);
	UINT (STDMETHODCALLTYPE *GetAvailableTextureMem)(IDirect3DDevice9 * This);
	HRESULT (STDMETHODCALLTYPE *EvictManagedResources)(IDirect3DDevice9 * This);
	HRESULT (STDMETHODCALLTYPE *GetDirect3D)(IDirect3DDevice9 * This, IDirect3D9** ppD3D9);
	HRESULT (STDMETHODCALLTYPE *GetDeviceCaps)(IDirect3DDevice9 * This, D3DCAPS9* pCaps);
	HRESULT (STDMETHODCALLTYPE *GetDisplayMode)(IDirect3DDevice9 * This, UINT iSwapChain,D3DDISPLAYMODE* pMode);
	HRESULT (STDMETHODCALLTYPE *GetCreationParameters)(IDirect3DDevice9 * This, D3DDEVICE_CREATION_PARAMETERS *pParameters);
	HRESULT (STDMETHODCALLTYPE *SetCursorProperties)(IDirect3DDevice9 * This, UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9* pCursorBitmap);
	void (STDMETHODCALLTYPE *SetCursorPosition)(IDirect3DDevice9 * This, int X,int Y,DWORD Flags);
	BOOL (STDMETHODCALLTYPE *ShowCursor)(IDirect3DDevice9 * This, BOOL bShow);
	HRESULT (STDMETHODCALLTYPE *CreateAdditionalSwapChain)(IDirect3DDevice9 * This, D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain);
	HRESULT (STDMETHODCALLTYPE *GetSwapChain)(IDirect3DDevice9 * This, UINT iSwapChain,IDirect3DSwapChain9** pSwapChain);
	UINT (STDMETHODCALLTYPE *GetNumberOfSwapChains)(IDirect3DDevice9 * This);
	HRESULT (STDMETHODCALLTYPE *Reset)(IDirect3DDevice9 * This, D3DPRESENT_PARAMETERS* pPresentationParameters);
	HRESULT (STDMETHODCALLTYPE *Present)(IDirect3DDevice9 * This, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);
	HRESULT (STDMETHODCALLTYPE *GetBackBuffer)(IDirect3DDevice9 * This, UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer);
	HRESULT (STDMETHODCALLTYPE *GetRasterStatus)(IDirect3DDevice9 * This, UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus);
	HRESULT (STDMETHODCALLTYPE *SetDialogBoxMode)(IDirect3DDevice9 * This, BOOL bEnableDialogs);
	void (STDMETHODCALLTYPE *SetGammaRamp)(IDirect3DDevice9 * This, UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp);
	void (STDMETHODCALLTYPE *GetGammaRamp)(IDirect3DDevice9 * This, UINT iSwapChain,D3DGAMMARAMP* pRamp);
	HRESULT (STDMETHODCALLTYPE *CreateTexture)(IDirect3DDevice9 * This, UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle);
	HRESULT (STDMETHODCALLTYPE *CreateVolumeTexture)(IDirect3DDevice9 * This, UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle);
	HRESULT (STDMETHODCALLTYPE *CreateCubeTexture)(IDirect3DDevice9 * This, UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle);
	HRESULT (STDMETHODCALLTYPE *CreateVertexBuffer)(IDirect3DDevice9 * This, UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle);
	HRESULT (STDMETHODCALLTYPE *CreateIndexBuffer)(IDirect3DDevice9 * This, UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle);
	HRESULT (STDMETHODCALLTYPE *CreateRenderTarget)(IDirect3DDevice9 * This, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle);
	HRESULT (STDMETHODCALLTYPE *CreateDepthStencilSurface)(IDirect3DDevice9 * This, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle);
	HRESULT (STDMETHODCALLTYPE *UpdateSurface)(IDirect3DDevice9 * This, IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPoint);
	HRESULT (STDMETHODCALLTYPE *UpdateTexture)(IDirect3DDevice9 * This, IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture);
	HRESULT (STDMETHODCALLTYPE *GetRenderTargetData)(IDirect3DDevice9 * This, IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface);
	HRESULT (STDMETHODCALLTYPE *GetFrontBufferData)(IDirect3DDevice9 * This, UINT iSwapChain,IDirect3DSurface9* pDestSurface);
	HRESULT (STDMETHODCALLTYPE *StretchRect)(IDirect3DDevice9 * This, IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter);
	HRESULT (STDMETHODCALLTYPE *ColorFill)(IDirect3DDevice9 * This, IDirect3DSurface9* pSurface,CONST RECT* pRect,D3DCOLOR color);
	HRESULT (STDMETHODCALLTYPE *CreateOffscreenPlainSurface)(IDirect3DDevice9 * This, UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle);
	HRESULT (STDMETHODCALLTYPE *SetRenderTarget)(IDirect3DDevice9 * This, DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget);
	HRESULT (STDMETHODCALLTYPE *GetRenderTarget)(IDirect3DDevice9 * This, DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget);
	HRESULT (STDMETHODCALLTYPE *SetDepthStencilSurface)(IDirect3DDevice9 * This, IDirect3DSurface9* pNewZStencil);
	HRESULT (STDMETHODCALLTYPE *GetDepthStencilSurface)(IDirect3DDevice9 * This, IDirect3DSurface9** ppZStencilSurface);
	HRESULT (STDMETHODCALLTYPE *BeginScene)(IDirect3DDevice9 * This);
	HRESULT (STDMETHODCALLTYPE *EndScene)(IDirect3DDevice9 * This);
	HRESULT (STDMETHODCALLTYPE *Clear)(IDirect3DDevice9 * This, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil);
	HRESULT (STDMETHODCALLTYPE *SetTransform)(IDirect3DDevice9 * This, D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix);
	HRESULT (STDMETHODCALLTYPE *GetTransform)(IDirect3DDevice9 * This, D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix);
	HRESULT (STDMETHODCALLTYPE *MultiplyTransform)(IDirect3DDevice9 * This, D3DTRANSFORMSTATETYPE,CONST D3DMATRIX*);
	HRESULT (STDMETHODCALLTYPE *SetViewport)(IDirect3DDevice9 * This, CONST D3DVIEWPORT9* pViewport);
	HRESULT (STDMETHODCALLTYPE *GetViewport)(IDirect3DDevice9 * This, D3DVIEWPORT9* pViewport);
	HRESULT (STDMETHODCALLTYPE *SetMaterial)(IDirect3DDevice9 * This, CONST D3DMATERIAL9* pMaterial);
	HRESULT (STDMETHODCALLTYPE *GetMaterial)(IDirect3DDevice9 * This, D3DMATERIAL9* pMaterial);
	HRESULT (STDMETHODCALLTYPE *SetLight)(IDirect3DDevice9 * This, DWORD Index,CONST D3DLIGHT9*);
	HRESULT (STDMETHODCALLTYPE *GetLight)(IDirect3DDevice9 * This, DWORD Index,D3DLIGHT9*);
	HRESULT (STDMETHODCALLTYPE *LightEnable)(IDirect3DDevice9 * This, DWORD Index,BOOL Enable);
	HRESULT (STDMETHODCALLTYPE *GetLightEnable)(IDirect3DDevice9 * This, DWORD Index,BOOL* pEnable);
	HRESULT (STDMETHODCALLTYPE *SetClipPlane)(IDirect3DDevice9 * This, DWORD Index,CONST float* pPlane);
	HRESULT (STDMETHODCALLTYPE *GetClipPlane)(IDirect3DDevice9 * This, DWORD Index,float* pPlane);
	HRESULT (STDMETHODCALLTYPE *SetRenderState)(IDirect3DDevice9 * This, D3DRENDERSTATETYPE State,DWORD Value);
	HRESULT (STDMETHODCALLTYPE *GetRenderState)(IDirect3DDevice9 * This, D3DRENDERSTATETYPE State,DWORD* pValue);
	HRESULT (STDMETHODCALLTYPE *CreateStateBlock)(IDirect3DDevice9 * This, D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB);
	HRESULT (STDMETHODCALLTYPE *BeginStateBlock)(IDirect3DDevice9 * This);
	HRESULT (STDMETHODCALLTYPE *EndStateBlock)(IDirect3DDevice9 * This, IDirect3DStateBlock9** ppSB);
	HRESULT (STDMETHODCALLTYPE *SetClipStatus)(IDirect3DDevice9 * This, CONST D3DCLIPSTATUS9* pClipStatus);
	HRESULT (STDMETHODCALLTYPE *GetClipStatus)(IDirect3DDevice9 * This, D3DCLIPSTATUS9* pClipStatus);
	HRESULT (STDMETHODCALLTYPE *GetTexture)(IDirect3DDevice9 * This, DWORD Stage,IDirect3DBaseTexture9** ppTexture);
	HRESULT (STDMETHODCALLTYPE *SetTexture)(IDirect3DDevice9 * This, DWORD Stage,IDirect3DBaseTexture9* pTexture);
	HRESULT (STDMETHODCALLTYPE *GetTextureStageState)(IDirect3DDevice9 * This, DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue);
	HRESULT (STDMETHODCALLTYPE *SetTextureStageState)(IDirect3DDevice9 * This, DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value);
	HRESULT (STDMETHODCALLTYPE *GetSamplerState)(IDirect3DDevice9 * This, DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue);
	HRESULT (STDMETHODCALLTYPE *SetSamplerState)(IDirect3DDevice9 * This, DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value);
	HRESULT (STDMETHODCALLTYPE *ValidateDevice)(IDirect3DDevice9 * This, DWORD* pNumPasses);
	HRESULT (STDMETHODCALLTYPE *SetPaletteEntries)(IDirect3DDevice9 * This, UINT PaletteNumber,CONST PALETTEENTRY* pEntries);
	HRESULT (STDMETHODCALLTYPE *GetPaletteEntries)(IDirect3DDevice9 * This, UINT PaletteNumber,PALETTEENTRY* pEntries);
	HRESULT (STDMETHODCALLTYPE *SetCurrentTexturePalette)(IDirect3DDevice9 * This, UINT PaletteNumber);
	HRESULT (STDMETHODCALLTYPE *GetCurrentTexturePalette)(IDirect3DDevice9 * This, UINT *PaletteNumber);
	HRESULT (STDMETHODCALLTYPE *SetScissorRect)(IDirect3DDevice9 * This, CONST RECT* pRect);
	HRESULT (STDMETHODCALLTYPE *GetScissorRect)(IDirect3DDevice9 * This, RECT* pRect);
	HRESULT (STDMETHODCALLTYPE *SetSoftwareVertexProcessing)(IDirect3DDevice9 * This, BOOL bSoftware);
	BOOL (STDMETHODCALLTYPE *GetSoftwareVertexProcessing)(IDirect3DDevice9 * This);
	HRESULT (STDMETHODCALLTYPE *SetNPatchMode)(IDirect3DDevice9 * This, float nSegments);
	float (STDMETHODCALLTYPE *GetNPatchMode)(IDirect3DDevice9 * This);
	HRESULT (STDMETHODCALLTYPE *DrawPrimitive)(IDirect3DDevice9 * This, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount);
	HRESULT (STDMETHODCALLTYPE *DrawIndexedPrimitive)(IDirect3DDevice9 * This, D3DPRIMITIVETYPE,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount);
	HRESULT (STDMETHODCALLTYPE *DrawPrimitiveUP)(IDirect3DDevice9 * This, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
	HRESULT (STDMETHODCALLTYPE *DrawIndexedPrimitiveUP)(IDirect3DDevice9 * This, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
	HRESULT (STDMETHODCALLTYPE *ProcessVertices)(IDirect3DDevice9 * This, UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags);
	HRESULT (STDMETHODCALLTYPE *CreateVertexDeclaration)(IDirect3DDevice9 * This, CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl);
	HRESULT (STDMETHODCALLTYPE *SetVertexDeclaration)(IDirect3DDevice9 * This, IDirect3DVertexDeclaration9* pDecl);
	HRESULT (STDMETHODCALLTYPE *GetVertexDeclaration)(IDirect3DDevice9 * This, IDirect3DVertexDeclaration9** ppDecl);
	HRESULT (STDMETHODCALLTYPE *SetFVF)(IDirect3DDevice9 * This, DWORD FVF);
	HRESULT (STDMETHODCALLTYPE *GetFVF)(IDirect3DDevice9 * This, DWORD* pFVF);
	HRESULT (STDMETHODCALLTYPE *CreateVertexShader)(IDirect3DDevice9 * This, CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader);
	HRESULT (STDMETHODCALLTYPE *SetVertexShader)(IDirect3DDevice9 * This, IDirect3DVertexShader9* pShader);
	HRESULT (STDMETHODCALLTYPE *GetVertexShader)(IDirect3DDevice9 * This, IDirect3DVertexShader9** ppShader);
	HRESULT (STDMETHODCALLTYPE *SetVertexShaderConstantF)(IDirect3DDevice9 * This, UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
	HRESULT (STDMETHODCALLTYPE *GetVertexShaderConstantF)(IDirect3DDevice9 * This, UINT StartRegister,float* pConstantData,UINT Vector4fCount);
	HRESULT (STDMETHODCALLTYPE *SetVertexShaderConstantI)(IDirect3DDevice9 * This, UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);
	HRESULT (STDMETHODCALLTYPE *GetVertexShaderConstantI)(IDirect3DDevice9 * This, UINT StartRegister,int* pConstantData,UINT Vector4iCount);
	HRESULT (STDMETHODCALLTYPE *SetVertexShaderConstantB)(IDirect3DDevice9 * This, UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
	HRESULT (STDMETHODCALLTYPE *GetVertexShaderConstantB)(IDirect3DDevice9 * This, UINT StartRegister,BOOL* pConstantData,UINT BoolCount);
	HRESULT (STDMETHODCALLTYPE *SetStreamSource)(IDirect3DDevice9 * This, UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride);
	HRESULT (STDMETHODCALLTYPE *GetStreamSource)(IDirect3DDevice9 * This, UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* pOffsetInBytes,UINT* pStride);
	HRESULT (STDMETHODCALLTYPE *SetStreamSourceFreq)(IDirect3DDevice9 * This, UINT StreamNumber,UINT Setting);
	HRESULT (STDMETHODCALLTYPE *GetStreamSourceFreq)(IDirect3DDevice9 * This, UINT StreamNumber,UINT* pSetting);
	HRESULT (STDMETHODCALLTYPE *SetIndices)(IDirect3DDevice9 * This, IDirect3DIndexBuffer9* pIndexData);
	HRESULT (STDMETHODCALLTYPE *GetIndices)(IDirect3DDevice9 * This, IDirect3DIndexBuffer9** ppIndexData);
	HRESULT (STDMETHODCALLTYPE *CreatePixelShader)(IDirect3DDevice9 * This, CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader);
	HRESULT (STDMETHODCALLTYPE *SetPixelShader)(IDirect3DDevice9 * This, IDirect3DPixelShader9* pShader);
	HRESULT (STDMETHODCALLTYPE *GetPixelShader)(IDirect3DDevice9 * This, IDirect3DPixelShader9** ppShader);
	HRESULT (STDMETHODCALLTYPE *SetPixelShaderConstantF)(IDirect3DDevice9 * This, UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
	HRESULT (STDMETHODCALLTYPE *GetPixelShaderConstantF)(IDirect3DDevice9 * This, UINT StartRegister,float* pConstantData,UINT Vector4fCount);
	HRESULT (STDMETHODCALLTYPE *SetPixelShaderConstantI)(IDirect3DDevice9 * This, UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);
	HRESULT (STDMETHODCALLTYPE *GetPixelShaderConstantI)(IDirect3DDevice9 * This, UINT StartRegister,int* pConstantData,UINT Vector4iCount);
	HRESULT (STDMETHODCALLTYPE *SetPixelShaderConstantB)(IDirect3DDevice9 * This, UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
	HRESULT (STDMETHODCALLTYPE *GetPixelShaderConstantB)(IDirect3DDevice9 * This, UINT StartRegister,BOOL* pConstantData,UINT BoolCount);
	HRESULT (STDMETHODCALLTYPE *DrawRectPatch)(IDirect3DDevice9 * This, UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo);
	HRESULT (STDMETHODCALLTYPE *DrawTriPatch)(IDirect3DDevice9 * This, UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo);
	HRESULT (STDMETHODCALLTYPE *DeletePatch)(IDirect3DDevice9 * This, UINT Handle);
	HRESULT (STDMETHODCALLTYPE *CreateQuery)(IDirect3DDevice9 * This, D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery);
} IDirect3DDevice9Vtbl;
