#pragma once
#include <windows.h>
#include <string>
#include <stdlib.h>
#include <ObjBase.h>
#include <Guiddef.h>

inline std::string wcs2mbstring(const wchar_t *wcs)
{
	size_t len = wcslen(wcs) + 1;
	char *mbs = new char[len];
	wcstombs(mbs, wcs, len);

	std::string mbstring(mbs);
	delete mbs;
	return mbstring;
}

inline std::wstring mbs2wcstring(const char *mbs)
{
	size_t len = strlen(mbs) + 1;
	wchar_t *wcs = new wchar_t[len];
	mbstowcs(wcs, mbs, len);

	std::wstring wcstring(wcs);
	delete wcs;
	return wcstring;
}

inline std::string CLSID2String(REFCLSID clsid)
{
	LPOLESTR wcs;
	StringFromCLSID(clsid, &wcs);
	if (!wcs) {
		return std::string();
	}

	std::string mbstring = wcs2mbstring(wcs);
	CoTaskMemFree(wcs);
	return mbstring;
}

inline std::string IID2String(REFIID riid)
{
	LPOLESTR wcs;
	StringFromIID(riid, &wcs);
	if (!wcs) {
		return std::string();
	}

	std::string mbstring = wcs2mbstring(wcs);
	CoTaskMemFree(wcs);
	return mbstring;
}

template <class Vtbl, class Interface>
HWND GetDeviceWindow(Vtbl &vtbl, Interface *This)
{
	IDirect3DSwapChain9 *pSwapChain = NULL;
	if (vtbl.GetSwapChain(This, 0, &pSwapChain) != D3D_OK) {
		return NULL;
	}
	D3DPRESENT_PARAMETERS dp;
	pSwapChain->GetPresentParameters(&dp);
	pSwapChain->Release();
	return dp.hDeviceWindow;
}

inline DWORD GetSleepTime(int nFrameRate)
{
	static DWORD tLast, tSleep;
	static double dResidual;

	if (nFrameRate <= 0 || nFrameRate > 1000) {
		return 0;
	}

	DWORD t = timeGetTime();
	double
		dPeriod = 1000.0 / nFrameRate,
		dSleep = dPeriod - (t - tLast - tSleep) + dResidual;
	tSleep = (DWORD)dSleep;
	if (tSleep > dPeriod) {
		tSleep = (DWORD)dPeriod;
		dResidual = 0;
	} else {
		dResidual = dSleep - tSleep;
	}
	tLast = t;
	return tSleep;
}

inline BOOL WINAPI WaitOnAddress_BeforeWin8(
	volatile VOID * Address,
	PVOID CompareAddress,
	SIZE_T AddressSize,
	DWORD dwMilliseconds)
{
	while (!memcmp((void *)Address, CompareAddress, AddressSize)) {
		if (dwMilliseconds-- == 0) {
			return FALSE;
		}
		Sleep(1);
	}
	return TRUE;
}
