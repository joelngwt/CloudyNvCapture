/*!
 * \brief
 * NvIFREncoderD3D9 (derived from NvIFREncoder) is the interface used in 
 * hooked D3D9 API calls.
 *
 * \file
 *
 * The major compoments of this class include:
 * 1. Setup/Cleanup NvIFR interface
 * 2. Update the shared resource
 * 3. Destroy the shared resource
 * 4. Copy shared resource to the back buffer of the encoding device
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include "NvIFREncoderD3D9.h"
#include "Logger.h"

extern simplelogger::Logger *logger;

BOOL NvIFREncoderD3D9::SetupNvIFR() 
{
	NvIFREncoder::SetupNvIFR();

	//Create the D3D object.
	IDirect3D9Ex *pD3D;
	if(FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &pD3D))) {
		LOG_ERROR(logger, "Direct3DCreate9Ex() failed");
		return FALSE;
	}

	//Set up the structure used to create the D3DDevice. 
	D3DPRESENT_PARAMETERS d3dpp = {0};
	d3dpp.Windowed = TRUE;
	d3dpp.BackBufferWidth  = nWidth;
	d3dpp.BackBufferHeight = nHeight;
	d3dpp.BackBufferFormat = d3dFormat;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.EnableAutoDepthStencil = FALSE;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	HRESULT hr;
	if(FAILED(hr = pD3D->CreateDeviceEx(
		GetGridAdapterOrdinal_D3D9(pAppParam ? pAppParam->iGpu : -1, AdapterAccessor_D3D9<IDirect3D9Ex>(pD3D)), 
		D3DDEVTYPE_HAL, hwndEncoder,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, NULL, &pDevice))
	) {
		LOG_ERROR(logger, "pD3D->CreateDeviceEx() failed, hr=" << hr);
		pD3D->Release();
		return FALSE;
	}
	pD3D->Release();

	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

	if (!NvIFRLib.load()) {
		LOG_ERROR(logger, "NvIFRLib.load() failed");
		return FALSE;
	}

	pIFR = (INvIFRToHWEncoder_v1 *)NvIFRLib.create(pDevice, NVIFR_TO_HWENCODER); 
	if(!pIFR) {
		LOG_ERROR(logger, "failed to create NvIFRToH264HWEncoder");
		return FALSE;
	}

	LOG_DEBUG(logger, "succeeded to create NvIFRToH264HWEncoder");
	return TRUE;
}

void NvIFREncoderD3D9::CleanupNvIFR() 
{
	DestroySharedSurface();

	if (pIFR) {
		pIFR->NvIFRRelease();
	}

	if (pDevice) {
		pDevice->Release();
	}
	NvIFREncoder::CleanupNvIFR();
}

BOOL NvIFREncoderD3D9::UpdateBackBuffer() 
{
	BOOL ret = FALSE;

	EnterCriticalSection(&cs_hNvSharedSurface);
	if (hNvSharedSurface) {
		IDirect3DSurface9 *pBackBuffer;
		pDevice->GetRenderTarget(0, &pBackBuffer);
		ret = NvIFRLib.CopyFromSharedSurface(pDevice, hNvSharedSurface, pBackBuffer);
		pBackBuffer->Release();
	}
	LeaveCriticalSection(&cs_hNvSharedSurface);
	
	return ret;
}

BOOL NvIFREncoderD3D9::UpdateSharedSurface(IDirect3DDevice9 * pDeviceR, IDirect3DSurface9 * pBackBufferR) 
{
	if (!pDeviceR || !pBackBufferR) {
		return FALSE;
	}
	if (hNvSharedSurface && pDeviceR != this->pDeviceR) {
		DestroySharedSurface();
	}
	if (!hNvSharedSurface) {
		D3DSURFACE_DESC desc;
		pBackBufferR->GetDesc(&desc);
		if (!NvIFRLib.CreateSharedSurface(pDeviceR, desc.Width, desc.Height, &hNvSharedSurface)) {
			LOG_ERROR(logger, "Something is wrong with the driver. cannot initialize NvIFR surface sharing.");
			return FALSE;
		}
		LOG_DEBUG(logger, "Shared surface is created.");
		this->pDeviceR = pDeviceR;
	}
	return NvIFRLib.CopyToSharedSurface(pDeviceR, hNvSharedSurface, pBackBufferR);
}

void NvIFREncoderD3D9::DestroySharedSurface() 
{
	EnterCriticalSection(&cs_hNvSharedSurface);
	if (hNvSharedSurface) {
		NvIFRLib.DestroySharedSurface(pDeviceR, hNvSharedSurface);
		pDeviceR = NULL;
		hNvSharedSurface = NULL;
		LOG_DEBUG(logger, "shared surface destroyed");
	}
	LeaveCriticalSection(&cs_hNvSharedSurface);
}
