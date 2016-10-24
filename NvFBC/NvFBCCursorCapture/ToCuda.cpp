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

#include <cuda.h>
#include "helper_cuda_drvapi.h"
#include <NvFBC/nvFBCCuda.h>

#include "Grabber.h"
#include "bitmap.h"

#define OUTPUT_WIDTH 1280
#define OUTPUT_HEIGHT 720
#define FILENAME "NVFBCTest.h264"

class cuCtxAutoLock
{
private:
    CUcontext ctxOrig;
    cuCtxAutoLock ()
        : ctxOrig (NULL)
    {
    }
public:
    cuCtxAutoLock (CUcontext ctx)
        : ctxOrig (NULL)
    {
        cuCtxPopCurrent(&ctxOrig);
        cuCtxPushCurrent(ctx);
    }
    ~cuCtxAutoLock ()
    {
        CUcontext ctxTemp = NULL;
        cuCtxPopCurrent (&ctxTemp);
        if (ctxOrig)
        {
            cuCtxPushCurrent (ctxOrig);
        }
    }
};

void ToCudaGrabber::release ()
{
    //! Cleanup
    {
        cuCtxAutoLock lock (cudaCtx);
        if (pCPUBuffer)
        {
            checkCudaErrors (cuMemFreeHost (pCPUBuffer));
        }

        if (pDevGrabBuffer)
        {
            checkCudaErrors (cuMemFree (pDevGrabBuffer));
        }
        SAFE_RELEASE_2 (m_pFBC, NvFBCCudaRelease);
    }

    //! Release the Cuda context that we had passed to NvFBC (if any)
    //! This must be done in the same order as demonstrated here.
    checkCudaErrors (cuCtxDestroy (cudaCtx));
}

bool ToCudaGrabber::init ()
{
    NvFBCCreateParams createParams = { 0 };
    createParams.dwVersion = NVFBC_CREATE_PARAMS_VER;

    NvFBCStatusEx status = { 0 };
    status.dwVersion = NVFBC_STATUS_VER;

    checkCudaErrors (cuInit (0));

    if (!lib.load ())
    {
        fprintf (stderr, __FUNCTION__": Failed to load NVFBC.\n", getAdapterIdx ());
        return false;
    }

    if (!NVFBC_SUCCESS == lib.getStatus (&status))
    {
        fprintf (stderr, __FUNCTION__": Failed to check NVFBC status. Quit.\n", getAdapterIdx ());
        return false;
    }

    if (status.bCurrentlyCapturing)
    {
        fprintf (stderr, __FUNCTION__": NVFBC already capturing for adapter %d.\n", getAdapterIdx ());
        return false;
    }
    
    if (!status.bIsCapturePossible)
    {
        fprintf (stderr, __FUNCTION__": NVFBC not enabled.\n", getAdapterIdx ());
        return false;
    }

    if (FAILED (InitD3D9 ()))
    {
        fprintf (stderr, __FUNCTION__": Failed to init d3d9 for adapter %d.\n", getAdapterIdx ());
        return false;
    }
      
    checkCudaErrors (cuD3D9CtxCreate (&cudaCtx, &cudaDevice, 0, m_pD3D9Device));
    if (!cudaCtx)
    {
        fprintf (stderr, __FUNCTION__": [%d] : Failed to create CUDA context.\n", getAdapterIdx ());
        return false;
    }

    // Scope for cuda ctx lock
    {
        cuCtxAutoLock lock (cudaCtx);

        createParams.dwInterfaceType = dwFBCType;
        createParams.dwAdapterIdx = getAdapterIdx ();
        createParams.pDevice = m_pD3D9Device;
        createParams.cudaCtx = cudaCtx;

        if (NVFBC_SUCCESS != lib.createEx (&createParams))
        {
            fprintf (stderr, __FUNCTION__": [%d] : Failed to create NvFBCCuda instance.\n", getAdapterIdx ());
            return false;
        }
        m_pvFBC = createParams.pNvFBC;
        m_dwMaxWidth = createParams.dwMaxDisplayWidth;
        m_dwMaxHeight = createParams.dwMaxDisplayHeight;
        m_pFBC = (NvFBCCuda *)m_pvFBC;
        fprintf (stderr, __FUNCTION__": [%d] : Created NVFBC session instance.\n", getAdapterIdx ());
    }

    return true;
}

