/*!
 * \brief
 * Demonstrates using the NvFBCToCuda interface to copy the desktop
 * into a CUDA buffer.
 * 
 * \file
 * This sample demonstrates using the NvFBCToCuda interface to copy the desktop 
 * into a CUDA buffer.
 * Encoder.h: Declares the Encoder class, this class is simply a wrapper around 
 * the CUDA video encoder API.
 * Encoder.cpp: Defines the Encoder class, wraps up the details when using the 
 * CUDA video encoder so they don't detract from the NvFBCCuda example.
 * NvFBCCuda.cpp: Demonstrates usage of the NvFBCCuda interface.  Specifically it 
 * covers loading the NvFBC.dll, loading the NvFBC function pointers, creating an 
 * instance of NvFBCCuda and using NvFBCCuda to copy the frame buffer into 
 * a CUDA device pointer.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <windows.h>
#include <d3d9.h>

//#include <atltime.h>
#include <stdio.h> 

#include <string>

#include <cuda.h>
#include <cudaD3D9.h>

#include <Timer.h>

#include <NvFBCLibrary.h>
#include <NvFBC/nvFBCCuda.h>

#include <cuda_runtime_api.h>
#include <helper_cuda.h>
#include <helper_string.h>

#include "Bitmap.h"

// Function used to launch the CUDA post-processing
extern "C" cudaError launch_CudaProcess(int w, int h, CUdeviceptr pImage, CUdeviceptr pHostMappedResult);

#define FRAME_COUNT 1

IDirect3D9Ex *g_pD3D = NULL;
IDirect3DDevice9Ex *g_pD3DDevice = NULL;
//! Command line arguments
typedef struct
{
    int         iFrameCnt; // Number of frames to dump
    std::string sBaseName; // Basename for the output file
    bool        bUseD3D9;  // Demonstrate D3D9 interop
}AppArguments;

void printHelp()
{
    printf("Usage NvFBCCUDASimple: [-frames framecnt] [-output filename]\n");
    printf("  -d3d9                 Demonstrate D3D9 interop.\n");
    printf("  -frames framecnt      The number of frames to dump.\n");
    printf("  -output filename      The base name for output stream.\n");
}

bool parseArgs(int argc, char **argv, AppArguments &args)
{
    args.iFrameCnt = FRAME_COUNT;
    args.sBaseName = "NvFBCCudaSimple";

    for(int cnt = 1; cnt < argc; ++cnt)
    {
        if(0 == STRCASECMP(argv[cnt], "-d3d9"))
        {
            args.bUseD3D9 = true;
        }
        else if(0 == STRCASECMP(argv[cnt], "-frames"))
        {
            ++cnt;

            if(cnt < argc)
            {
                args.iFrameCnt = atoi(argv[cnt]);
            }
            else
            {
                printf("Missing frame count argument\n");
                printHelp();
                return false;
            }
        }
        else if(0 == STRCASECMP(argv[cnt], "-output"))
        {
            ++cnt;

            if(cnt < argc)
            {
                args.sBaseName = argv[cnt];
            }
            else
            {
                printf("Missing output argument\n");
                printHelp();
                return false;
            }
        }
        else
        {
            printf("Unexpected argument: %s\n", argv[cnt]);
            printHelp();
            return false;
        }
    }

    return true;
}

/*! 
 * Creates the D3D9 Device and get render target.
 */
HRESULT InitD3D()
{
    //! Create the D3D object.
    if( FAILED(Direct3DCreate9Ex( D3D_SDK_VERSION, &g_pD3D) ) )
        return E_FAIL;

    //! Set up the structure used to create the D3DDevice. 
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof( d3dpp ) );
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth  = 720;
    d3dpp.BackBufferHeight = 480;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;  
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    //! Create the D3DDevice
    if( FAILED( g_pD3D->CreateDeviceEx( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetDesktopWindow(),
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &d3dpp, NULL, &g_pD3DDevice ) ) )
    {
        return E_FAIL;
    }
    return S_OK;
}

/*!
 * Main program
 */
