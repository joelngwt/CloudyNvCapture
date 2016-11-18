/*
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma once
#include <d3dx9.h>
#include <stdlib.h>
#include <stdio.h>
#include "NvFBC\NvFBC.h"


// Simple macro which checks for NvFBC errors
#define NVFBC_SAFE_CALL(result) nvfbcSafeCall(result, __FILE__, __LINE__)

inline void nvfbcSafeCall(NVFBCRESULT result, const char *file, const int line)
{
    if(result != NVFBC_SUCCESS)
    {
        fprintf(stderr, "NvFBC call failed %s:%d\n", file, line);
        exit(-1);
    }
}



NvU32 GetFramesFB(LPDIRECT3DDEVICE9 pDevice, HWND hWnd, NvU32 dwWidth, NvU32 dwHeight);