bool ToCudaGrabber::setup ()
{
    if (!m_pFBC)
    {
        fprintf (stderr, __FUNCTION__": [%d] : NVFBC session not created.\n", getAdapterIdx ());
        return false;
    }

    cuCtxAutoLock lock (cudaCtx);
    NVFBCRESULT fbcRes = m_pFBC->NvFBCCudaGetMaxBufferSize (&m_dwMaxBufferSize);
    if (fbcRes != NVFBC_SUCCESS && m_dwMaxBufferSize)
    {
        fprintf (stderr, __FUNCTION__": [%d] : Failed to query max buffer size. Error %d\n", getAdapterIdx (), fbcRes);
        return false;
    }

    //! Allocate memory on the CUDA device to store the framebuffer. 
    checkCudaErrors (cuMemAlloc (&pDevGrabBuffer, m_dwMaxBufferSize));
    checkCudaErrors (cuMemAllocHost (&pCPUBuffer, m_dwMaxBufferSize));

    NVFBC_CUDA_SETUP_PARAMS params = { 0 };
    params.dwVersion = NVFBC_CUDA_SETUP_PARAMS_VER;
    params.bEnableSeparateCursorCapture = 1;
    fbcRes = m_pFBC->NvFBCCudaSetup (&params);
    if (NVFBC_SUCCESS != fbcRes)
    {
        fprintf (stderr, __FUNCTION__"[%d] : failed to setup cursor capture. Error %d\n", getAdapterIdx(), fbcRes);
        return false;
    }
    m_hCursorEvent = (HANDLE)params.hCursorCaptureEvent;
    if (!m_hCursorEvent)
    {
        fprintf (stderr, __FUNCTION__"[%d] : NULL hEvent returned by NVFBC\n", getAdapterIdx ());
        return false;
    }
    return true;
}

bool ToCudaGrabber::grab ()
{
    NVFBCRESULT fbcRes = NVFBC_SUCCESS;
    if (!m_pFBC)
    {
        fprintf (stderr, __FUNCTION__": [%d] : NVFBC session not created.\n", getAdapterIdx ());
        return false;
    }

    cuCtxAutoLock lock (cudaCtx);
    NvFBCFrameGrabInfo grabInfo = { 0 };
    NVFBC_CUDA_GRAB_FRAME_PARAMS fbcCudaGrabParams = { 0 };
    fbcCudaGrabParams.dwVersion = NVFBC_CUDA_GRAB_FRAME_PARAMS_VER;
    fbcCudaGrabParams.pCUDADeviceBuffer = (void *)pDevGrabBuffer;
    fbcCudaGrabParams.pNvFBCFrameGrabInfo = &grabInfo;
    fbcCudaGrabParams.dwFlags = NVFBC_TOCUDA_NOWAIT | NVFBC_TOCUDA_WITH_HWCURSOR;

    fbcRes = m_pFBC->NvFBCCudaGrabFrame (&fbcCudaGrabParams);
    if (fbcRes != NVFBC_SUCCESS)
    {
        fprintf (stderr, __FUNCTION__": [%d] : NvFBCCudaGrabFrame failed with error: %d.\n", getAdapterIdx (), fbcRes);
        return false;
    }

    checkCudaErrors (cuMemcpyDtoH (pCPUBuffer, pDevGrabBuffer, m_dwMaxBufferSize));
    char outName[256] = { 0 };
    sprintf (outName, "NVFBC_Cuda_%d_adapter%d.bmp", m_uFrameNo, getAdapterIdx());
    SaveARGB (outName, (BYTE *)pCPUBuffer, grabInfo.dwWidth, grabInfo.dwHeight, grabInfo.dwBufferWidth);
    return true;
}

bool ToCudaGrabber::cursorCapture()
{
    NVFBC_CURSOR_CAPTURE_PARAMS cursorParams;
    cursorParams.dwVersion = NVFBC_CURSOR_CAPTURE_PARAMS_VER;

    NVFBCRESULT status = NVFBC_SUCCESS;

    if (!m_pFBC)
    {
        fprintf (stderr, __FUNCTION__": [%d] : NVFBC session not created.\n", getAdapterIdx ());
        return false;
    }
    cuCtxAutoLock lock (cudaCtx);
    m_uFrameNo++;
    status = m_pFBC->NvFBCCudaCursorCapture(&cursorParams);
    if (status != NVFBC_SUCCESS)
    {
        fprintf(stderr, __FUNCTION__": [%d]:Cursor Grab Failed\n", getAdapterIdx());
        return false;
    }
    CompareCursor(&cursorParams);

    return true;
}