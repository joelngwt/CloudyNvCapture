#include <d3d9.h>
#include <d3d11.h>
#include "NvIFREncoderDXGIBase.h"

#pragma comment(lib, "d3d10_1.lib")

BOOL NvIFREncoderDXGIBase::SetupNvIFR()
{
	LOG_DEBUG(logger, __FUNCTION__);

	NvIFREncoder::SetupNvIFR();

	DXGI_SWAP_CHAIN_DESC sc = { 0 };
	sc.BufferCount = 1;
	sc.BufferDesc.Width = nWidth;
	sc.BufferDesc.Height = nHeight;
	sc.BufferDesc.Format = dxgiFormat;
	sc.BufferDesc.RefreshRate.Numerator = 0;
	sc.BufferDesc.RefreshRate.Denominator = 1;
	sc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sc.OutputWindow = hwndEncoder;
	sc.SampleDesc.Count = 1;
	sc.SampleDesc.Quality = 0;
	sc.Windowed = TRUE;

	HRESULT hr = D3D10CreateDeviceAndSwapChain1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
		D3D10_FEATURE_LEVEL_10_1, D3D10_1_SDK_VERSION, &sc, &pSwapChain, &pDevice);
	if (FAILED(hr)) {
		LOG_ERROR(logger, "Unable to create encoding device and swapchain, hr=" << hr);
		return FALSE;
	}

	hr = pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr)) {
		LOG_ERROR(logger, "Unable to get encoding render target.");
		return FALSE;
	}

	//Create a render target view
	hr = pDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView);
	if (FAILED(hr)) {
		LOG_ERROR(logger, "Unable to create encoding render target view.");
		return FALSE;
	}
	pDevice->OMSetRenderTargets(1, &pRenderTargetView, NULL);

	D3D10_TEXTURE2D_DESC td;
	pBackBuffer->GetDesc(&td);
	td.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET;
	td.MiscFlags |= bKeyedMutex ? D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX : D3D10_RESOURCE_MISC_SHARED;
	pDevice->CreateTexture2D(&td, NULL, &pSharedTexture);

	if (!pSharedTexture) {
		LOG_ERROR(logger, "failed to create pSharedTexture");
		return FALSE;
	}

	IDXGIResource *pDxgiResource;
	pSharedTexture->QueryInterface(__uuidof(IDXGIResource), (void **)&pDxgiResource);
	hr = pDxgiResource->GetSharedHandle(&hSharedTexture);
	if (FAILED(hr)) {
		LOG_ERROR(logger, "failed create hSharedTexture");
		pDxgiResource->Release();
		return FALSE;
	}
	pDxgiResource->Release();

	pBackBuffer->GetDesc(&td);
	td.BindFlags = 0;
	td.Usage = D3D10_USAGE_STAGING;
	td.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	pDevice->CreateTexture2D(&td, NULL, &pStagingTexture);

	CreateCommitTexture(pDevice, dxgiFormat, &pCommitTexture);

	NvIFRLibrary NvIFRLib;
	if (!NvIFRLib.load()) {
		LOG_ERROR(logger, "NvIFRLib.load() failed");
		return FALSE;
	}

	pIFR = (INvIFRToHWEncoder_v1 *)NvIFRLib.create(pDevice, NVIFR_TO_HWENCODER); 
	if (!pIFR) {
		LOG_DEBUG(logger, "failed to create NvIFRToH264HWEncoder");
		return FALSE;
	}

	LOG_DEBUG(logger, "succeeded to create NvIFRToH264HWEncoder");
	return TRUE;
}

void NvIFREncoderDXGIBase::CleanupNvIFR()
{
	if (pIFR) {
		pIFR->NvIFRRelease();
	}

	if (pCommitTexture) {
		pCommitTexture->Release();
	}

	if (pStagingTexture) {
		pStagingTexture->Release();
	}

	if (pSharedTexture) {
		pSharedTexture->Release();
	}

	if (pRenderTargetView) {
		pRenderTargetView->Release();
	}

	if (pBackBuffer) {
		pBackBuffer->Release();
	}

	if (pSwapChain) {
		pSwapChain->Release();
	}

	if (pDevice) {
		pDevice->Release();
	}

	NvIFREncoder::CleanupNvIFR();
}

BOOL NvIFREncoderDXGIBase::UpdateBackBuffer()
{
	if (!pBackBuffer || !pSharedTexture) {
		return FALSE;
	}

	if (!bKeyedMutex) {
		CommitUpdate(pDevice, pCommitTexture, pSharedTexture);
		pDevice->CopyResource(pBackBuffer, pSharedTexture);
		CommitUpdate(pDevice, pCommitTexture, pBackBuffer);
	}
	else {
		IDXGIKeyedMutex *pDXGIKeyedMutex;
		if (FAILED(pSharedTexture->QueryInterface(_uuidof(IDXGIKeyedMutex), (void **)&pDXGIKeyedMutex))) {
			LOG_ERROR(logger, "QueryInterface() for IDXGIKeyedMutex failed.");
			return FALSE;
		}
		pDXGIKeyedMutex->AcquireSync(0, INFINITE);
		pDevice->CopyResource(pBackBuffer, pSharedTexture);
		pDXGIKeyedMutex->ReleaseSync(0);
		pDXGIKeyedMutex->Release();
	}

	return TRUE;
}
