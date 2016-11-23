/*
pVtbl->QueryInterface = IDirect3DDevice9Ex_QueryInterface_LogProxy;
pVtbl->AddRef = IDirect3DDevice9Ex_AddRef_LogProxy;
pVtbl->Release = IDirect3DDevice9Ex_Release_LogProxy;
pVtbl->TestCooperativeLevel = IDirect3DDevice9Ex_TestCooperativeLevel_LogProxy;
pVtbl->GetAvailableTextureMem = IDirect3DDevice9Ex_GetAvailableTextureMem_LogProxy;
pVtbl->EvictManagedResources = IDirect3DDevice9Ex_EvictManagedResources_LogProxy;
pVtbl->GetDirect3D = IDirect3DDevice9Ex_GetDirect3D_LogProxy;
pVtbl->GetDeviceCaps = IDirect3DDevice9Ex_GetDeviceCaps_LogProxy;
pVtbl->GetDisplayMode = IDirect3DDevice9Ex_GetDisplayMode_LogProxy;
pVtbl->GetCreationParameters = IDirect3DDevice9Ex_GetCreationParameters_LogProxy;
pVtbl->SetCursorProperties = IDirect3DDevice9Ex_SetCursorProperties_LogProxy;
pVtbl->SetCursorPosition = IDirect3DDevice9Ex_SetCursorPosition_LogProxy;
pVtbl->ShowCursor = IDirect3DDevice9Ex_ShowCursor_LogProxy;
pVtbl->CreateAdditionalSwapChain = IDirect3DDevice9Ex_CreateAdditionalSwapChain_LogProxy;
pVtbl->GetSwapChain = IDirect3DDevice9Ex_GetSwapChain_LogProxy;
pVtbl->GetNumberOfSwapChains = IDirect3DDevice9Ex_GetNumberOfSwapChains_LogProxy;
pVtbl->Reset = IDirect3DDevice9Ex_Reset_LogProxy;
pVtbl->Present = IDirect3DDevice9Ex_Present_LogProxy;
pVtbl->GetBackBuffer = IDirect3DDevice9Ex_GetBackBuffer_LogProxy;
pVtbl->GetRasterStatus = IDirect3DDevice9Ex_GetRasterStatus_LogProxy;
pVtbl->SetDialogBoxMode = IDirect3DDevice9Ex_SetDialogBoxMode_LogProxy;
pVtbl->SetGammaRamp = IDirect3DDevice9Ex_SetGammaRamp_LogProxy;
pVtbl->GetGammaRamp = IDirect3DDevice9Ex_GetGammaRamp_LogProxy;
pVtbl->CreateTexture = IDirect3DDevice9Ex_CreateTexture_LogProxy;
pVtbl->CreateVolumeTexture = IDirect3DDevice9Ex_CreateVolumeTexture_LogProxy;
pVtbl->CreateCubeTexture = IDirect3DDevice9Ex_CreateCubeTexture_LogProxy;
pVtbl->CreateVertexBuffer = IDirect3DDevice9Ex_CreateVertexBuffer_LogProxy;
pVtbl->CreateIndexBuffer = IDirect3DDevice9Ex_CreateIndexBuffer_LogProxy;
pVtbl->CreateRenderTarget = IDirect3DDevice9Ex_CreateRenderTarget_LogProxy;
pVtbl->CreateDepthStencilSurface = IDirect3DDevice9Ex_CreateDepthStencilSurface_LogProxy;
pVtbl->UpdateSurface = IDirect3DDevice9Ex_UpdateSurface_LogProxy;
pVtbl->UpdateTexture = IDirect3DDevice9Ex_UpdateTexture_LogProxy;
pVtbl->GetRenderTargetData = IDirect3DDevice9Ex_GetRenderTargetData_LogProxy;
pVtbl->GetFrontBufferData = IDirect3DDevice9Ex_GetFrontBufferData_LogProxy;
pVtbl->StretchRect = IDirect3DDevice9Ex_StretchRect_LogProxy;
pVtbl->ColorFill = IDirect3DDevice9Ex_ColorFill_LogProxy;
pVtbl->CreateOffscreenPlainSurface = IDirect3DDevice9Ex_CreateOffscreenPlainSurface_LogProxy;
pVtbl->SetRenderTarget = IDirect3DDevice9Ex_SetRenderTarget_LogProxy;
pVtbl->GetRenderTarget = IDirect3DDevice9Ex_GetRenderTarget_LogProxy;
pVtbl->SetDepthStencilSurface = IDirect3DDevice9Ex_SetDepthStencilSurface_LogProxy;
pVtbl->GetDepthStencilSurface = IDirect3DDevice9Ex_GetDepthStencilSurface_LogProxy;
pVtbl->BeginScene = IDirect3DDevice9Ex_BeginScene_LogProxy;
pVtbl->EndScene = IDirect3DDevice9Ex_EndScene_LogProxy;
pVtbl->Clear = IDirect3DDevice9Ex_Clear_LogProxy;
pVtbl->SetTransform = IDirect3DDevice9Ex_SetTransform_LogProxy;
pVtbl->GetTransform = IDirect3DDevice9Ex_GetTransform_LogProxy;
pVtbl->MultiplyTransform = IDirect3DDevice9Ex_MultiplyTransform_LogProxy;
pVtbl->SetViewport = IDirect3DDevice9Ex_SetViewport_LogProxy;
pVtbl->GetViewport = IDirect3DDevice9Ex_GetViewport_LogProxy;
pVtbl->SetMaterial = IDirect3DDevice9Ex_SetMaterial_LogProxy;
pVtbl->GetMaterial = IDirect3DDevice9Ex_GetMaterial_LogProxy;
pVtbl->SetLight = IDirect3DDevice9Ex_SetLight_LogProxy;
pVtbl->GetLight = IDirect3DDevice9Ex_GetLight_LogProxy;
pVtbl->LightEnable = IDirect3DDevice9Ex_LightEnable_LogProxy;
pVtbl->GetLightEnable = IDirect3DDevice9Ex_GetLightEnable_LogProxy;
pVtbl->SetClipPlane = IDirect3DDevice9Ex_SetClipPlane_LogProxy;
pVtbl->GetClipPlane = IDirect3DDevice9Ex_GetClipPlane_LogProxy;
pVtbl->SetRenderState = IDirect3DDevice9Ex_SetRenderState_LogProxy;
pVtbl->GetRenderState = IDirect3DDevice9Ex_GetRenderState_LogProxy;
pVtbl->CreateStateBlock = IDirect3DDevice9Ex_CreateStateBlock_LogProxy;
pVtbl->BeginStateBlock = IDirect3DDevice9Ex_BeginStateBlock_LogProxy;
pVtbl->EndStateBlock = IDirect3DDevice9Ex_EndStateBlock_LogProxy;
pVtbl->SetClipStatus = IDirect3DDevice9Ex_SetClipStatus_LogProxy;
pVtbl->GetClipStatus = IDirect3DDevice9Ex_GetClipStatus_LogProxy;
pVtbl->GetTexture = IDirect3DDevice9Ex_GetTexture_LogProxy;
pVtbl->SetTexture = IDirect3DDevice9Ex_SetTexture_LogProxy;
pVtbl->GetTextureStageState = IDirect3DDevice9Ex_GetTextureStageState_LogProxy;
pVtbl->SetTextureStageState = IDirect3DDevice9Ex_SetTextureStageState_LogProxy;
pVtbl->GetSamplerState = IDirect3DDevice9Ex_GetSamplerState_LogProxy;
pVtbl->SetSamplerState = IDirect3DDevice9Ex_SetSamplerState_LogProxy;
pVtbl->ValidateDevice = IDirect3DDevice9Ex_ValidateDevice_LogProxy;
pVtbl->SetPaletteEntries = IDirect3DDevice9Ex_SetPaletteEntries_LogProxy;
pVtbl->GetPaletteEntries = IDirect3DDevice9Ex_GetPaletteEntries_LogProxy;
pVtbl->SetCurrentTexturePalette = IDirect3DDevice9Ex_SetCurrentTexturePalette_LogProxy;
pVtbl->GetCurrentTexturePalette = IDirect3DDevice9Ex_GetCurrentTexturePalette_LogProxy;
pVtbl->SetScissorRect = IDirect3DDevice9Ex_SetScissorRect_LogProxy;
pVtbl->GetScissorRect = IDirect3DDevice9Ex_GetScissorRect_LogProxy;
pVtbl->SetSoftwareVertexProcessing = IDirect3DDevice9Ex_SetSoftwareVertexProcessing_LogProxy;
pVtbl->GetSoftwareVertexProcessing = IDirect3DDevice9Ex_GetSoftwareVertexProcessing_LogProxy;
pVtbl->SetNPatchMode = IDirect3DDevice9Ex_SetNPatchMode_LogProxy;
pVtbl->GetNPatchMode = IDirect3DDevice9Ex_GetNPatchMode_LogProxy;
pVtbl->DrawPrimitive = IDirect3DDevice9Ex_DrawPrimitive_LogProxy;
pVtbl->DrawIndexedPrimitive = IDirect3DDevice9Ex_DrawIndexedPrimitive_LogProxy;
pVtbl->DrawPrimitiveUP = IDirect3DDevice9Ex_DrawPrimitiveUP_LogProxy;
pVtbl->DrawIndexedPrimitiveUP = IDirect3DDevice9Ex_DrawIndexedPrimitiveUP_LogProxy;
pVtbl->ProcessVertices = IDirect3DDevice9Ex_ProcessVertices_LogProxy;
pVtbl->CreateVertexDeclaration = IDirect3DDevice9Ex_CreateVertexDeclaration_LogProxy;
pVtbl->SetVertexDeclaration = IDirect3DDevice9Ex_SetVertexDeclaration_LogProxy;
pVtbl->GetVertexDeclaration = IDirect3DDevice9Ex_GetVertexDeclaration_LogProxy;
pVtbl->SetFVF = IDirect3DDevice9Ex_SetFVF_LogProxy;
pVtbl->GetFVF = IDirect3DDevice9Ex_GetFVF_LogProxy;
pVtbl->CreateVertexShader = IDirect3DDevice9Ex_CreateVertexShader_LogProxy;
pVtbl->SetVertexShader = IDirect3DDevice9Ex_SetVertexShader_LogProxy;
pVtbl->GetVertexShader = IDirect3DDevice9Ex_GetVertexShader_LogProxy;
pVtbl->SetVertexShaderConstantF = IDirect3DDevice9Ex_SetVertexShaderConstantF_LogProxy;
pVtbl->GetVertexShaderConstantF = IDirect3DDevice9Ex_GetVertexShaderConstantF_LogProxy;
pVtbl->SetVertexShaderConstantI = IDirect3DDevice9Ex_SetVertexShaderConstantI_LogProxy;
pVtbl->GetVertexShaderConstantI = IDirect3DDevice9Ex_GetVertexShaderConstantI_LogProxy;
pVtbl->SetVertexShaderConstantB = IDirect3DDevice9Ex_SetVertexShaderConstantB_LogProxy;
pVtbl->GetVertexShaderConstantB = IDirect3DDevice9Ex_GetVertexShaderConstantB_LogProxy;
pVtbl->SetStreamSource = IDirect3DDevice9Ex_SetStreamSource_LogProxy;
pVtbl->GetStreamSource = IDirect3DDevice9Ex_GetStreamSource_LogProxy;
pVtbl->SetStreamSourceFreq = IDirect3DDevice9Ex_SetStreamSourceFreq_LogProxy;
pVtbl->GetStreamSourceFreq = IDirect3DDevice9Ex_GetStreamSourceFreq_LogProxy;
pVtbl->SetIndices = IDirect3DDevice9Ex_SetIndices_LogProxy;
pVtbl->GetIndices = IDirect3DDevice9Ex_GetIndices_LogProxy;
pVtbl->CreatePixelShader = IDirect3DDevice9Ex_CreatePixelShader_LogProxy;
pVtbl->SetPixelShader = IDirect3DDevice9Ex_SetPixelShader_LogProxy;
pVtbl->GetPixelShader = IDirect3DDevice9Ex_GetPixelShader_LogProxy;
pVtbl->SetPixelShaderConstantF = IDirect3DDevice9Ex_SetPixelShaderConstantF_LogProxy;
pVtbl->GetPixelShaderConstantF = IDirect3DDevice9Ex_GetPixelShaderConstantF_LogProxy;
pVtbl->SetPixelShaderConstantI = IDirect3DDevice9Ex_SetPixelShaderConstantI_LogProxy;
pVtbl->GetPixelShaderConstantI = IDirect3DDevice9Ex_GetPixelShaderConstantI_LogProxy;
pVtbl->SetPixelShaderConstantB = IDirect3DDevice9Ex_SetPixelShaderConstantB_LogProxy;
pVtbl->GetPixelShaderConstantB = IDirect3DDevice9Ex_GetPixelShaderConstantB_LogProxy;
pVtbl->DrawRectPatch = IDirect3DDevice9Ex_DrawRectPatch_LogProxy;
pVtbl->DrawTriPatch = IDirect3DDevice9Ex_DrawTriPatch_LogProxy;
pVtbl->DeletePatch = IDirect3DDevice9Ex_DeletePatch_LogProxy;
pVtbl->CreateQuery = IDirect3DDevice9Ex_CreateQuery_LogProxy;
pVtbl->SetConvolutionMonoKernel = IDirect3DDevice9Ex_SetConvolutionMonoKernel_LogProxy;
pVtbl->ComposeRects = IDirect3DDevice9Ex_ComposeRects_LogProxy;
pVtbl->PresentEx = IDirect3DDevice9Ex_PresentEx_LogProxy;
pVtbl->GetGPUThreadPriority = IDirect3DDevice9Ex_GetGPUThreadPriority_LogProxy;
pVtbl->SetGPUThreadPriority = IDirect3DDevice9Ex_SetGPUThreadPriority_LogProxy;
pVtbl->WaitForVBlank = IDirect3DDevice9Ex_WaitForVBlank_LogProxy;
pVtbl->CheckResourceResidency = IDirect3DDevice9Ex_CheckResourceResidency_LogProxy;
pVtbl->SetMaximumFrameLatency = IDirect3DDevice9Ex_SetMaximumFrameLatency_LogProxy;
pVtbl->GetMaximumFrameLatency = IDirect3DDevice9Ex_GetMaximumFrameLatency_LogProxy;
pVtbl->CheckDeviceState = IDirect3DDevice9Ex_CheckDeviceState_LogProxy;
pVtbl->CreateRenderTargetEx = IDirect3DDevice9Ex_CreateRenderTargetEx_LogProxy;
pVtbl->CreateOffscreenPlainSurfaceEx = IDirect3DDevice9Ex_CreateOffscreenPlainSurfaceEx_LogProxy;
pVtbl->CreateDepthStencilSurfaceEx = IDirect3DDevice9Ex_CreateDepthStencilSurfaceEx_LogProxy;
pVtbl->ResetEx = IDirect3DDevice9Ex_ResetEx_LogProxy;
pVtbl->GetDisplayModeEx = IDirect3DDevice9Ex_GetDisplayModeEx_LogProxy;
*/

