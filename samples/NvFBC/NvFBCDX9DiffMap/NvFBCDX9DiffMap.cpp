/*!
 * \brief
 * Demonstrates using the NvFBCToDx9 interface to capture DiffMaps of the desktop.
 * The results are copied to the host as a 16x16 diffmap.  This sample also generates
 * a downscaled copy of the frame buffer to a bitmap, and saves it to file.
 *
 * \file
 * This sample demonstrates using the NvFBCToDx9 interface for diffmaps and capture.
 *
 * Reader.h: Declares the Reader class, this class is simply a wrapper around
 * the NVENC reader API.
 *
 * Reader.cpp: Defines the Reader class, wraps up the details when using the
 * NVENC video reader so it don't detract from the NvFBCDX9 example.
 *
 * NvFBCDX9DiffMap.cpp: Demonstrates usage of the NvFBCToDX9 interface with the DiffMap 
 * capture API. It demonstrates how to load NvFBC.dll, initialize NvFBC function pointers,
 * create an instance of NvFBCDX9 and using NvFBCDX9 to extract a 16x16 diffmap from
 * the driver, and also capture a downscaled version of the desktop.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */


#include <windows.h>
#include <stdio.h> 
#include <string>
#include <math.h>

#include <Timer.h>
#include <assert.h>
#include <initguid.h>
#include <d3d9.h>

#include <NvFBCLibrary.h>
#include <NvFBC/NvFBCToDx9vid.h>
#include <helper_string.h>
#include "bitmap.h"

//! Number of frames to record.
#define FRAME_COUNT 120

