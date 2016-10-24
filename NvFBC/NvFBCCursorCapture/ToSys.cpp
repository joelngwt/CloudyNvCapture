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
#include "Grabber.h"
#include "bitmap.h"

void ToSysGrabber::release()
{
    SAFE_RELEASE_2(m_pFBC,NvFBCToSysRelease);
}

bool ToSysGrabber::init() 
{ 
    if (Grabber::init()) 
    { 
        m_pFBC = (NvFBCToSys *)m_pvFBC; 
        return true;
    } 
    return false; 
}

bool ToSysGrabber::setup()
{
    if (!m_pFBC)
    {
        return false;
    }

    NVFBC_TOSYS_SETUP_PARAMS fbcSysSetupParams = { 0 };
    fbcSysSetupParams.dwVersion = NVFBC_TOSYS_SETUP_PARAMS_VER;
    fbcSysSetupParams.eMode = NVFBC_TOSYS_ARGB;
    fbcSysSetupParams.bWithHWCursor = TRUE;
    fbcSysSetupParams.bEnableSeparateCursorCapture = TRUE;
    fbcSysSetupParams.bDiffMap = FALSE;
    fbcSysSetupParams.ppBuffer = (void **)&m_pSysMemBuf;
    fbcSysSetupParams.ppDiffMap = NULL;

    NVFBCRESULT status = m_pFBC->NvFBCToSysSetUp(&fbcSysSetupParams);
    m_hCursorEvent = (HANDLE)fbcSysSetupParams.hCursorCaptureEvent;
    return (status == NVFBC_SUCCESS) ? true : false;
}

bool ToSysGrabber::grab()
{
    if (!m_pFBC)
    {
        return false;
    }

    char outName[256] = {0};
    FILE *outputFile = NULL;
    NVFBCRESULT status = NVFBC_SUCCESS;

    sprintf(outName, "NVFBC_ToSys_%d.bmp", m_uFrameNo);
    NVFBC_TOSYS_GRAB_FRAME_PARAMS fbcSysGrabParams = {0};
    NvFBCFrameGrabInfo grabInfo = {0};
    fbcSysGrabParams.dwVersion = NVFBC_TOSYS_GRAB_FRAME_PARAMS_VER;
    fbcSysGrabParams.dwFlags = NVFBC_TOSYS_NOWAIT;
    fbcSysGrabParams.pNvFBCFrameGrabInfo = &grabInfo;

    status = m_pFBC->NvFBCToSysGrabFrame(&fbcSysGrabParams);
    if (status == NVFBC_SUCCESS)
    {
        SaveARGB(outName, (BYTE *)m_pSysMemBuf, grabInfo.dwWidth, grabInfo.dwHeight, grabInfo.dwBufferWidth);
        fprintf (stderr, "Grab succeeded. Wrote %s as RGB.\n", outName);
    }
    else
    {
        fprintf(stderr, "Grab Failed\n");
    }
    return false;
}

bool ToSysGrabber::cursorCapture()
{
    if (!m_pFBC)
    {
        return false;
    }
    NVFBC_CURSOR_CAPTURE_PARAMS cursorParams;
    cursorParams.dwVersion = NVFBC_CURSOR_CAPTURE_PARAMS_VER;

    NVFBCRESULT status = NVFBC_SUCCESS;
    m_uFrameNo++;
    status = m_pFBC->NvFBCToSysCursorCapture(&cursorParams);
    if (status != NVFBC_SUCCESS)
    {
        fprintf(stderr, "Grab Failed\n");
    }
    CompareCursor(&cursorParams);

    return true;
}