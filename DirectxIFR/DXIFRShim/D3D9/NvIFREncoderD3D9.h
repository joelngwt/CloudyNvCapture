/*!
 * \brief
 * NvIFREncoderD3D9 (derived from NvIFREncoder) is the interface used in 
 * hooked D3D9 API calls.
 *
 * \file
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma once

#include <d3d9.h>
#include <NvIFRLibrary.h>
#include "NvIFREncoder.h"

class NvIFREncoderD3D9 : public NvIFREncoder {
public:
	NvIFREncoderD3D9(void *pPresenter, int nWidth, int nHeight, D3DFORMAT d3dFormat, AppParam *pAppParam) :
		NvIFREncoder(pPresenter, nWidth, nHeight, DXGI_FORMAT_B8G8R8A8_UNORM, FALSE, pAppParam), 
		pDevice(NULL),
		d3dFormat(d3dFormat),
		hNvSharedSurface(NULL), pDeviceR(NULL) 
	{
		InitializeCriticalSection(&cs_hNvSharedSurface);
	}
	~NvIFREncoderD3D9()
	{
		if (!bStopEncoder) {
			StopEncoder();
		}
		DeleteCriticalSection(&cs_hNvSharedSurface);
	}

	virtual BOOL UpdateBackBuffer();
	BOOL UpdateSharedSurface(IDirect3DDevice9 * pDeviceR, IDirect3DSurface9 * pBackBufferR);
	void DestroySharedSurface();

private:
	virtual BOOL SetupNvIFR();
	virtual void CleanupNvIFR();

	IDirect3DDevice9Ex *pDevice;
	D3DFORMAT d3dFormat;
	NvIFRLibrary NvIFRLib;

	IFRSharedSurfaceHandle hNvSharedSurface;
	IDirect3DSurface9 * pTransferSurface;

	IDirect3DDevice9 *pDeviceR;
	CRITICAL_SECTION cs_hNvSharedSurface;
};
