/*!
 * \brief
 * Demonstrates the use of NvFBCToSys to copy the desktop to system memory. 
 *
 * \file
 *
 * This sample demonstrates the use of NvFBCToSys class to record the
 * entire desktop. It covers loading the NvFBC DLL, loading the NvFBC 
 * function pointers, creating an instance of NvFBCToSys and using it 
 * to copy the full screen frame buffer into system memory.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <windows.h>
#include <d3d9.h>
#include <stdio.h>
#include <stdlib.h>
#include <NvFBCLibrary.h>
#include <NvFBC/nvFBCToSys.h>
#include <NvFBC/nvFBCToDx9Vid.h>
#include <NvFBC/nvFBCHWEnc.h>
#include "Grabber.h"
#include "bitmap.h"

Grabber::~Grabber()
{
    SAFE_RELEASE_1(m_pGDICursorSurf);
    SAFE_RELEASE_1(m_pFBCCursorSurf);
    SAFE_RELEASE_1(m_pD3D9Device);
    SAFE_RELEASE_1(m_pD3DEx);

    lib.close();
}

HRESULT Grabber::InitD3D9()
{
    HRESULT hr = S_OK;
    D3DPRESENT_PARAMETERS d3dpp;
    D3DADAPTER_IDENTIFIER9 adapterId;
    unsigned int iAdapter = NULL;

    Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3DEx);

    if (m_dwAdapterIdx >= m_pD3DEx->GetAdapterCount ())
    {
        fprintf (stderr, __FUNCTION__": [%d] Error: (deviceID=%d) is not a valid GPU device. Headless video devices will not be detected.  <<\n\n", m_dwAdapterIdx, m_dwAdapterIdx);
        return E_FAIL;
    }

    hr = m_pD3DEx->GetAdapterIdentifier (m_dwAdapterIdx, 0, &adapterId);
    if (hr != S_OK)
    {
        fprintf (stderr, __FUNCTION__": [%d] Error: (deviceID=%d) is not a valid GPU device. <<\n\n", m_dwAdapterIdx, m_dwAdapterIdx);
        return E_FAIL;
    }

    // Create the Direct3D9 device and the swap chain. In this example, the swap
    // chain is the same size as the current display mode. The format is RGB-32.
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = true;
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferWidth = 1280;
    d3dpp.BackBufferHeight = 720;
    d3dpp.BackBufferCount = 1;
    d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    d3dpp.Flags = D3DPRESENTFLAG_VIDEO;
    DWORD dwBehaviorFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING;
    hr = m_pD3DEx->CreateDeviceEx(
                                    m_dwAdapterIdx,
                                    D3DDEVTYPE_HAL,
                                    NULL,
                                    dwBehaviorFlags,
                                    &d3dpp,
                                    NULL,
                                    &m_pD3D9Device);
    if (SUCCEEDED(hr))
    {
        hr = m_pD3D9Device->CreateRenderTarget(128, 128, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &m_pGDICursorSurf, NULL);
        if (SUCCEEDED(hr))
        {
            m_pD3D9Device->ColorFill(m_pGDICursorSurf, NULL, (D3DCOLOR)0);
        }

        hr = m_pD3D9Device->CreateRenderTarget(128, 128, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &m_pFBCCursorSurf, NULL);
        if (SUCCEEDED(hr))
        {
            m_pD3D9Device->ColorFill(m_pFBCCursorSurf, NULL, (D3DCOLOR)0);
        }
    }
    return hr;
}

bool Grabber::init()
{
    NvFBCStatusEx status = {0};
    status.dwVersion = NVFBC_STATUS_VER;
    if (lib.load())
    {
        if (NVFBC_SUCCESS == lib.getStatus(&status))
        {
            if (status.bIsCapturePossible && !status.bCurrentlyCapturing)
            {
                if (status.bCanCreateNow)
                {
                    if (!SUCCEEDED(InitD3D9()))
                    {
                        fprintf(stderr, __FUNCTION__": [%d] Failed to init d3d9\n", m_dwAdapterIdx);
                        return false;
                    }
                }
                NvFBCCreateParams params = { 0 };
                params.dwVersion = NVFBC_CREATE_PARAMS_VER;
                params.dwInterfaceType = dwFBCType;
                params.pDevice = m_pD3D9Device;
                NVFBCRESULT res = lib.createEx (&params);
                if (NVFBC_SUCCESS != res)
                {
                    fprintf (stderr, __FUNCTION__": [%d] Failed to create NVFBC session. Error %d\n", m_dwAdapterIdx, res);
                }
                m_pvFBC = params.pNvFBC;
                m_dwMaxWidth = params.dwMaxDisplayWidth;
                m_dwMaxHeight = params.dwMaxDisplayHeight;
                if (m_pvFBC)
                {
                    fprintf(stderr, __FUNCTION__":  [%d] Created NVFBC session instance\n", m_dwAdapterIdx);
                    return true;
                }
            }
        }
    }
    else
    {
        fprintf(stderr, __FUNCTION__": [%d] Failed to load NVFBC\n");
        return false;
    }

    fprintf(stderr, __FUNCTION__": [%d] Failed to Create NVFBC session.\n", m_dwAdapterIdx);
    if (!status.bIsCapturePossible)
        fprintf(stderr, __FUNCTION__": [%d] NVFBC not enabled\n", m_dwAdapterIdx);
    else if(status.bCurrentlyCapturing)
        fprintf(stderr, __FUNCTION__": [%d] Another NVFBC instance is running\n", m_dwAdapterIdx);

    return false;
}

void Grabber::CompareCursor(NVFBC_CURSOR_CAPTURE_PARAMS *pParams, BOOL bDumpFrame)
{
    POINT pt = {0};
    GetPhysicalCursorPos(&pt);

    char filename1[256] = {0};
    char filename2[256] = {0};
    HDC hSurfaceDC;
    HWND hWindow = WindowFromPhysicalPoint(pt);
    DWORD windowTid = GetWindowThreadProcessId(hWindow, 0);
    DWORD currentTid = GetCurrentThreadId();
    HCURSOR hCursor = NULL;
    DWORD dwFlags = 0;

    char logname[64] = {0};
    sprintf(logname, "Cursorlog_adapter%d.txt", m_dwAdapterIdx);
    FILE *outputFile = fopen(logname, "at");
    if (pParams->bIsHwCursor)
    {
        fprintf(stderr,__FUNCTION__": [%d] HW cursor update. Frame No %d\n", m_dwAdapterIdx, m_uFrameNo);
        fprintf(outputFile,__FUNCTION__": [%d] %3d.\tX: %3d\tY: %3d\tflags: %3d\t", m_dwAdapterIdx, m_uFrameNo, pParams->dwXHotSpot, pParams->dwYHotSpot, pParams->dwPointerFlags );
        fprintf(outputFile,__FUNCTION__": [%d] height: %2d,\twidth: %2d,\tpitch: %2d,\tcounter: %2d\n", m_dwAdapterIdx,pParams->dwHeight, pParams->dwWidth, pParams->dwPitch, pParams->dwUpdateCounter);
    }
    else
    {
        fprintf(stderr, __FUNCTION__": [%d] OS is using SW cursor\n", m_dwAdapterIdx);
        fprintf(outputFile, __FUNCTION__": [%d] %3d.\tOS is using SW cursor\n", m_dwAdapterIdx, m_uFrameNo);
    }
    fclose(outputFile);

    if (!pParams->bIsHwCursor)
        return;

    D3DLOCKED_RECT lock = {0};
#if 0
    m_pD3D9Device->ColorFill(m_pGDICursorSurf, NULL, (D3DCOLOR)0);
    CURSORINFO cursorInfo = {0};
    cursorInfo.cbSize = sizeof(CURSORINFO);
    if (!GetCursorInfo(&cursorInfo))
    {
        fprintf(stderr, "OS API GetCursorInfo failed with error 0x%x", GetLastError());
        return;
    }
    sprintf(filename1, "Test_Cursors_OS_%d_adapter%d.bmp", m_uFrameNo, m_dwAdapterIdx);


    if (windowTid != currentTid)
    {
        AttachThreadInput(windowTid, currentTid, TRUE);
    }

    fprintf(stderr, "Writing cursor captured through OS API to %s. Frame No %d\n", filename1, m_uFrameNo);
    m_pGDICursorSurf->GetDC(&hSurfaceDC);
    hCursor = GetCursor();
    DrawIcon(hSurfaceDC, 10, 10, hCursor);
    m_pGDICursorSurf->ReleaseDC(hSurfaceDC);
    HRESULT hr = m_pGDICursorSurf->LockRect(&lock, NULL, D3DLOCK_READONLY);
    if (SUCCEEDED(hr))
    {
        SaveARGB(filename1, (BYTE *)lock.pBits, 128, 128, 128);
        fprintf(stderr, "Wrote cursor captured through OS API to %s. Frame No %d\n", filename1, m_uFrameNo);
    }
    else
    {
        fprintf(stderr, "Failed to lock temp buffer. Frame no %d\n", m_uFrameNo);
    }
    m_pGDICursorSurf->UnlockRect();
#endif

    POINTERFLAGS flags;
    flags.Value = pParams->dwPointerFlags;

    m_pD3D9Device->ColorFill(m_pFBCCursorSurf, NULL, (D3DCOLOR)0);
    sprintf(filename2, "Test_Cursors_NV_%d_adapter%d.bmp", m_uFrameNo, m_dwAdapterIdx);

    if (flags.Monochrome)
    {
        fprintf(stderr,__FUNCTION__": [%d] Creating cursor using data captured through NVFBC API. Frame No %d\n", m_dwAdapterIdx, m_uFrameNo);
        char *pXORMask = (char *)pParams->pBits + pParams->dwWidth*pParams->dwPitch;
        HCURSOR hCursor1 = CreateCursor(GetModuleHandle(NULL), pParams->dwXHotSpot, pParams->dwYHotSpot, pParams->dwWidth, pParams->dwHeight, pParams->pBits, (void *)pXORMask);
        if (hCursor1)
        {
            fprintf(stderr,__FUNCTION__": [%d] Writing cursor captured through NVFBC API to %s. Frame No %d\n", m_dwAdapterIdx, filename2, m_uFrameNo);
            m_pFBCCursorSurf->GetDC(&hSurfaceDC);
            DrawIcon(hSurfaceDC, 10, 10, hCursor1);
            DestroyCursor(hCursor1);
            m_pFBCCursorSurf->ReleaseDC(hSurfaceDC);

            RECT r = {0,0,128,128};
            HRESULT hr = m_pFBCCursorSurf->LockRect(&lock, &r, D3DLOCK_READONLY);
            if (SUCCEEDED(hr))
            {
                SaveARGB(filename2, (BYTE *)lock.pBits, 128, 128, 128);
                fprintf(stderr,__FUNCTION__": [%d] Wrote cursor captured through NVFBC API to %s. Frame No %d\n", m_dwAdapterIdx, filename2, m_uFrameNo);
            }
            else
            {
                fprintf(stderr,__FUNCTION__": [%d] Failed to lock temp buffer. Frame no %d\n", m_dwAdapterIdx, m_uFrameNo);
            }
            m_pFBCCursorSurf->UnlockRect();
        }
        else
        {
            fprintf(stderr,__FUNCTION__": [%d] Unable to create MONOCHROME cursor using captured data\n", m_dwAdapterIdx);
        }
    }
    else if (flags.Color)
    {
        SaveARGB(filename2, (BYTE *)pParams->pBits, pParams->dwWidth, pParams->dwHeight, pParams->dwWidth);
    }
    else if (flags.MaskedColor)
    {
        fprintf(stderr,__FUNCTION__": [%d] Test app does not support createing bitmap from cursor with masked color. dumping raw cursor data.\n", m_dwAdapterIdx);
        sprintf(filename2, "Cursor_Flag4_%d_adapter%d.txt", m_uFrameNo, m_dwAdapterIdx);
        FILE *cursorfile = fopen(filename2, "wb");
        memset(filename2, 0, 256);
        fwrite(pParams->pBits, 64*64*4, 1, cursorfile);
        fclose(cursorfile);
    }
    else
    {
        fprintf(stderr,__FUNCTION__": [%d] Unable to create cursor using captured data\n", m_dwAdapterIdx);
    }
}

void Grabber::ComputeResult()
{
#if 0
    char filename1[256];
    char filename2[256];
    char command[512];
    for (unsigned int i = 1; i <= m_uFrameNo; i++)
    {
        memset(filename1, 0, 256);
        memset(filename2, 0, 256);
        memset(command, 0, 256);
 
        sprintf(filename1, "Test_Cursors_OS_%d.bmp", i);      
        sprintf(filename2, "Test_Cursors_NV_%d.bmp", i);

        sprintf(command, "fc /b %s %s", filename1, filename2);
        system((const char *)command);
    }
#endif
}
