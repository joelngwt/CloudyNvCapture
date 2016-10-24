/*!
 * \brief
 * Grid-capable adapter selection
 *
 * \file
 *
 * The selection criterion is the adapter description name. If a 
 * paticular substring is found, it is regarded as Grid-capable.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma once
#include <vector>
#include <algorithm>
#include <d3d9.h>
#include <dxgi.h>

using namespace std;

extern simplelogger::Logger *logger;

static char *keywords[] = {"GRID", "Quadro"};

template <class D3D>
class AdapterAccessor_D3D9
{
public:
	AdapterAccessor_D3D9(
		D3D *pD3D,
		UINT(STDMETHODCALLTYPE * GetAdapterCount)(D3D *This) = NULL,
		HRESULT(STDMETHODCALLTYPE *GetAdapterIdentifier)(D3D *This, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier) = NULL,
		HMONITOR(STDMETHODCALLTYPE * GetAdapterMonitor)(D3D *This, UINT Adapter) = NULL
	) : pD3D(pD3D), mGetAdapterCount(GetAdapterCount), mGetAdapterIdentifier(GetAdapterIdentifier), mGetAdapterMonitor(GetAdapterMonitor)
	{}
	UINT GetAdapterCount() 
	{
		if (mGetAdapterCount) {
			return mGetAdapterCount(pD3D);
		}
		return pD3D->GetAdapterCount();
	}
	HRESULT GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier)
	{
		if (mGetAdapterIdentifier) {
			return mGetAdapterIdentifier(pD3D, Adapter, Flags, pIdentifier);
		}
		return pD3D->GetAdapterIdentifier(Adapter, Flags, pIdentifier);
	}
	HMONITOR GetAdapterMonitor(UINT Adapter)
	{
		if (mGetAdapterMonitor) {
			return mGetAdapterMonitor(pD3D, Adapter);
		}
		return pD3D->GetAdapterMonitor(Adapter);
	}
private:
	D3D *pD3D;
	UINT(STDMETHODCALLTYPE * mGetAdapterCount)(D3D *This);
	HRESULT(STDMETHODCALLTYPE *mGetAdapterIdentifier)(D3D *This, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier);
	HMONITOR(STDMETHODCALLTYPE * mGetAdapterMonitor)(D3D *This, UINT Adapter);
};

template <class Adapter, class Factory>
class AdapterAccessor_DXGI
{
public:
	/* If EnumAdapters isn't provided, template must be <IDXGIFactory1, IDXGIAdapter>; 
	   otherwise, types will be incorrect.*/
	AdapterAccessor_DXGI(
		Factory *This = NULL,
		HRESULT(STDMETHODCALLTYPE *EnumAdapters)(Factory *, UINT, Adapter **) = NULL
	) : mEnumAdapters(EnumAdapters), pFactory(This)
	{
		if (!mEnumAdapters) {
			if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)&pFactory))){
				LOG_ERROR(logger, "CreateDXGIFactory1() failed in " << __FUNCTION__);
			}
			return;
		}
		if (!This) {
			LOG_ERROR(logger, "Argument This can't be NULL when EnumAdapters isn't NULL");
		}
	}
	~AdapterAccessor_DXGI()
	{
		if (!mEnumAdapters && pFactory) {
			pFactory->Release();
		}
	}
	HRESULT EnumAdapters(UINT iAdapter, Adapter **ppAdapter)
	{
		if (mEnumAdapters) {
			return mEnumAdapters(pFactory, iAdapter, ppAdapter); 
		}
		if (!pFactory) {
			LOG_ERROR(logger, "No DXGI factory");
			*ppAdapter = NULL;
			return DXGI_ERROR_NOT_FOUND;
		}
		return pFactory->EnumAdapters(iAdapter, (IDXGIAdapter **)ppAdapter);
	}
private:
	Factory *pFactory;
	HRESULT(STDMETHODCALLTYPE *mEnumAdapters)(Factory *, UINT, Adapter **);
};

inline vector<UINT> SortOrdinal(vector<RECT> &vRect)
{
	vector<UINT> vOrdinal(vRect.size());
	for (UINT i = 0; i < vOrdinal.size(); i++) {
		vOrdinal[i] = i;
	}

	struct Comparator {
		bool operator() (UINT i, UINT j) {
			return (*pvRect)[i].top == (*pvRect)[j].top ?
				(*pvRect)[i].left < (*pvRect)[j].left : (*pvRect)[i].top < (*pvRect)[j].top;
		}
		vector<RECT> *pvRect;
		Comparator(vector<RECT> *pvRect) : pvRect(pvRect) {}
	} comparator(&vRect);
	sort(vOrdinal.begin(), vOrdinal.end(), comparator);
	return vOrdinal;
}

