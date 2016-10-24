/*!
 * \brief
 * NvIFREncoderDXGI (derived from NvIFREncoder) is the interface used in 
 * hooked D3D10/11 API calls.
 *
 * \file
 * NvIFREncoderDXGI is extended from NvIFREncoderDXGIBase.
 * UpdateSharedSurface() is specific to NvIFREncoderDXGI. If keyed mutex 
 * is not available, the shared surface is opened/closed every time 
 * UpdateSharedSurface() is called; otherwise, the shared resource is 
 * always opened, and the keyed mutex is used to improve performance.
 * 
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma once

#include <NvIFRLibrary.h>
#include "NvIFREncoderDXGIBase.h"
#include "Logger.h"

extern simplelogger::Logger *logger;

template <class IDevice, class ITexture>
class NvIFREncoderDXGI : public NvIFREncoderDXGIBase {
public:
	NvIFREncoderDXGI(void *pPresenter, int nWidth, int nHeight, DXGI_FORMAT d3dFormat, BOOL bKeyedMutex, AppParam *pAppParam) :
		NvIFREncoderDXGIBase(pPresenter, nWidth, nHeight, d3dFormat, bKeyedMutex, pAppParam),
		pLastDeviceR(NULL), pSharedTextureR(NULL), pCommitTextureR(NULL) {}
	~NvIFREncoderDXGI()
	{
		if (!bStopEncoder) {
			StopEncoder();
		}
	}

	BOOL NvIFREncoderDXGI<IDevice, ITexture>::UpdateSharedSurface(IDevice *pDeviceR, ITexture *pBackBufferR)
	{
		if (!hSharedTexture || !pDeviceR || !pBackBufferR) {
			LOG_ERROR(logger, __FUNCTION__ << " " << __LINE__);
			return FALSE;
		}

		if (pLastDeviceR != pDeviceR) {
			LOG_INFO(logger, "DXGI: to open shared resource");
			
			pLastDeviceR = NULL;
			if (pSharedTextureR) {
				pSharedTextureR->Release();
				pSharedTextureR = NULL;
			}
			if (pCommitTextureR) {
				pCommitTextureR->Release();
				pCommitTextureR = NULL;
			}

			HRESULT hr = pDeviceR->OpenSharedResource(hSharedTexture, __uuidof(ITexture), (void **)&pSharedTextureR);
			if (FAILED(hr)) {
				LOG_ERROR(logger, "failed to open shared resource, hr=" << hr);
				return FALSE;
			}
			LOG_INFO(logger, "succeeded to open shared resource");

			hr = CreateCommitTexture(pDeviceR, dxgiFormat, &pCommitTextureR);
			if (FAILED(hr)) {
				LOG_DEBUG(logger, "CreateCommitSurface() failed, hr=" << hr);
				return FALSE;
			}

			pLastDeviceR = pDeviceR;
		}

		if (!bKeyedMutex) {
			CopyResource(pDeviceR, pSharedTextureR, pBackBufferR);
			CommitUpdate(pDeviceR, pCommitTextureR, pSharedTextureR);
		} else {
			IDXGIKeyedMutex *pDXGIKeyedMutex;
			if (FAILED(pSharedTextureR->QueryInterface(_uuidof(IDXGIKeyedMutex), (void **)&pDXGIKeyedMutex))) {
				return FALSE;
			}
			pDXGIKeyedMutex->AcquireSync(0, INFINITE);
			CopyResource(pDeviceR, pSharedTextureR, pBackBufferR);
			pDXGIKeyedMutex->ReleaseSync(0);
			pDXGIKeyedMutex->Release();
		}

		return TRUE;
	}

protected:
	virtual void CleanupNvIFR()
	{
		if (pSharedTextureR) {
			pSharedTextureR->Release();
		}
		if (pCommitTextureR) {
			pCommitTextureR->Release();
		}
		NvIFREncoderDXGIBase::CleanupNvIFR();
	}

private:
	static void CopyResource(ID3D10Device *pDeviceX, ID3D10Texture2D * pSharedTextureX, ID3D10Texture2D * pBackBufferX) 
	{
		D3D10_TEXTURE2D_DESC desc;
		pBackBufferX->GetDesc(&desc);
		if (desc.SampleDesc.Count == 1) {
			pDeviceX->CopyResource(pSharedTextureX, pBackBufferX);
		} else {
			pDeviceX->ResolveSubresource(pSharedTextureX, 0, pBackBufferX, 0, desc.Format);
		}
	}
	static void CopyResource(ID3D11Device *pDeviceX, ID3D11Texture2D * pSharedTextureX, ID3D11Texture2D * pBackBufferX) 
	{
		ID3D11DeviceContext *pContextX;
		pDeviceX->GetImmediateContext(&pContextX);
		D3D11_TEXTURE2D_DESC desc;
		pBackBufferX->GetDesc(&desc);
		if (desc.SampleDesc.Count == 1) {
			pContextX->CopyResource(pSharedTextureX, pBackBufferX);
		} else {
			pContextX->ResolveSubresource(pSharedTextureX, 0, pBackBufferX, 0, desc.Format);
		}
		pContextX->Release();
	}

private:
	IDevice *pLastDeviceR;
	ITexture *pSharedTextureR;
	ITexture *pCommitTextureR;
};
