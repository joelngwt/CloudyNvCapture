/*!
 * \brief
 * Application parameters set by the launcher for encoding control
 * 
 * \file
 * 
 * A shared memory is created to contain the needed application parameters.
 * The operations on the shared memory is encapsulated in the 
 * AppParamManager class for easier use.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma once

#include <tchar.h>
#include "ControlInfo.h"

#define N_USER_INPUT 16

struct AppParam
{
	DWORD hwnd;
	BOOL bForceHwnd;

	int iGpu;
	char szAudioKeyword[80];
	BOOL bDwm;

	UINT cxEncoding, cyEncoding;
	BOOL bHEVC;
	DWORD dwAvgBitRate;
	DWORD dwGOPLength;
	BOOL bEnableYUV444Encoding;
	DWORD eRateControl;
	DWORD ePresetConfig;
	int numPlayers;
	int rows;
	int cols;
	int splitHeight;
	int splitWidth;
	int height;
	int width;

	char szStreamingDest[80];

	// Total number of slots of the ring buffer. Must be set to N_USER_INPUT upon initialization
	DWORD nUserInput;
	/* Absolute index of the next empty slot. 
	   To index into aui, use the modulo value (iUserInput % nUserInput)
	   Setting iUserInput to (DWORD)-1 signals application termination.*/
	DWORD iUserInput;
	UserInput aui[N_USER_INPUT];

	BOOL bForceCdeclInEnumDevicesCallback;
};

class AppParamManager
{
	enum AppStatus {
		APP_UNINITIALIZED,
		APP_INITIALIZED,
	};
	struct SharedMemStruct {
		DWORD dwSize;
		AppStatus eAppStatus;
		AppParam appParam;
	};

public:
	AppParamManager(const ULONGLONG *pId = NULL);
	~AppParamManager();
	TCHAR *GetAppParamEnv()
	{
		return szAppParamEnv;
	}
	AppParam *GetAppParam() 
	{
		return pStruct ? &pStruct->appParam : NULL;
	}
	BOOL IsAppUninitialized() 
	{
		return pStruct->eAppStatus == APP_UNINITIALIZED;
	}

private:
	BOOL CreateSharedMem(TCHAR *szMemName);
	BOOL OpenSharedMem(TCHAR *szMemName);
	HANDLE hMem;
	SharedMemStruct *pStruct;
	TCHAR *szAppParamEnv;
};