int main(int argc, char *argv[])
{
    NvFBCLibrary nvfbc;
    NvFBCCuda *nvfbcCuda = NULL;
    NVFBCRESULT fbcRes = NVFBC_SUCCESS;

    DWORD currentWidth = 0, currentHeight = 0;
    DWORD maxBufferSize = -1;

    NvFBCFrameGrabInfo frameGrabInfo;
    AppArguments args = {0};
    char buf[1024] = {0};

    CUcontext cudaCtx = NULL;
    CUdevice  cudaDevice = NULL;
    CUdeviceptr pDevGrabBuffer = NULL;
    void * pCPUBuffer = NULL;
	
    if(!parseArgs(argc, argv, args))
        return -1;
    
    //! Load the nvfbc Library
    if(!nvfbc.load())
    {
        fprintf(stderr, "Unable to load the NvFBC library\n");
        return -1;
    }

    checkCudaErrors(cuInit(0));
    if (args.bUseD3D9)
    {
        InitD3D();
        checkCudaErrors(cuD3D9CtxCreate(&cudaCtx, &cudaDevice, 0, g_pD3DDevice));
    }

    NvFBCCreateParams createParams;
    memset(&createParams, 0, sizeof(createParams));
    createParams.dwVersion = NVFBC_CREATE_PARAMS_VER;
    createParams.dwInterfaceType = NVFBC_SHARED_CUDA;
    if (args.bUseD3D9)
    {
        createParams.pDevice = (void *)g_pD3DDevice;
        createParams.cudaCtx = (void *)cudaCtx;
    }

    //! Create an instance of the NvFBCCuda class, the first argument specifies the frame buffer
    fbcRes = nvfbc.createEx(&createParams);

    nvfbcCuda = (NvFBCCuda *)createParams.pNvFBC;
    if (fbcRes != NVFBC_SUCCESS || nvfbcCuda == NULL)
    {
        fprintf(stderr, "Failed to create an instance of NvFBCCuda\n");
        return -1;
    }

    //! Get the max buffer size from NvFBCCuda
    nvfbcCuda->NvFBCCudaGetMaxBufferSize(&maxBufferSize);
    
    if (!args.bUseD3D9)
    {
        //! Get the cuda context NvFBC is working on
        checkCudaErrors(cuCtxPopCurrent(&cudaCtx));
        checkCudaErrors(cuCtxPushCurrent(cudaCtx));
    }

    //! Allocate memory on the CUDA device to store the framebuffer. 
    checkCudaErrors(cuMemAlloc(&pDevGrabBuffer, maxBufferSize));
    checkCudaErrors(cuMemAllocHost(&pCPUBuffer, maxBufferSize));

    for(int frameCnt = 0; frameCnt < args.iFrameCnt; ++frameCnt)
    {
        NVFBCRESULT fbcRes = NVFBC_SUCCESS;
        {            
            NVFBC_CUDA_GRAB_FRAME_PARAMS fbcCudaGrabParams = {0};
            fbcCudaGrabParams.dwVersion = NVFBC_CUDA_GRAB_FRAME_PARAMS_VER;
            fbcCudaGrabParams.pCUDADeviceBuffer = (void *)pDevGrabBuffer;
            fbcCudaGrabParams.pNvFBCFrameGrabInfo = &frameGrabInfo;
            fbcCudaGrabParams.dwFlags = NVFBC_TOCUDA_NOWAIT;
            
            fbcRes = nvfbcCuda->NvFBCCudaGrabFrame(&fbcCudaGrabParams);

        }

        if(fbcRes == NVFBC_SUCCESS)
        {
            printf("grab ok\n");
            checkCudaErrors(cuMemcpyDtoH(pCPUBuffer, pDevGrabBuffer, maxBufferSize));
            sprintf(buf, ".\\%s_%d.bmp", args.sBaseName.c_str(), frameCnt);
            SaveARGB(buf, (BYTE *)pCPUBuffer, frameGrabInfo.dwWidth, frameGrabInfo.dwHeight, frameGrabInfo.dwBufferWidth);
        }
        else
        {
            fprintf(stderr, "Grab frame failed. NvFBCCudaGrabFrame returned: %d\n", fbcRes );
            return -1;
        }
    }


    //! Cleanup
    if (pCPUBuffer)
    {
        checkCudaErrors(cuMemFreeHost(pCPUBuffer));
    }
    if (pDevGrabBuffer)
    {
        checkCudaErrors(cuMemFree(pDevGrabBuffer));
    }
    
    //! Release the NvFBCCuda instance
    nvfbcCuda->NvFBCCudaRelease();

    if (args.bUseD3D9)
    {
        //! Release the Cuda context that we had passed to NvFBC (if any)
        //! This must be done in the same order as demonstrated here.
        checkCudaErrors(cuCtxDestroy(cudaCtx));
    }

    if (g_pD3DDevice)
    {
        g_pD3DDevice->Release();
        g_pD3DDevice = NULL;
    }

    if (g_pD3D)
    {
        g_pD3D->Release();
        g_pD3D = NULL;
    }

    return 0;
}