template <class D3D>
UINT GetGridAdapterOrdinal_D3D9(int iOrdinal, AdapterAccessor_D3D9<D3D> &accessor)
{
	vector<RECT> vRect;
	for (UINT i = 0; i < accessor.GetAdapterCount(); i++) {
		HMONITOR hMon = accessor.GetAdapterMonitor(i);
		MONITORINFO mi = { sizeof(MONITORINFO) };
		GetMonitorInfo(hMon, &mi);
		vRect.push_back(mi.rcMonitor);
	}

	if (vRect.size() == 0) {
		LOG_ERROR(logger, "No adapter found.");
		return 0;
	}

	vector<UINT> vOrdinal = SortOrdinal(vRect);
	if (vOrdinal.size() == 1) {
		LOG_INFO(logger, "Only one adapter found. Using it without further checks.");
		return 0;
	}

	if (iOrdinal < 0) {
		for (size_t i = 0; i < vOrdinal.size(); i++) {
			D3DADAPTER_IDENTIFIER9 id;
			accessor.GetAdapterIdentifier(vOrdinal[i], 0, &id);
			LOG_TRACE(logger, "D3D9 adapter#" << i << ": " << id.Description);
			for (int j = 0; j < sizeof(keywords) / sizeof(keywords[0]); j++) {
				if (strstr(id.Description, keywords[j])) {
					LOG_INFO(logger, "Grid adapter found (D3D9): " << id.Description << ", coordinate ordinal=" << i << ", D3D ordinal=" << vOrdinal[i]);
					return vOrdinal[i];
				}
			}
		}
	}

	if ((size_t)iOrdinal >= vOrdinal.size()) {
		LOG_ERROR(logger, "Adapter ordinal out of range: coordinate ordinal=" << iOrdinal << ". Using the default adapter.");
		return 0;
	}

	LOG_INFO(logger, "(D3D9) Using adapter: coordinate ordinal=" << iOrdinal << ", D3D ordinal=" << vOrdinal[iOrdinal])
	return vOrdinal[iOrdinal];
}

template <class Adapter, class Factory>
UINT GetGridAdapterOrdinal_DXGI(int iOrdinal, Adapter **ppAdapter, AdapterAccessor_DXGI<Adapter, Factory> &accessor)
{
	if (ppAdapter) {
		accessor.EnumAdapters(0, ppAdapter);
	}

	vector<RECT> vRect;
	Adapter *pAdapter = NULL;
	for (UINT i = 0; accessor.EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; i++) {
		IDXGIOutput *pOutput = NULL;
		pAdapter->EnumOutputs(0, &pOutput);
		if (!pOutput) {
			pAdapter->Release();
			break;
		}
		DXGI_OUTPUT_DESC desc;
		pOutput->GetDesc(&desc);
		pOutput->Release();
		pAdapter->Release();
		vRect.push_back(desc.DesktopCoordinates);
	}

	if (vRect.size() == 0) {
		LOG_ERROR(logger, "No adapter found.");
		return 0;
	}

	vector<UINT> vOrdinal = SortOrdinal(vRect);
	if (vOrdinal.size() == 1) {
		LOG_INFO(logger, "Only one adapter found. Using it without further checks.");
		return 0;
	}

	if (iOrdinal < 0) {
		for (size_t i = 0; i < vOrdinal.size(); i++) {
			accessor.EnumAdapters(vOrdinal[i], &pAdapter);
			DXGI_ADAPTER_DESC desc;
			pAdapter->GetDesc(&desc);
			char szDesc[80];
			wcstombs(szDesc, desc.Description, sizeof(szDesc));
			for (int j = 0; j < sizeof(keywords) / sizeof(keywords[0]); j++) {
				if (strstr(szDesc, keywords[j])) {
					LOG_INFO(logger, "Grid adapter found (DXGI): " << szDesc << ", coordinate ordinal=" << i << ", D3D ordinal=" << vOrdinal[i]);
					pAdapter->Release();
					return vOrdinal[i];
				}
			}
			pAdapter->Release();
		}
		LOG_WARN(logger, "No GRID adapter. Using the default adapter.");
		return 0;
	}

	if ((size_t)iOrdinal >= vOrdinal.size()) {
		LOG_ERROR(logger, "Adapter ordinal out of range: coordinate ordinal=" << iOrdinal << ". Using the default adapter.");
		return 0;
	}

	if (ppAdapter) {
		if (*ppAdapter) {
			(*ppAdapter)->Release();
		}
		accessor.EnumAdapters(vOrdinal[iOrdinal], ppAdapter);
	}
	LOG_INFO(logger, "(DXGI) Using adapter: coordinate ordinal=" << iOrdinal << ", D3D ordinal=" << vOrdinal[iOrdinal])
	return vOrdinal[iOrdinal];
}
