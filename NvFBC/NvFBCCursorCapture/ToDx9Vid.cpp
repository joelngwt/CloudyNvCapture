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
#include <NvFBC/nvFBCToDx9Vid.h>
#include "Grabber.h"
#include "bitmap.h"

void ToDX9VidGrabber::release()
{
    SAFE_RELEASE_2(m_pFBC,NvFBCToDx9VidRelease);
}

bool ToDX9VidGrabber::init()
{ 
    if (Grabber::init())
    {
        m_pFBC = (NvFBCToDx9Vid *)m_pvFBC;
        if (SUCCEEDED(m_pD3D9Device->CreateOffscreenPlainSurface(1280, 720, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pSurf, NULL)))
        {
            return true;
        }
        return false;
    }
    return false;
}

bool ToDX9VidGrabber::setup()
{
    if (!m_pFBC)
    {
        return false;
    }

    NVFBC_TODX9VID_OUT_BUF out = {0};
    out.pPrimary = m_pSurf;

    NVFBC_TODX9VID_SETUP_PARAMS fbcSetupParams = { 0 };
    fbcSetupParams.dwVersion = NVFBC_TODX9VID_SETUP_PARAMS_VER;
    fbcSetupParams.eMode = NVFBC_TODX9VID_ARGB;
    fbcSetupParams.bWithHWCursor = TRUE;
    fbcSetupParams.bEnableSeparateCursorCapture = TRUE;
    fbcSetupParams.bDiffMap = FALSE;
    fbcSetupParams.dwNumBuffers = 1;
    fbcSetupParams.ppBuffer = &out;
    fbcSetupParams.ppDiffMap = NULL;
    NVFBCRESULT status = m_pFBC->NvFBCToDx9VidSetUp(&fbcSetupParams);

    m_hCursorEvent = (HANDLE)fbcSetupParams.hCursorCaptureEvent;
    return (status == NVFBC_SUCCESS) ? true : false;
}

bool ToDX9VidGrabber::grab()
{
    char outName[256] = {0};
    FILE *outputFile = NULL;
    NVFBCRESULT status = NVFBC_SUCCESS;

    if (!m_pFBC)
    {
        return false;
    }

    sprintf(outName, "NVFBC_ToDx9Vid_%d_adapter%d.bmp", m_uFrameNo, getAdapterIdx());
    NVFBC_TODX9VID_GRAB_FRAME_PARAMS fbcGrabParams = {0};
    NvFBCFrameGrabInfo grabInfo = {0};
    fbcGrabParams.dwVersion = NVFBC_TODX9VID_GRAB_FRAME_PARAMS_VER;
    fbcGrabParams.dwFlags = NVFBC_TODX9VID_NOWAIT;
    fbcGrabParams.eGMode = NVFBC_TODX9VID_SOURCEMODE_SCALE;
    fbcGrabParams.dwTargetWidth = 1280;
    fbcGrabParams.dwTargetHeight= 720;
    fbcGrabParams.dwBufferIdx = 0;
    fbcGrabParams.pNvFBCFrameGrabInfo = &grabInfo;

    status = m_pFBC->NvFBCToDx9VidGrabFrame(&fbcGrabParams);
    if (status == NVFBC_SUCCESS)
    {
        D3DLOCKED_RECT lock = {0};
        m_pSurf->LockRect(&lock, NULL, 0);
        SaveARGB(outName, (BYTE *)lock.pBits, grabInfo.dwWidth, grabInfo.dwHeight, grabInfo.dwBufferWidth);
        m_pSurf->UnlockRect();
        fprintf (stderr, "Grab succeeded. Wrote %s as RGB.\n", outName);
    }
    else
    {
        fprintf(stderr, "Grab Failed\n");
    }
    return false;
}

bool ToDX9VidGrabber::cursorCapture()
{
    NVFBC_CURSOR_CAPTURE_PARAMS cursorParams;
    cursorParams.dwVersion = NVFBC_CURSOR_CAPTURE_PARAMS_VER;

    NVFBCRESULT status = NVFBC_SUCCESS;

    if (!m_pFBC)
    {
        return false;
    }

    m_uFrameNo++;
    status = m_pFBC->NvFBCToDx9VidCursorCapture(&cursorParams);
    if (status != NVFBC_SUCCESS)
    {
        fprintf(stderr, "Grab Failed\n");
    }
    CompareCursor(&cursorParams);

    return true;
}