//! Starting reader bitrate
#define READER_WIDTH 1280
#define READER_HEIGHT 720
const D3DFORMAT D3DFMT_NV12 = (D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2');

// Structure to store the command line arguments
struct AppArguments
{
    int   iFrameCnt; // Number of frames to grab
    int   iStartX; // Grab start x coord
    int   iStartY; // Grab start y coord
    int   iWidth; // Grab width
    int   iHeight; // Grab height
    bool  bHWCursor; // Grab hardware cursor
    NVFBCToDx9VidGrabMode       grabMode; // Grab mode
    NVFBCToDx9VidGrabFlags      grabFlags; // Grab flags
    NVFBCToDx9VidBufferFormat   eFormat;
    std::string                 sBaseName; // Output file name
};

void printHelp()
{
    printf("Usage: NvFBCDX9DiffMap [options]\n");
    printf("  -frames frameCnt     The number of frames to grab\n");
    printf("                       , this value defaults to one.  If\n");
    printf("                       the value is greater than one only\n");
    printf("                       the final frame is saved as a bitmap.\n");
    printf("  -scale width height  Scales the grabbed frame buffer\n");
    printf("                       to the provided width and height\n");
    printf("  -crop x y width height  Crops the grabbed frame buffer to\n");
    printf("                          the provided region\n");
    printf("  -grabCursor          Grabs the hardware cursor\n");
    printf("  -output              Output file name prefix\n");
    printf("  -nowait              Grab with the no wait flag\n");
    printf("  -format [ARGB|NV12|ARGB10]    default is ARGB\n");
}

bool parseArgs(int argc, char **argv, AppArguments &args)
{
    args.iFrameCnt = 16;
    args.iStartX = 0;
    args.iStartY = 0;
    args.iWidth = READER_WIDTH;
    args.iHeight = READER_HEIGHT;
    args.grabFlags = NVFBC_TODX9VID_NOFLAGS;
    args.bHWCursor = false;
    args.grabMode = NVFBC_TODX9VID_SOURCEMODE_SCALE;
    args.sBaseName = "NvFBCDX9DiffMap";
    args.eFormat = NVFBC_TODX9VID_ARGB;
    for (int cnt = 1; cnt < argc; ++cnt)
    {
        if (0 == _stricmp(argv[cnt], "-frames"))
        {
            ++cnt;

            if (cnt >= argc)
            {
                printf("Missing -frames option\n");
                printHelp();
                return false;
            }

            args.iFrameCnt = atoi(argv[cnt]);

            // Must grab at least one frame.
            if (args.iFrameCnt < 1)
                args.iFrameCnt = 1;
        }
        else if (0 == _stricmp(argv[cnt], "-scale"))
        {
            if ((cnt + 2) > argc)
            {
                printf("Missing -scale options\n");
                printHelp();
                return false;
            }

            args.iWidth = atoi(argv[cnt + 1]);
            args.iHeight = atoi(argv[cnt + 2]);
            args.grabMode = NVFBC_TODX9VID_SOURCEMODE_SCALE;

            cnt += 2;
        }
        else if (0 == _stricmp(argv[cnt], "-crop"))
        {
            if ((cnt + 4) >= argc)
            {
                printf("Missing -crop options\n");
                printHelp();
                return false;
            }

            args.iStartX = atoi(argv[cnt + 1]);
            args.iStartY = atoi(argv[cnt + 2]);
            args.iWidth = atoi(argv[cnt + 3]);
            args.iHeight = atoi(argv[cnt + 4]);
            args.grabMode = NVFBC_TODX9VID_SOURCEMODE_CROP;

            cnt += 4;
        }
        else if (0 == _stricmp(argv[cnt], "-grabCursor"))
        {
            args.bHWCursor = true;
        }
        else if (0 == _stricmp(argv[cnt], "-nowait"))
        {
            args.grabFlags = NVFBC_TODX9VID_NOWAIT;
        }
        else if (0 == _stricmp(argv[cnt], "-output"))
        {
            ++cnt;

            if (cnt >= argc)
            {
                printf("Missing -output option\n");
                printHelp();
                return false;
            }

            args.sBaseName = argv[cnt];
        }
        else if (0 == _stricmp(argv[cnt], "-format"))
        {
            ++cnt;

            if (cnt >= argc)
            {
                printf("Missing -output option\n");
                printHelp();
                return false;
            }
            if (0 == _stricmp(argv[cnt], "NV12"))
            {
                args.eFormat = NVFBC_TODX9VID_NV12;
            }
            else if (0 == _stricmp(argv[cnt], "ARGB"))
            {
                args.eFormat = NVFBC_TODX9VID_ARGB;
            }
            else
            {
                printf("Invalid format specified\n");
                printHelp();
                return false;
            }

        }
        else
        {
            printf("Unexpected argument %s\n", argv[cnt]);
            printHelp();
            return false;
        }
    }

	return true;
}

// DirectX resources
IDirect3D9Ex        *g_pD3DEx = NULL;
IDirect3DDevice9Ex  *g_pD3D9Device = NULL;
IDirect3DSurface9   *g_pD3D9Surf = NULL;

bool g_bNvFBCLibLoaded = false;

NvFBCToDx9Vid *NvFBCDX9 = NULL;

NvFBCLibrary *pNVFBCLib;

const int DIFF_MAP_BUF_SIZE = 4096;
const int NUM_DIFF_MAPS     = 16;
void *g_pDiffMap = NULL;

void Cleanup()
{
    //! Release the NvFBCDX9 instance
    if (NvFBCDX9)
    {
        NvFBCDX9->NvFBCToDx9VidRelease();
        NvFBCDX9 = NULL;
    }

    if (g_pD3D9Surf)
    {
        g_pD3D9Surf->Release();
        g_pD3D9Surf = NULL;
    }

    if (g_pD3D9Device)
    {
        g_pD3D9Device->Release();
        g_pD3D9Device = NULL;
    }

    if (g_pD3DEx)
    {
        g_pD3DEx->Release();
        g_pD3DEx = NULL;
    }

    if (g_bNvFBCLibLoaded)
    {
        pNVFBCLib->close();
    }

    if (pNVFBCLib)
    {
        delete pNVFBCLib;
        pNVFBCLib = NULL;
    }

    if (g_pDiffMap) {
        VirtualFree(g_pDiffMap, DIFF_MAP_BUF_SIZE, MEM_RELEASE);
    }
}

HRESULT InitD3D9(unsigned int deviceID)
{
    HRESULT hr = S_OK;
    D3DPRESENT_PARAMETERS d3dpp;
    D3DADAPTER_IDENTIFIER9 adapterId;
    unsigned int iAdapter = NULL;

    Direct3DCreate9Ex(D3D_SDK_VERSION, &g_pD3DEx);

    if (deviceID >= g_pD3DEx->GetAdapterCount())
    {
        printf("Error: (deviceID=%d) is not a valid GPU device. Headless video devices will not be detected.  <<\n\n", deviceID);
        return E_FAIL;
    }

    hr = g_pD3DEx->GetAdapterIdentifier(deviceID, 0, &adapterId);
    if (hr != S_OK)
    {
        printf("Error: (deviceID=%d) is not a valid GPU device. <<\n\n", deviceID);
        return E_FAIL;
    }

    // Create the Direct3D9 device and the swap chain. In this example, the swap
    // chain is the same size as the current display mode. The format is RGB-32.
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = true;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpp.BackBufferWidth  = 800;
    d3dpp.BackBufferHeight = 600;
    d3dpp.BackBufferCount = 1;
    d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    d3dpp.Flags = D3DPRESENTFLAG_VIDEO;
    DWORD dwBehaviorFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING;
    hr = g_pD3DEx->CreateDeviceEx(
        deviceID,
        D3DDEVTYPE_HAL,
        NULL,
        dwBehaviorFlags,
        &d3dpp,
        NULL,
        &g_pD3D9Device);

    assert(SUCCEEDED(hr));

    return hr;
}

HRESULT InitD3D9Surfaces(int nWidth, int nHeight)
{
    HRESULT hr = E_FAIL;
    if (g_pD3D9Device)
    {
        hr = g_pD3D9Device->CreateOffscreenPlainSurface(nWidth, nHeight, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_pD3D9Surf, NULL);
        if (FAILED(hr))
        {
            fprintf(stderr, "Failed to create D3D9 surfaces for output. Error 0x%08x\n", hr);
        }
    }
    return hr;
}

void DumpDiffMap(BYTE *pDiffMap, int nSize, FILE *fp) 
{
    int xyDim = sqrt((float)nSize);
    for (int y = 0; y < xyDim; y++) {
        for (int x = 0; x < xyDim; x++) {
            printf("%02x ", pDiffMap[y * 16 + x]);
        }
        printf("\n");
    }
    printf("\n");

    for (int y = 0; y < xyDim; y++) {
        for (int x = 0; x < xyDim; x++) {
            fprintf(fp, "%02x ", pDiffMap[y * 16 + x]);
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
}

void DumpOffsceenSurface(IDirect3DDevice9 *pDevice, IDirect3DSurface9 *pOffscreenSurface, const char *filename) 
{
    HRESULT hr = E_FAIL;
    D3DSURFACE_DESC desc;

    hr = pOffscreenSurface->GetDesc(&desc);
    if (hr != S_OK)
    {
        printf("pOffscreenSurface->GetDesc() failed!\n");
        return;
    }
    D3DLOCKED_RECT lockedRect;

    hr = pOffscreenSurface->LockRect(&lockedRect, NULL, 0);
    if (hr != S_OK)
    {
        printf("pOffscreenSurface->LockRect() failed!\n");
        return;
    }

    byte* pSurfaceData = (byte *)(new unsigned int[desc.Width * desc.Height]);
    byte* pTempOutput = pSurfaceData;
    byte* pTempSrc = (byte *)lockedRect.pBits;
    DOUBLE totalHeight = desc.Height;
    int    totalWidth = desc.Width*4;
    if (desc.Format == D3DFMT_NV12)
    {
        totalHeight = desc.Height*1.5;
        totalWidth  = desc.Width;
    }
    for (unsigned int i = 0; i < totalHeight; i++)
    {
        memcpy(pTempOutput, pTempSrc, totalWidth);
        pTempOutput += totalWidth;
        pTempSrc += lockedRect.Pitch;
    }
    pOffscreenSurface->UnlockRect();

    switch (desc.Format)
    { 
    case D3DFMT_A8R8G8B8:
        SaveARGB(filename, (BYTE *)pSurfaceData, desc.Width, desc.Height, desc.Width);
        break;
    case D3DFMT_NV12:
        SaveNV12(filename, (BYTE *)pSurfaceData, desc.Width, desc.Height, desc.Width);
        break;
    default:
        printf("\nUnable to dump. Invalid Format\n");
    }
    delete[] pSurfaceData;
}

/*!
 * Main program
 */
 int main(int argc, char *argv[])
{
    AppArguments args;

    //! Timers
    Timer grabTimer;

     DWORD currentWidth = 0, currentHeight = 0;
     DWORD maxDisplayWidth = -1, maxDisplayHeight = -1;
     DWORD maxBufferSize = -1;

     //! DX9 resources
     NVFBC_TODX9VID_OUT_BUF NvFBC_OutBuf[1];

     if (!parseArgs(argc, argv, args))
     {
         fprintf(stderr, "Invalid params. Quitting\n");
         return -1;
     }

     //! Load the nvfbc Library
     pNVFBCLib = new NvFBCLibrary();
     if (!pNVFBCLib->load())
     {
         fprintf(stderr, "Unable to load the NvFBC library\n");
         return -1;
     }

     g_bNvFBCLibLoaded = true;
     if (!SUCCEEDED(InitD3D9(0)))
     {
         fprintf(stderr, "Unable to create a D3D9Ex Device\n");
         Cleanup();
         return -1;
     }

     if (!SUCCEEDED(InitD3D9Surfaces(args.iWidth, args.iHeight)))
     {
         fprintf(stderr, "Unable to create a D3D9Ex Device\n");
         Cleanup();
         return -1;
     }
     //! Create an instance of the NvFBCDX9 class, the first argument specifies the frame buffer
     NvFBCDX9 = (NvFBCToDx9Vid *)pNVFBCLib->create(NVFBC_TO_DX9_VID, &maxDisplayWidth, &maxDisplayHeight, 0, (void *)g_pD3D9Device);
     if (!NvFBCDX9)
     {
         fprintf(stderr, "Failed to create an instance of NvFBCToDx9Vid.  Please check the following requirements\n");
         fprintf(stderr, "1) You must be using R361 or newer driver with a Quadro or GRID capable product\n");
         fprintf(stderr, "2) Make sure to run \"NvFBCEnable -enable\" after a new driver installation\n");
         Cleanup();
         return -1;
     }

     fprintf(stderr, "NvFBCToDX9Vid CreateEx() success!\n");

     NvFBC_OutBuf[0].pPrimary = g_pD3D9Surf;

     g_pDiffMap = VirtualAlloc(NULL, DIFF_MAP_BUF_SIZE, MEM_COMMIT, PAGE_READWRITE);

     NVFBC_TODX9VID_SETUP_PARAMS DX9SetupParams = { 0 };
     DX9SetupParams.dwVersion = NVFBC_TODX9VID_SETUP_PARAMS_V2_VER;
     DX9SetupParams.bWithHWCursor = args.bHWCursor;
     DX9SetupParams.bStereoGrab = 0;
     DX9SetupParams.eMode = NVFBC_TODX9VID_ARGB;
     DX9SetupParams.dwNumBuffers = 1;
     DX9SetupParams.ppBuffer = NvFBC_OutBuf;
     DX9SetupParams.bDiffMap = TRUE;
     DX9SetupParams.dwDiffMapBuffSize = DIFF_MAP_BUF_SIZE;
     DX9SetupParams.ppDiffMap = (void **)&g_pDiffMap;

     if (NVFBC_SUCCESS != NvFBCDX9->NvFBCToDx9VidSetUp(&DX9SetupParams))
     {
         fprintf(stderr, "Failed when calling NvFBCDX9->NvFBCToDX9VidSetup()\n");
         Cleanup();
         return -1;
     }

     int nBytesDiffMap = (args.iWidth + 127) / 128 * (args.iHeight + 127) / 128;
     for (int frameCnt = 0; frameCnt < args.iFrameCnt; frameCnt++)
     {
         double grabTime;
         // Setup NvFBC the DX9 Video Grab Parameters
         NVFBC_TODX9VID_GRAB_FRAME_PARAMS fbcDX9GrabParams = { 0 };
         NvFBCFrameGrabInfo frameGrabInfo = { 0 };
         NVFBCRESULT fbcRes = NVFBC_SUCCESS;
         {
             fbcDX9GrabParams.dwVersion = NVFBC_TODX9VID_GRAB_FRAME_PARAMS_VER;
             fbcDX9GrabParams.dwFlags = args.grabFlags;
             fbcDX9GrabParams.eGMode = args.grabMode;
             fbcDX9GrabParams.dwTargetWidth = args.iWidth;
             fbcDX9GrabParams.dwTargetHeight = args.iHeight;
             fbcDX9GrabParams.dwStartX = args.iStartX;
             fbcDX9GrabParams.dwStartY = args.iStartY;
             fbcDX9GrabParams.pNvFBCFrameGrabInfo = &frameGrabInfo;

             grabTimer.reset();
             fbcRes = NvFBCDX9->NvFBCToDx9VidGrabFrame(&fbcDX9GrabParams);
             grabTime = grabTimer.now();
         }

         if (fbcRes == NVFBC_SUCCESS)
         {
             // First copy it to our array of diff maps, so we can compare against previous diff map runs
             printf("Capture %d: frame %d, grab time %.2f\n",
                 frameCnt, frameCnt, grabTime);

             for (int i = 0; i < DIFF_MAP_BUF_SIZE; i++)
             {
                 // check diffmap
                 if (((BYTE *)g_pDiffMap)[i])
                 {
                     FILE *fp = fopen((args.sBaseName + ".diff").c_str(), "at");
                     fprintf(fp, "g_DiffMap[%d] =\n", frameCnt);
                     DumpDiffMap((BYTE *)g_pDiffMap, DIFF_MAP_BUF_SIZE, fp);
                     fclose(fp);

                     // save to bitmap only when there's change
                     char buf[10];
                     DumpOffsceenSurface(g_pD3D9Device, g_pD3D9Surf, (args.sBaseName + "_" + _itoa(frameCnt, buf, 10) + ".bmp").c_str());

                     break;
                 }
             }
         }
         else
         {
             fprintf(stderr, "Grab frame failed. NvFBCDX9GrabFrame returned: %d\n", fbcRes);
             Cleanup();
             return -1;
         }
     }
     Cleanup();
     return 0;
 }
