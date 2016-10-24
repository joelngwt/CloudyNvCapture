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
#include <NvFBC/nvFBCHWEnc.h>
#include "Grabber.h"
#include "bitmap.h"

#define BITSTREAM_SIZE 2*1024*1024
#define AVG_BITRATE 5000000

bool ToHWEncGrabber::init()
{
    if (Grabber::init()) 
    { 
        m_pFBC = (INvFBCHWEncoder *)m_pvFBC;
        m_pBitStreamBuf = malloc(BITSTREAM_SIZE);
        m_pFout = fopen("NVFBC_ToDx9Vid.h264", "wb");
        return true;
    } 
    return false; 
}

void ToHWEncGrabber::release()
{ 
    SAFE_RELEASE_2(m_pFBC,NvFBCHWEncRelease); 
    free(m_pBitStreamBuf);
}

bool ToHWEncGrabber::setup()
{
    if (!m_pFBC)
    {
        return false;
    }

    NVFBC_HW_ENC_SETUP_PARAMS fbcSetupParams = { 0 };
    fbcSetupParams.dwVersion = NVFBC_HW_ENC_SETUP_PARAMS_VER;
    fbcSetupParams.dwBSMaxSize = BITSTREAM_SIZE;
    fbcSetupParams.bWithHWCursor = TRUE;
    fbcSetupParams.bEnableSeparateCursorCapture = TRUE;

    // Some default encoder config params
    fbcSetupParams.EncodeConfig.dwVersion = NV_HW_ENC_CONFIG_PARAMS_VER;
    fbcSetupParams.EncodeConfig.eCodec = NV_HW_ENC_H264;
    fbcSetupParams.EncodeConfig.dwProfile = 77;
    fbcSetupParams.EncodeConfig.dwFrameRateNum = 30;
    fbcSetupParams.EncodeConfig.dwFrameRateDen = 1; // Set the target frame rate at 30
    fbcSetupParams.EncodeConfig.dwAvgBitRate = AVG_BITRATE;
    fbcSetupParams.EncodeConfig.dwPeakBitRate = (NvU32)(AVG_BITRATE * 1.50); // Set the peak bitrate to 150% of the average
    fbcSetupParams.EncodeConfig.dwGOPLength = 10; // The I-Frame frequency
    fbcSetupParams.EncodeConfig.eRateControl = NV_HW_ENC_PARAMS_RC_VBR; // Set the rate control
    fbcSetupParams.EncodeConfig.ePresetConfig = NV_HW_ENC_PRESET_LOW_LATENCY_HQ;
    fbcSetupParams.EncodeConfig.dwQP = 26; // Quantization parameter, between 0 and 51 

    NVFBCRESULT status = m_pFBC->NvFBCHWEncSetUp(&fbcSetupParams);

    m_hCursorEvent = (HANDLE)fbcSetupParams.hCursorCaptureEvent;
    return (status == NVFBC_SUCCESS) ? true : false;
}

bool ToHWEncGrabber::grab()
{
    if (!m_pFBC)
    {
        return false;
    }

    NVFBCRESULT status = NVFBC_SUCCESS;

    NVFBC_HW_ENC_GRAB_FRAME_PARAMS fbcGrabParams = {0};
    fbcGrabParams.dwVersion = NVFBC_HW_ENC_GRAB_FRAME_PARAMS_VER;
    fbcGrabParams.dwFlags = NVFBC_HW_ENC_NOWAIT;
    //fbcGrabParams.eGMode = NVFBC_HW_ENC_SOURCEMODE_SCALE;
    //fbcGrabParams.dwTargetWidth = 1280;
    //fbcGrabParams.dwTargetHeight= 720;
    fbcGrabParams.pBitStreamBuffer = (NvU8 *)m_pBitStreamBuf;
    
    status = m_pFBC->NvFBCHWEncGrabFrame(&fbcGrabParams);
    if (status == NVFBC_SUCCESS)
    {
        fprintf (stderr, "Grab succeeded. Encoded bitstream size %d bytes.\n", fbcGrabParams.GetBitStreamParams.dwByteSize);
        fwrite(m_pBitStreamBuf, fbcGrabParams.GetBitStreamParams.dwByteSize, 1, m_pFout);
    }
    else
    {
        fprintf(stderr, "Grab Failed\n");
    }
    return false;
}

bool ToHWEncGrabber::cursorCapture()
{
    if (!m_pFBC)
    {
        return false;
    }

    NVFBC_CURSOR_CAPTURE_PARAMS cursorParams;
    cursorParams.dwVersion = NVFBC_CURSOR_CAPTURE_PARAMS_VER;

    NVFBCRESULT status = NVFBC_SUCCESS;
    m_uFrameNo++;
    status = m_pFBC->NvFBCHWEncCursorCapture(&cursorParams);
    if (status != NVFBC_SUCCESS)
    {
        fprintf(stderr, "Grab Failed\n");
    }
    CompareCursor(&cursorParams, FALSE);

    return true;
}