#define FUNC_HEADER LOG_DEBUG(logger, __FUNCTION__)

HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_QueryInterface_LogProxy(IDirect3DDevice9Ex * This, REFIID riid, void** ppvObj) {
	FUNC_HEADER;

	return vtbl.QueryInterface(This, riid, ppvObj);
}
ULONG STDMETHODCALLTYPE IDirect3DDevice9Ex_AddRef_LogProxy(IDirect3DDevice9Ex * This) {
	FUNC_HEADER;

	return vtbl.AddRef(This);
}
ULONG STDMETHODCALLTYPE IDirect3DDevice9Ex_Release_LogProxy(IDirect3DDevice9Ex * This) {
	FUNC_HEADER;

	return vtbl.Release(This);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_TestCooperativeLevel_LogProxy(IDirect3DDevice9Ex * This) {
	FUNC_HEADER;

	return vtbl.TestCooperativeLevel(This);
}
UINT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetAvailableTextureMem_LogProxy(IDirect3DDevice9Ex * This) {
	FUNC_HEADER;

	return vtbl.GetAvailableTextureMem(This);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_EvictManagedResources_LogProxy(IDirect3DDevice9Ex * This) {
	FUNC_HEADER;

	return vtbl.EvictManagedResources(This);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetDirect3D_LogProxy(IDirect3DDevice9Ex * This, IDirect3D9** ppD3D9) {
	FUNC_HEADER;

	return vtbl.GetDirect3D(This, ppD3D9);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetDeviceCaps_LogProxy(IDirect3DDevice9Ex * This, D3DCAPS9* pCaps) {
	FUNC_HEADER;

	return vtbl.GetDeviceCaps(This, pCaps);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetDisplayMode_LogProxy(IDirect3DDevice9Ex * This, UINT iSwapChain,D3DDISPLAYMODE* pMode) {
	FUNC_HEADER;

	return vtbl.GetDisplayMode(This, iSwapChain, pMode);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetCreationParameters_LogProxy(IDirect3DDevice9Ex * This, D3DDEVICE_CREATION_PARAMETERS *pParameters) {
	FUNC_HEADER;

	return vtbl.GetCreationParameters(This, pParameters);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetCursorProperties_LogProxy(IDirect3DDevice9Ex * This, UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9* pCursorBitmap) {
	FUNC_HEADER;

	return vtbl.SetCursorProperties(This, XHotSpot, YHotSpot, pCursorBitmap);
}
void STDMETHODCALLTYPE IDirect3DDevice9Ex_SetCursorPosition_LogProxy(IDirect3DDevice9Ex * This, int X,int Y,DWORD Flags) {
	FUNC_HEADER;

	vtbl.SetCursorPosition(This, X, Y, Flags);
}
BOOL STDMETHODCALLTYPE IDirect3DDevice9Ex_ShowCursor_LogProxy(IDirect3DDevice9Ex * This, BOOL bShow) {
	FUNC_HEADER;

	return vtbl.ShowCursor(This, bShow);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateAdditionalSwapChain_LogProxy(IDirect3DDevice9Ex * This, D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain) {
	FUNC_HEADER;

	return vtbl.CreateAdditionalSwapChain(This, pPresentationParameters, pSwapChain);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetSwapChain_LogProxy(IDirect3DDevice9Ex * This, UINT iSwapChain,IDirect3DSwapChain9** pSwapChain) {
	FUNC_HEADER;

	return vtbl.GetSwapChain(This, iSwapChain, pSwapChain);
}
UINT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetNumberOfSwapChains_LogProxy(IDirect3DDevice9Ex * This) {
	FUNC_HEADER;

	return vtbl.GetNumberOfSwapChains(This);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_Reset_LogProxy(IDirect3DDevice9Ex * This, D3DPRESENT_PARAMETERS* pPresentationParameters) {
	FUNC_HEADER;

	return vtbl.Reset(This, pPresentationParameters);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_Present_LogProxy(IDirect3DDevice9Ex * This, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) {
	FUNC_HEADER;

	return vtbl.Present(This, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetBackBuffer_LogProxy(IDirect3DDevice9Ex * This, UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer) {
	FUNC_HEADER;

	return vtbl.GetBackBuffer(This, iSwapChain, iBackBuffer, Type, ppBackBuffer);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetRasterStatus_LogProxy(IDirect3DDevice9Ex * This, UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus) {
	FUNC_HEADER;

	return vtbl.GetRasterStatus(This, iSwapChain, pRasterStatus);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetDialogBoxMode_LogProxy(IDirect3DDevice9Ex * This, BOOL bEnableDialogs) {
	FUNC_HEADER;

	return vtbl.SetDialogBoxMode(This, bEnableDialogs);
}
void STDMETHODCALLTYPE IDirect3DDevice9Ex_SetGammaRamp_LogProxy(IDirect3DDevice9Ex * This, UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp) {
	FUNC_HEADER;

	vtbl.SetGammaRamp(This, iSwapChain, Flags, pRamp);
}
void STDMETHODCALLTYPE IDirect3DDevice9Ex_GetGammaRamp_LogProxy(IDirect3DDevice9Ex * This, UINT iSwapChain,D3DGAMMARAMP* pRamp) {
	FUNC_HEADER;

	vtbl.GetGammaRamp(This, iSwapChain, pRamp);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateTexture_LogProxy(IDirect3DDevice9Ex * This, UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle) {
	FUNC_HEADER;

	return vtbl.CreateTexture(This, Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateVolumeTexture_LogProxy(IDirect3DDevice9Ex * This, UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle) {
	FUNC_HEADER;

	return vtbl.CreateVolumeTexture(This, Width, Height, Depth, Levels, Usage, Format, Pool, ppVolumeTexture, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateCubeTexture_LogProxy(IDirect3DDevice9Ex * This, UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle) {
	FUNC_HEADER;

	return vtbl.CreateCubeTexture(This, EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateVertexBuffer_LogProxy(IDirect3DDevice9Ex * This, UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle) {
	FUNC_HEADER;

	return vtbl.CreateVertexBuffer(This, Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateIndexBuffer_LogProxy(IDirect3DDevice9Ex * This, UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle) {
	FUNC_HEADER;

	return vtbl.CreateIndexBuffer(This, Length, Usage, Format, Pool, ppIndexBuffer, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateRenderTarget_LogProxy(IDirect3DDevice9Ex * This, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) {
	FUNC_HEADER;

	return vtbl.CreateRenderTarget(This, Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateDepthStencilSurface_LogProxy(IDirect3DDevice9Ex * This, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) {
	FUNC_HEADER;

	return vtbl.CreateDepthStencilSurface(This, Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_UpdateSurface_LogProxy(IDirect3DDevice9Ex * This, IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPoint) {
	FUNC_HEADER;

	return vtbl.UpdateSurface(This, pSourceSurface, pSourceRect, pDestinationSurface, pDestPoint);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_UpdateTexture_LogProxy(IDirect3DDevice9Ex * This, IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture) {
	FUNC_HEADER;

	return vtbl.UpdateTexture(This, pSourceTexture, pDestinationTexture);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetRenderTargetData_LogProxy(IDirect3DDevice9Ex * This, IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface) {
	FUNC_HEADER;

	return vtbl.GetRenderTargetData(This, pRenderTarget, pDestSurface);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetFrontBufferData_LogProxy(IDirect3DDevice9Ex * This, UINT iSwapChain,IDirect3DSurface9* pDestSurface) {
	FUNC_HEADER;

	return vtbl.GetFrontBufferData(This, iSwapChain, pDestSurface);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_StretchRect_LogProxy(IDirect3DDevice9Ex * This, IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter) {
	FUNC_HEADER;

	return vtbl.StretchRect(This, pSourceSurface, pSourceRect, pDestSurface, pDestRect, Filter);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_ColorFill_LogProxy(IDirect3DDevice9Ex * This, IDirect3DSurface9* pSurface,CONST RECT* pRect,D3DCOLOR color) {
	FUNC_HEADER;

	return vtbl.ColorFill(This, pSurface, pRect, color);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateOffscreenPlainSurface_LogProxy(IDirect3DDevice9Ex * This, UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) {
	FUNC_HEADER;

	return vtbl.CreateOffscreenPlainSurface(This, Width, Height, Format, Pool, ppSurface, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetRenderTarget_LogProxy(IDirect3DDevice9Ex * This, DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget) {
	FUNC_HEADER;

	return vtbl.SetRenderTarget(This, RenderTargetIndex, pRenderTarget);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetRenderTarget_LogProxy(IDirect3DDevice9Ex * This, DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget) {
	FUNC_HEADER;

	return vtbl.GetRenderTarget(This, RenderTargetIndex, ppRenderTarget);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetDepthStencilSurface_LogProxy(IDirect3DDevice9Ex * This, IDirect3DSurface9* pNewZStencil) {
	FUNC_HEADER;

	return vtbl.SetDepthStencilSurface(This, pNewZStencil);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetDepthStencilSurface_LogProxy(IDirect3DDevice9Ex * This, IDirect3DSurface9** ppZStencilSurface) {
	FUNC_HEADER;

	return vtbl.GetDepthStencilSurface(This, ppZStencilSurface);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_BeginScene_LogProxy(IDirect3DDevice9Ex * This) {
	FUNC_HEADER;

	return vtbl.BeginScene(This);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_EndScene_LogProxy(IDirect3DDevice9Ex * This) {
	FUNC_HEADER;

	return vtbl.EndScene(This);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_Clear_LogProxy(IDirect3DDevice9Ex * This, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil) {
	FUNC_HEADER;

	return vtbl.Clear(This, Count, pRects, Flags, Color, Z, Stencil);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetTransform_LogProxy(IDirect3DDevice9Ex * This, D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix) {
	FUNC_HEADER;

	return vtbl.SetTransform(This, State, pMatrix);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetTransform_LogProxy(IDirect3DDevice9Ex * This, D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix) {
	FUNC_HEADER;

	return vtbl.GetTransform(This, State, pMatrix);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_MultiplyTransform_LogProxy(IDirect3DDevice9Ex * This, D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix) {
	FUNC_HEADER;

	return vtbl.MultiplyTransform(This, State, pMatrix);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetViewport_LogProxy(IDirect3DDevice9Ex * This, CONST D3DVIEWPORT9* pViewport) {
	FUNC_HEADER;

	return vtbl.SetViewport(This, pViewport);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetViewport_LogProxy(IDirect3DDevice9Ex * This, D3DVIEWPORT9* pViewport) {
	FUNC_HEADER;

	return vtbl.GetViewport(This, pViewport);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetMaterial_LogProxy(IDirect3DDevice9Ex * This, CONST D3DMATERIAL9* pMaterial) {
	FUNC_HEADER;

	return vtbl.SetMaterial(This, pMaterial);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetMaterial_LogProxy(IDirect3DDevice9Ex * This, D3DMATERIAL9* pMaterial) {
	FUNC_HEADER;

	return vtbl.GetMaterial(This, pMaterial);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetLight_LogProxy(IDirect3DDevice9Ex * This, DWORD Index,CONST D3DLIGHT9* pLight) {
	FUNC_HEADER;

	return vtbl.SetLight(This, Index, pLight);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetLight_LogProxy(IDirect3DDevice9Ex * This, DWORD Index,D3DLIGHT9* pLight) {
	FUNC_HEADER;

	return vtbl.GetLight(This, Index, pLight);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_LightEnable_LogProxy(IDirect3DDevice9Ex * This, DWORD Index,BOOL Enable) {
	FUNC_HEADER;

	return vtbl.LightEnable(This, Index, Enable);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetLightEnable_LogProxy(IDirect3DDevice9Ex * This, DWORD Index,BOOL* pEnable) {
	FUNC_HEADER;

	return vtbl.GetLightEnable(This, Index, pEnable);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetClipPlane_LogProxy(IDirect3DDevice9Ex * This, DWORD Index,CONST float* pPlane) {
	FUNC_HEADER;

	return vtbl.SetClipPlane(This, Index, pPlane);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetClipPlane_LogProxy(IDirect3DDevice9Ex * This, DWORD Index,float* pPlane) {
	FUNC_HEADER;

	return vtbl.GetClipPlane(This, Index, pPlane);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetRenderState_LogProxy(IDirect3DDevice9Ex * This, D3DRENDERSTATETYPE State,DWORD Value) {
	FUNC_HEADER;

	return vtbl.SetRenderState(This, State, Value);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetRenderState_LogProxy(IDirect3DDevice9Ex * This, D3DRENDERSTATETYPE State,DWORD* pValue) {
	FUNC_HEADER;

	return vtbl.GetRenderState(This, State, pValue);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateStateBlock_LogProxy(IDirect3DDevice9Ex * This, D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB) {
	FUNC_HEADER;

	return vtbl.CreateStateBlock(This, Type, ppSB);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_BeginStateBlock_LogProxy(IDirect3DDevice9Ex * This) {
	FUNC_HEADER;

	return vtbl.BeginStateBlock(This);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_EndStateBlock_LogProxy(IDirect3DDevice9Ex * This, IDirect3DStateBlock9** ppSB) {
	FUNC_HEADER;

	return vtbl.EndStateBlock(This, ppSB);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetClipStatus_LogProxy(IDirect3DDevice9Ex * This, CONST D3DCLIPSTATUS9* pClipStatus) {
	FUNC_HEADER;

	return vtbl.SetClipStatus(This, pClipStatus);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetClipStatus_LogProxy(IDirect3DDevice9Ex * This, D3DCLIPSTATUS9* pClipStatus) {
	FUNC_HEADER;

	return vtbl.GetClipStatus(This, pClipStatus);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetTexture_LogProxy(IDirect3DDevice9Ex * This, DWORD Stage,IDirect3DBaseTexture9** ppTexture) {
	FUNC_HEADER;

	return vtbl.GetTexture(This, Stage, ppTexture);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetTexture_LogProxy(IDirect3DDevice9Ex * This, DWORD Stage,IDirect3DBaseTexture9* pTexture) {
	FUNC_HEADER;

	return vtbl.SetTexture(This, Stage, pTexture);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetTextureStageState_LogProxy(IDirect3DDevice9Ex * This, DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue) {
	FUNC_HEADER;

	return vtbl.GetTextureStageState(This, Stage, Type, pValue);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetTextureStageState_LogProxy(IDirect3DDevice9Ex * This, DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value) {
	FUNC_HEADER;

	return vtbl.SetTextureStageState(This, Stage, Type, Value);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetSamplerState_LogProxy(IDirect3DDevice9Ex * This, DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue) {
	FUNC_HEADER;

	return vtbl.GetSamplerState(This, Sampler, Type, pValue);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetSamplerState_LogProxy(IDirect3DDevice9Ex * This, DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value) {
	FUNC_HEADER;

	return vtbl.SetSamplerState(This, Sampler, Type, Value);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_ValidateDevice_LogProxy(IDirect3DDevice9Ex * This, DWORD* pNumPasses) {
	FUNC_HEADER;

	return vtbl.ValidateDevice(This, pNumPasses);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetPaletteEntries_LogProxy(IDirect3DDevice9Ex * This, UINT PaletteNumber,CONST PALETTEENTRY* pEntries) {
	FUNC_HEADER;

	return vtbl.SetPaletteEntries(This, PaletteNumber, pEntries);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetPaletteEntries_LogProxy(IDirect3DDevice9Ex * This, UINT PaletteNumber,PALETTEENTRY* pEntries) {
	FUNC_HEADER;

	return vtbl.GetPaletteEntries(This, PaletteNumber, pEntries);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetCurrentTexturePalette_LogProxy(IDirect3DDevice9Ex * This, UINT PaletteNumber) {
	FUNC_HEADER;

	return vtbl.SetCurrentTexturePalette(This, PaletteNumber);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetCurrentTexturePalette_LogProxy(IDirect3DDevice9Ex * This, UINT *PaletteNumber) {
	FUNC_HEADER;

	return vtbl.GetCurrentTexturePalette(This, PaletteNumber);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetScissorRect_LogProxy(IDirect3DDevice9Ex * This, CONST RECT* pRect) {
	FUNC_HEADER;

	return vtbl.SetScissorRect(This, pRect);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetScissorRect_LogProxy(IDirect3DDevice9Ex * This, RECT* pRect) {
	FUNC_HEADER;

	return vtbl.GetScissorRect(This, pRect);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetSoftwareVertexProcessing_LogProxy(IDirect3DDevice9Ex * This, BOOL bSoftware) {
	FUNC_HEADER;

	return vtbl.SetSoftwareVertexProcessing(This, bSoftware);
}
BOOL STDMETHODCALLTYPE IDirect3DDevice9Ex_GetSoftwareVertexProcessing_LogProxy(IDirect3DDevice9Ex * This) {
	FUNC_HEADER;

	return vtbl.GetSoftwareVertexProcessing(This);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetNPatchMode_LogProxy(IDirect3DDevice9Ex * This, float nSegments) {
	FUNC_HEADER;

	return vtbl.SetNPatchMode(This, nSegments);
}
float STDMETHODCALLTYPE IDirect3DDevice9Ex_GetNPatchMode_LogProxy(IDirect3DDevice9Ex * This) {
	FUNC_HEADER;

	return vtbl.GetNPatchMode(This);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_DrawPrimitive_LogProxy(IDirect3DDevice9Ex * This, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount) {
	FUNC_HEADER;

	return vtbl.DrawPrimitive(This, PrimitiveType, StartVertex, PrimitiveCount);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_DrawIndexedPrimitive_LogProxy(IDirect3DDevice9Ex * This, D3DPRIMITIVETYPE PrimitiveType,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount) {
	FUNC_HEADER;

	return vtbl.DrawIndexedPrimitive(This, PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_DrawPrimitiveUP_LogProxy(IDirect3DDevice9Ex * This, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) {
	FUNC_HEADER;

	return vtbl.DrawPrimitiveUP(This, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_DrawIndexedPrimitiveUP_LogProxy(IDirect3DDevice9Ex * This, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) {
	FUNC_HEADER;

	return vtbl.DrawIndexedPrimitiveUP(This, PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_ProcessVertices_LogProxy(IDirect3DDevice9Ex * This, UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags) {
	FUNC_HEADER;

	return vtbl.ProcessVertices(This, SrcStartIndex, DestIndex, VertexCount, pDestBuffer, pVertexDecl, Flags);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateVertexDeclaration_LogProxy(IDirect3DDevice9Ex * This, CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl) {
	FUNC_HEADER;

	return vtbl.CreateVertexDeclaration(This, pVertexElements, ppDecl);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetVertexDeclaration_LogProxy(IDirect3DDevice9Ex * This, IDirect3DVertexDeclaration9* pDecl) {
	FUNC_HEADER;

	return vtbl.SetVertexDeclaration(This, pDecl);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetVertexDeclaration_LogProxy(IDirect3DDevice9Ex * This, IDirect3DVertexDeclaration9** ppDecl) {
	FUNC_HEADER;

	return vtbl.GetVertexDeclaration(This, ppDecl);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetFVF_LogProxy(IDirect3DDevice9Ex * This, DWORD FVF) {
	FUNC_HEADER;

	return vtbl.SetFVF(This, FVF);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetFVF_LogProxy(IDirect3DDevice9Ex * This, DWORD* pFVF) {
	FUNC_HEADER;

	return vtbl.GetFVF(This, pFVF);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateVertexShader_LogProxy(IDirect3DDevice9Ex * This, CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader) {
	FUNC_HEADER;

	return vtbl.CreateVertexShader(This, pFunction, ppShader);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetVertexShader_LogProxy(IDirect3DDevice9Ex * This, IDirect3DVertexShader9* pShader) {
	FUNC_HEADER;

	return vtbl.SetVertexShader(This, pShader);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetVertexShader_LogProxy(IDirect3DDevice9Ex * This, IDirect3DVertexShader9** ppShader) {
	FUNC_HEADER;

	return vtbl.GetVertexShader(This, ppShader);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetVertexShaderConstantF_LogProxy(IDirect3DDevice9Ex * This, UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount) {
	FUNC_HEADER;

	return vtbl.SetVertexShaderConstantF(This, StartRegister, pConstantData, Vector4fCount);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetVertexShaderConstantF_LogProxy(IDirect3DDevice9Ex * This, UINT StartRegister,float* pConstantData,UINT Vector4fCount) {
	FUNC_HEADER;

	return vtbl.GetVertexShaderConstantF(This, StartRegister, pConstantData, Vector4fCount);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetVertexShaderConstantI_LogProxy(IDirect3DDevice9Ex * This, UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount) {
	FUNC_HEADER;

	return vtbl.SetVertexShaderConstantI(This, StartRegister, pConstantData, Vector4iCount);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetVertexShaderConstantI_LogProxy(IDirect3DDevice9Ex * This, UINT StartRegister,int* pConstantData,UINT Vector4iCount) {
	FUNC_HEADER;

	return vtbl.GetVertexShaderConstantI(This, StartRegister, pConstantData, Vector4iCount);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetVertexShaderConstantB_LogProxy(IDirect3DDevice9Ex * This, UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount) {
	FUNC_HEADER;

	return vtbl.SetVertexShaderConstantB(This, StartRegister, pConstantData, BoolCount);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetVertexShaderConstantB_LogProxy(IDirect3DDevice9Ex * This, UINT StartRegister,BOOL* pConstantData,UINT BoolCount) {
	FUNC_HEADER;

	return vtbl.GetVertexShaderConstantB(This, StartRegister, pConstantData, BoolCount);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetStreamSource_LogProxy(IDirect3DDevice9Ex * This, UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride) {
	FUNC_HEADER;

	return vtbl.SetStreamSource(This, StreamNumber, pStreamData, OffsetInBytes, Stride);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetStreamSource_LogProxy(IDirect3DDevice9Ex * This, UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* pOffsetInBytes,UINT* pStride) {
	FUNC_HEADER;

	return vtbl.GetStreamSource(This, StreamNumber, ppStreamData, pOffsetInBytes, pStride);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetStreamSourceFreq_LogProxy(IDirect3DDevice9Ex * This, UINT StreamNumber,UINT Setting) {
	FUNC_HEADER;

	return vtbl.SetStreamSourceFreq(This, StreamNumber, Setting);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetStreamSourceFreq_LogProxy(IDirect3DDevice9Ex * This, UINT StreamNumber,UINT* pSetting) {
	FUNC_HEADER;

	return vtbl.GetStreamSourceFreq(This, StreamNumber, pSetting);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetIndices_LogProxy(IDirect3DDevice9Ex * This, IDirect3DIndexBuffer9* pIndexData) {
	FUNC_HEADER;

	return vtbl.SetIndices(This, pIndexData);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetIndices_LogProxy(IDirect3DDevice9Ex * This, IDirect3DIndexBuffer9** ppIndexData) {
	FUNC_HEADER;

	return vtbl.GetIndices(This, ppIndexData);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreatePixelShader_LogProxy(IDirect3DDevice9Ex * This, CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader) {
	FUNC_HEADER;

	return vtbl.CreatePixelShader(This, pFunction, ppShader);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetPixelShader_LogProxy(IDirect3DDevice9Ex * This, IDirect3DPixelShader9* pShader) {
	FUNC_HEADER;

	return vtbl.SetPixelShader(This, pShader);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetPixelShader_LogProxy(IDirect3DDevice9Ex * This, IDirect3DPixelShader9** ppShader) {
	FUNC_HEADER;

	return vtbl.GetPixelShader(This, ppShader);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetPixelShaderConstantF_LogProxy(IDirect3DDevice9Ex * This, UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount) {
	FUNC_HEADER;

	return vtbl.SetPixelShaderConstantF(This, StartRegister, pConstantData, Vector4fCount);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetPixelShaderConstantF_LogProxy(IDirect3DDevice9Ex * This, UINT StartRegister,float* pConstantData,UINT Vector4fCount) {
	FUNC_HEADER;

	return vtbl.GetPixelShaderConstantF(This, StartRegister, pConstantData, Vector4fCount);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetPixelShaderConstantI_LogProxy(IDirect3DDevice9Ex * This, UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount) {
	FUNC_HEADER;

	return vtbl.SetPixelShaderConstantI(This, StartRegister, pConstantData, Vector4iCount);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetPixelShaderConstantI_LogProxy(IDirect3DDevice9Ex * This, UINT StartRegister,int* pConstantData,UINT Vector4iCount) {
	FUNC_HEADER;

	return vtbl.GetPixelShaderConstantI(This, StartRegister, pConstantData, Vector4iCount);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetPixelShaderConstantB_LogProxy(IDirect3DDevice9Ex * This, UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount) {
	FUNC_HEADER;

	return vtbl.SetPixelShaderConstantB(This, StartRegister, pConstantData, BoolCount);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetPixelShaderConstantB_LogProxy(IDirect3DDevice9Ex * This, UINT StartRegister,BOOL* pConstantData,UINT BoolCount) {
	FUNC_HEADER;

	return vtbl.GetPixelShaderConstantB(This, StartRegister, pConstantData, BoolCount);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_DrawRectPatch_LogProxy(IDirect3DDevice9Ex * This, UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo) {
	FUNC_HEADER;

	return vtbl.DrawRectPatch(This, Handle, pNumSegs, pRectPatchInfo);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_DrawTriPatch_LogProxy(IDirect3DDevice9Ex * This, UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
	FUNC_HEADER;

	return vtbl.DrawTriPatch(This, Handle, pNumSegs, pTriPatchInfo);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_DeletePatch_LogProxy(IDirect3DDevice9Ex * This, UINT Handle) {
	FUNC_HEADER;

	return vtbl.DeletePatch(This, Handle);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateQuery_LogProxy(IDirect3DDevice9Ex * This, D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery) {
	FUNC_HEADER;

	return vtbl.CreateQuery(This, Type, ppQuery);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetConvolutionMonoKernel_LogProxy(IDirect3DDevice9Ex * This, UINT width,UINT height,float* rows,float* columns) {
	FUNC_HEADER;

	return vtbl.SetConvolutionMonoKernel(This, width, height, rows, columns);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_ComposeRects_LogProxy(IDirect3DDevice9Ex * This, IDirect3DSurface9* pSrc,IDirect3DSurface9* pDst,IDirect3DVertexBuffer9* pSrcRectDescs,UINT NumRects,IDirect3DVertexBuffer9* pDstRectDescs,D3DCOMPOSERECTSOP Operation,int Xoffset,int Yoffset) {
	FUNC_HEADER;

	return vtbl.ComposeRects(This, pSrc, pDst, pSrcRectDescs, NumRects, pDstRectDescs, Operation, Xoffset, Yoffset);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_PresentEx_LogProxy(IDirect3DDevice9Ex * This, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags) {
	FUNC_HEADER;

	return vtbl.PresentEx(This, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetGPUThreadPriority_LogProxy(IDirect3DDevice9Ex * This, INT* pPriority) {
	FUNC_HEADER;

	return vtbl.GetGPUThreadPriority(This, pPriority);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetGPUThreadPriority_LogProxy(IDirect3DDevice9Ex * This, INT Priority) {
	FUNC_HEADER;

	return vtbl.SetGPUThreadPriority(This, Priority);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_WaitForVBlank_LogProxy(IDirect3DDevice9Ex * This, UINT iSwapChain) {
	FUNC_HEADER;

	return vtbl.WaitForVBlank(This, iSwapChain);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CheckResourceResidency_LogProxy(IDirect3DDevice9Ex * This, IDirect3DResource9** pResourceArray,UINT32 NumResources) {
	FUNC_HEADER;

	return vtbl.CheckResourceResidency(This, pResourceArray, NumResources);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_SetMaximumFrameLatency_LogProxy(IDirect3DDevice9Ex * This, UINT MaxLatency) {
	FUNC_HEADER;

	return vtbl.SetMaximumFrameLatency(This, MaxLatency);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetMaximumFrameLatency_LogProxy(IDirect3DDevice9Ex * This, UINT* pMaxLatency) {
	FUNC_HEADER;

	return vtbl.GetMaximumFrameLatency(This, pMaxLatency);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CheckDeviceState_LogProxy(IDirect3DDevice9Ex * This, HWND hDestinationWindow) {
	FUNC_HEADER;

	return vtbl.CheckDeviceState(This, hDestinationWindow);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateRenderTargetEx_LogProxy(IDirect3DDevice9Ex * This, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle,DWORD Usage) {
	FUNC_HEADER;

	return vtbl.CreateRenderTargetEx(This, Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle, Usage);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateOffscreenPlainSurfaceEx_LogProxy(IDirect3DDevice9Ex * This, UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle,DWORD Usage) {
	FUNC_HEADER;

	return vtbl.CreateOffscreenPlainSurfaceEx(This, Width, Height, Format, Pool, ppSurface, pSharedHandle, Usage);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_CreateDepthStencilSurfaceEx_LogProxy(IDirect3DDevice9Ex * This, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle,DWORD Usage) {
	FUNC_HEADER;

	return vtbl.CreateDepthStencilSurfaceEx(This, Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface, pSharedHandle, Usage);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_ResetEx_LogProxy(IDirect3DDevice9Ex * This, D3DPRESENT_PARAMETERS* pPresentationParameters,D3DDISPLAYMODEEX *pFullscreenDisplayMode) {
	FUNC_HEADER;

	return vtbl.ResetEx(This, pPresentationParameters, pFullscreenDisplayMode);
}
HRESULT STDMETHODCALLTYPE IDirect3DDevice9Ex_GetDisplayModeEx_LogProxy(IDirect3DDevice9Ex * This, UINT iSwapChain,D3DDISPLAYMODEEX* pMode,D3DDISPLAYROTATION* pRotation) {
	FUNC_HEADER;

	return vtbl.GetDisplayModeEx(This, iSwapChain, pMode, pRotation);
}
