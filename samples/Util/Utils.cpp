/*!
* \brief
* Compares the performance of the NvIFR H.264 encoder to the x.264 encoder
*
* \file
*
* This DX9 sample demonstrates the performance of the NvIFR H.264 encoder
* given input encoding parameters for highest perf.
*
* \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */


#include "Util.h"
#include "NvAPI\NvAPI.h"

DWORD GetFramesFB(LPDIRECT3DDEVICE9 pDevice, HWND hWnd, NvU32 dwWidth, NvU32 dwHeight)
{
    NvU32 dwAvailableVidMem = 512 * 1024 * 1024;  // In KByters and by default  512 MB assumed
    NvU32 dwNumFrameVALimited = 0;
    NvU32 dwFramesToFit = 0;

    const NvU32 NODE_CONTEXT_CREATION_OVERHEADS = 64 * 1024 * 1024; // Reserving 64 MB for Context creation overheads

    //Get the available VMEM
    NvAPI_Status result = NVAPI_ERROR;
    NvU32 primaryDisplayId = 0;
    NvPhysicalGpuHandle hNvPhysicalGpu = NULL;
    NV_DISPLAY_DRIVER_MEMORY_INFO ddMemInfo = { 0 };

    if (NvAPI_Initialize() != NVAPI_OK)
    {
        printf("NvAPI_Initialize() failed!\n");
    }
    else
    {
        // determine the location of the window
        MONITORINFOEX MonInfo;
        HMONITOR hMon;

        hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
        ZeroMemory(&MonInfo, sizeof(MonInfo));
        MonInfo.cbSize = sizeof(MonInfo);
        GetMonitorInfo(hMon, &MonInfo);

        result = NvAPI_DISP_GetDisplayIdByDisplayName(MonInfo.szDevice, &primaryDisplayId);
        //result = NvAPI_DISP_GetGDIPrimaryDisplayId(&primaryDisplayId);
        if (result == NVAPI_OK)
        {

            result = NvAPI_SYS_GetPhysicalGpuFromDisplayId(primaryDisplayId, &hNvPhysicalGpu);
            if (result != NVAPI_OK)
            {
                printf("NvAPI_SYS_GetPhysicalGpuFromDisplayId() failed!\n");
            }
            else
            {
                ddMemInfo.version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER;

                if (NvAPI_GPU_GetMemoryInfo(hNvPhysicalGpu, &ddMemInfo) != NVAPI_OK)
                {
                    printf("NvAPI_GPU_GetMemoryInfo() failed!\n");
                }
                else
                {
                    dwAvailableVidMem = ddMemInfo.curAvailableDedicatedVideoMemory * 1024;
                }
            }
        }
    }


    // Check for user mode VA limitation
    const int MAX_NUM_SURFACE = 2048;
    LPDIRECT3DTEXTURE9      pTmpTexVid[MAX_NUM_SURFACE];
    BOOL bSuccess = TRUE;
    HRESULT hr;

    for (dwNumFrameVALimited = 0; dwNumFrameVALimited < MAX_NUM_SURFACE; dwNumFrameVALimited++)
    {
        hr = pDevice->CreateTexture(dwWidth, dwHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pTmpTexVid[dwNumFrameVALimited], NULL);

        if (hr != S_OK)
            break;
    }

    for (DWORD i = 0; i < dwNumFrameVALimited; i++)
    {
        pTmpTexVid[i]->Release();
    }
    DWORD dwFrameSize = dwWidth * dwHeight * 4;
    // Calucalte the actual number surface that should be used after substracting the node context creation overheads for encoding.
    dwAvailableVidMem = min(dwAvailableVidMem, dwNumFrameVALimited * dwFrameSize);
    dwFramesToFit = (dwAvailableVidMem - NODE_CONTEXT_CREATION_OVERHEADS) / dwFrameSize;
    return dwFramesToFit;
}
