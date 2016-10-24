/*!
 * \brief
 * The implementation of AppParamManager
 *
 * \file
 *
 * A shared memory is created to hold the needed application parameters.
 * After the shared memory is created, its name is written to an environment
 * variable; before opened, its name is read from the environment variable.
 * The environment variable is the key to connect the launcher and the 
 * application.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <windows.h>
#include <stdio.h>
#include "Logger.h"
#include "AppParam.h"

extern simplelogger::Logger *logger;

AppParamManager::AppParamManager(const ULONGLONG *pId) : 
hMem(NULL), pStruct(NULL), 
	szAppParamEnv(NULL)
{
	TCHAR szMemName[MAX_PATH];
	const TCHAR 
		*szAppParamEnvKey = _T("GRID_AppParam_MemName"), 
		*szAppParamEnvValueFmt = _T("GRID_AppParam_0x%llX");
	if (pId) {
		_stprintf_s(szMemName, sizeof(szMemName) / sizeof(szMemName[0]), szAppParamEnvValueFmt, *pId);
		if (!CreateSharedMem(szMemName)) {
			LOG_ERROR(logger, "Failed to create shared memory for AppParam");
			return;
		}
		SetEnvironmentVariable(szAppParamEnvKey, szMemName);
	} else {
		if (!GetEnvironmentVariable(szAppParamEnvKey, szMemName, sizeof(szMemName))) {
			LOG_DEBUG(logger, "No shared memroy to open (no shared memory name in environment variable).");
			return;
		}
		OpenSharedMem(szMemName);
	}
	size_t nc = _tcslen(szAppParamEnvKey) + _tcslen(szMemName) + 2;
	szAppParamEnv = new TCHAR[nc];
	_stprintf_s(szAppParamEnv, nc, _T("%s=%s"), szAppParamEnvKey, szMemName);
}
AppParamManager::~AppParamManager()
{
	if (szAppParamEnv) {
		delete[] szAppParamEnv;
	}
	if (pStruct) {
		UnmapViewOfFile(pStruct);
	}
	if (hMem) {
		CloseHandle(hMem);
	}
}

BOOL AppParamManager::CreateSharedMem(TCHAR *szMemName) 
{
	DWORD dwMemSize = sizeof(SharedMemStruct);
	hMem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, dwMemSize, szMemName);
	if (!hMem) {
		LOG_ERROR(logger, "CreateFileMapping() failed.");
		return FALSE;
	}
	pStruct = (SharedMemStruct *)MapViewOfFile(hMem, FILE_MAP_ALL_ACCESS, 0, 0, dwMemSize);
	if (!pStruct) {
		LOG_DEBUG(logger, "MapViewOfFile() failed.");
		return FALSE;
	}
	pStruct->dwSize = dwMemSize;
	pStruct->eAppStatus = APP_UNINITIALIZED;
	pStruct->appParam.nUserInput = N_USER_INPUT;
	return TRUE;
}

BOOL AppParamManager::OpenSharedMem(TCHAR *szMemName) 
{
	// Retrieve the size of the shared memory
	hMem = OpenFileMapping(FILE_MAP_READ, TRUE, szMemName);
	if (!hMem) {
		LOG_ERROR(logger, "OpenFileMapping() #1 fails.");
		return FALSE;
	}
	DWORD *pdwSize = (DWORD *)MapViewOfFile(hMem, FILE_MAP_READ, 0, 0, sizeof(DWORD));
	if (!pdwSize) {
		LOG_ERROR(logger, "MapViewOfFile() #1 fails.");
		return FALSE;
	}
	DWORD dwMemSize = *pdwSize;
	UnmapViewOfFile(pdwSize);
	CloseHandle(hMem);
	hMem = NULL;

	// The shared memory must be large enough to hold the struct
	if (dwMemSize < sizeof(SharedMemStruct)) {
		LOG_ERROR(logger, "Shared memory is smaller than the defined struct. Refuse to open it.");
		return FALSE;
	}
	if (dwMemSize > sizeof(SharedMemStruct)) {
		LOG_DEBUG(logger, "Shared memory doesn't match the defined struct. Some information may be invisible.");
	}

	// Open the shared memory for read & write
	hMem = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, TRUE, szMemName);
	if (!hMem) {
		LOG_ERROR(logger, "OpenFileMapping() #2 fails.");
		return FALSE;
	}
	pStruct = (SharedMemStruct *)MapViewOfFile(hMem, FILE_MAP_ALL_ACCESS, 0, 0, dwMemSize);
	if (!pStruct) {
		LOG_ERROR(logger, "MapViewOfFile() #2 fails.");
		return FALSE;
	}
	pStruct->eAppStatus = APP_INITIALIZED;
	return TRUE;
}
