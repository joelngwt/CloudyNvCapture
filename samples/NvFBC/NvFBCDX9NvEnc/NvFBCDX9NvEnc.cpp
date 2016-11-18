/*!
 * \brief
 * Demonstrates using the NvFBCToDx9 interface to copy the desktop
 * into a DX9 buffer and then send it to NVENC for hardware encode
 *
 * \file
 * This sample demonstrates using the NvFBCToDx9 interface to copy the desktop
 * into a DX9  buffer.  From the DX9 Buffer, we use VideoProcessBlt to transferred
 * to NVENC where the H.264 video encoder can encode the stream.
 *
 * Encoder.h: Declares the Encoder class, this class is simply a wrapper around
 * the NVENC encoder API.
 *
 * Encoder.cpp: Defines the Encoder class, wraps up the details when using the
 * NVENC video encoder so it don't detract from the NvFBCDX9 example.
 *
 * NvFBCDX9NvEnc.cpp: Demonstrates usage of the NvFBCToDX9 interface.
 * It demonstrates how to load vFBC.dll, initialize NvFBC function pointers,
 * create an instance of NvFBCDX9 and using NvFBCDX9 to copy the frame buffer
 * into a DX9 device pointer, where it was VideoProcessBlt, where is then passed
 * to NVENC to generate a H.264 video.
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

#include <Timer.h>
#include <assert.h>
#include <initguid.h>
#include <d3d9.h>

#include <NvFBCLibrary.h>
#include <NvFBC/NvFBCToDx9vid.h>
#include <helper_string.h>

#include "Encoder.h"
#include "bitmap.h"

//! Number of frames to record.
#define FRAME_COUNT 480

//! Starting encoder bitrate
#define ENCODER_BITRATE 8000000
#define ENCODER_WIDTH  1920
#define ENCODER_HEIGHT 1080

//! Command line arguments
typedef struct
{
    int         iFrameCnt; // Number of frames to encode
    int         iBitrate;  // Bitrate to use
	bool        bYUV444;   // Use 4:4:4 YUV encode
	int         iCodec;    // Codec format (H.264 or H.265)
	std::string sBaseName; // Basename for the output stream
}AppArguments;

char baseFileName[256];

FILE *GetOutputFile(std::string baseName, DWORD width, DWORD height, bool bHEVC)
{
    FILE *output = NULL;

	if (bHEVC)
	{
        sprintf(baseFileName, "%dx%d-%s.h265", width, height, baseName.c_str() );
	}
	else 
	{
        sprintf(baseFileName, "%dx%d-%s.h264", width, height, baseName.c_str() );
	}

    output = fopen(baseFileName, "wb");

    if (NULL == output)
    {
        fprintf(stderr, "Failed to open the file %s for writing.\n", baseFileName);
        return NULL;
    }

    return output;
}

void printHelp()
{
    printf("Usage NvFBCDX9NvEnc: [-bitrate bitrate] [-frames framecnt] [-output filename]\n");
    printf("  -bitrate bitrate      The bitrate to encode at.\n");
    printf("  -frames framecnt      The number of frames to encode.\n");
    printf("  -output filename      The base name for output stream.\n");
    printf("  -yuv444               The encoder will use 4:4:4 sampled YUV.\n");
	printf("  -codec  format        The Video Codec Format \"H264\" or \"HEVC\"");
}

bool parseArgs(int argc, char **argv, AppArguments &args)
{
    args.iBitrate   = ENCODER_BITRATE;
    args.iFrameCnt  = FRAME_COUNT;
    args.bYUV444    = false;
    args.iCodec = 0; // 0=H264, 1=HEVC
    args.sBaseName = "NvFBCDX9NvEnc";

    for (int cnt = 1; cnt < argc; ++cnt)
    {
        if (0 == STRCASECMP(argv[cnt], "-bitrate"))
        {
            ++cnt;

            if (cnt < argc)
            {
                args.iBitrate = atoi(argv[cnt]);
            }
            else
            {
                printf("Missing bitrate argument\n");
                printHelp();
                return false;
            }
        }
        else if (0 == STRCASECMP(argv[cnt], "-frames"))
        {
            ++cnt;

            if (cnt < argc)
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
        else if (0 == STRCASECMP(argv[cnt], "-output"))
        {
            ++cnt;

            if (cnt < argc)
            {
                args.sBaseName = argv[cnt];
            }
            else
            {
                printf("Missing output argument after -output\n");
                printHelp();
                return false;
            }
        }
        else if (0 == STRCASECMP(argv[cnt], "-yuv444"))
        {
            ++cnt;
            args.bYUV444 = true;
        }
        else if (0 == STRCASECMP(argv[cnt], "-codec"))
        {
            ++cnt;

            if (cnt < argc)
            {
                if (!STRCASECMP(argv[cnt], "H264"))
                {
                    args.iCodec = 0;
                }
                else if (!STRCASECMP(argv[cnt], "H265") ||
                    !STRCASECMP(argv[cnt], "HEVC"))
                {
                    args.iCodec = 1;
                }
                else
                {
                    printf("Invalid codec format, must be either H264 or H265\n");
                    printHelp();
                    return false;
                }
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

// DirectX resources
IDirect3D9Ex        *g_pD3DEx = NULL;
IDirect3DDevice9Ex  *g_pD3D9Device = NULL;
IDirect3DSurface9   *g_apD3D9Surf[MAX_BUF_QUEUE] = { NULL };
bool g_bEncoderInitDone = false;
bool g_bNvFBCLibLoaded = false;
FILE *fOut = NULL;
//! NVENC Encoder and DXVA2/D3D9 pre-processor for RGB2YUV CSC conversion
Encoder *g_pEncoder = NULL;

NvFBCToDx9Vid *NvFBCDX9 = NULL;

NvFBCLibrary *pNVFBCLib;

void Cleanup()
{
    if (g_bEncoderInitDone)
    {
        //! Terminate the encoder
        g_pEncoder->TearDown();
        delete g_pEncoder;
    }

    //! Close the output file
    if (NULL != fOut)
        fclose(fOut);

    //! Release the NvFBCDX9 instance
    if (NvFBCDX9)
    {
        NvFBCDX9->NvFBCToDx9VidRelease();
        NvFBCDX9 = NULL;
    }

    for (int i = 0; i < MAX_BUF_QUEUE; i++)
    {
        if (g_apD3D9Surf[i])
        {
            g_apD3D9Surf[i]->Release();
            g_apD3D9Surf[i] = NULL;
        }
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
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferWidth  = ENCODER_WIDTH;
    d3dpp.BackBufferHeight = ENCODER_HEIGHT;
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

HRESULT InitD3D9Surfaces()
{
    HRESULT hr = E_FAIL;
    if (g_pD3D9Device)
    {
        for (int i = 0; i < MAX_BUF_QUEUE; i++)
        {
            hr = g_pD3D9Device->CreateOffscreenPlainSurface(ENCODER_WIDTH, ENCODER_HEIGHT, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_apD3D9Surf[i], NULL);
            if (FAILED(hr))
            {
                fprintf(stderr, "Failed to create D3D9 surfaces for output. Error 0x%08x\n", hr);
            }
        }
    }
    return hr;
}

/*!
 * Main program
 */
 int main(int argc, char *argv[])
{
    AppArguments args;

    //! Timers
    Timer grabTimer;
    Timer frameTimer;

    DWORD currentWidth = 0, currentHeight = 0;
    DWORD maxDisplayWidth = -1, maxDisplayHeight = -1;
    DWORD maxBufferSize = -1;

    NvFBCFrameGrabInfo frameGrabInfo = { 0 };

    //! DX9 resources
    IDirect3DSurface9 *pYUVBuffer = NULL;
    NVFBC_TODX9VID_OUT_BUF NvFBC_OutBuf[MAX_BUF_QUEUE];

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

    if (!SUCCEEDED(InitD3D9Surfaces()))
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
        fprintf(stderr, "1) A driver R355 or newer with capable Tesla/Quadro/GRID capable product\n");
        fprintf(stderr, "2) Run \"NvFBCEnable -enable\" after a new driver installation\n");
        Cleanup();
        return -1;
    }

    fprintf(stderr, "NvFBCToDX9Vid CreateEx() success!\n");

    g_pEncoder = new Encoder();

    if (!g_pEncoder)
    {
        fprintf(stderr, "Failed to allocate encoder class object\n");
        Cleanup();
        return -1;
    }

    // Now we can initialize the NVENC encoder
    // IMPORTANT: NVFBC and NVENCodeAPI must share the same DX9 device
    if (S_OK != g_pEncoder->Init(g_pD3D9Device))
    {
        fprintf(stderr, "Failed to initialize NVENC video encoder\n");
        Cleanup();
        return -1;
    }

    g_bEncoderInitDone = true;

    for (unsigned int i = 0; i < MAX_BUF_QUEUE; i++)
    {
        NvFBC_OutBuf[i].pPrimary = g_apD3D9Surf[i];
    }

    NVFBC_TODX9VID_SETUP_PARAMS DX9SetupParams = {};
    DX9SetupParams.dwVersion     = NVFBC_TODX9VID_SETUP_PARAMS_V2_VER;
    DX9SetupParams.bWithHWCursor = 0;
    DX9SetupParams.bStereoGrab   = 0;
    DX9SetupParams.bDiffMap      = 0;
    DX9SetupParams.ppBuffer	     = NvFBC_OutBuf;
    DX9SetupParams.eMode         = NVFBC_TODX9VID_ARGB;
    DX9SetupParams.dwNumBuffers  = MAX_BUF_QUEUE;

    if (NVFBC_SUCCESS != NvFBCDX9->NvFBCToDx9VidSetUp(&DX9SetupParams))
    {
        fprintf(stderr, "Failed when calling NvFBCDX9->NvFBCToDX9VidSetup()\n");
        Cleanup();
        return -1;
    }

    frameTimer.reset();
    for (int frameCnt = 0; frameCnt < args.iFrameCnt; ++frameCnt)
    {
        unsigned int frameIDX = frameCnt % MAX_BUF_QUEUE;
        grabTimer.reset();

        // Setup NvFBC the DX9 Video Grab Parameters
        NVFBC_TODX9VID_GRAB_FRAME_PARAMS fbcDX9GrabParams = { 0 };
        NVFBCRESULT fbcRes = NVFBC_SUCCESS;
        {
            fbcDX9GrabParams.dwVersion      = NVFBC_TODX9VID_GRAB_FRAME_PARAMS_VER;
            fbcDX9GrabParams.dwFlags        = NVFBC_TODX9VID_NOWAIT;
            fbcDX9GrabParams.eGMode         = NVFBC_TODX9VID_SOURCEMODE_SCALE;
            fbcDX9GrabParams.dwTargetWidth  = ENCODER_WIDTH;
            fbcDX9GrabParams.dwTargetHeight = ENCODER_HEIGHT;
            fbcDX9GrabParams.pNvFBCFrameGrabInfo = &frameGrabInfo;

            fbcRes = NvFBCDX9->NvFBCToDx9VidGrabFrame(&fbcDX9GrabParams);
        }

        if (fbcRes == NVFBC_SUCCESS)
        {
            if (frameCnt == 0)
            {
                // Setup the NVENC for capture from NvFBC
                if (S_OK != g_pEncoder->SetupEncoder(ENCODER_WIDTH, ENCODER_HEIGHT, args.iBitrate, maxDisplayWidth, maxDisplayHeight,
                    (unsigned int)args.iCodec, g_apD3D9Surf, args.bYUV444))
                {
                    fprintf(stderr, "Failed when calling Encoder::SetupEncoder()\n");
                }
                currentWidth  = frameGrabInfo.dwBufferWidth;
                currentHeight = frameGrabInfo.dwHeight;
                fOut = GetOutputFile(args.sBaseName, frameGrabInfo.dwBufferWidth, frameGrabInfo.dwHeight, (bool)!!args.iCodec);
            }
            //! The frame number will only increment if the frame has changed
            double grabTime   = grabTimer.now();
            double encodeTime = frameTimer.now() - grabTime;

            printf("Capture frame %d: grab time %.2f, encode time %.2f ms to: %s\n",
                frameCnt, grabTime, encodeTime, baseFileName);

            frameTimer.reset();

            //! If the grab resolution is different then the current resolution the encoder must be re-initialized
            if ((currentWidth != frameGrabInfo.dwBufferWidth) || (currentHeight != frameGrabInfo.dwHeight))
            {
                // bool bNeedNewResources = (frameGrabInfo.dwWidth > currentWidth) || (frameGrabInfo.dwHeight > currentHeight);
                g_pEncoder->Reconfigure(frameGrabInfo.dwWidth, frameGrabInfo.dwHeight, args.iBitrate, FALSE);
                //! Save the height and width so we can determine if it has changed.
                currentWidth = frameGrabInfo.dwBufferWidth;
                currentHeight = frameGrabInfo.dwHeight;

                //! Close the output file
                if (NULL != fOut)
                {
                    fclose(fOut);
                    fOut = NULL;
                }

                //! Get a new file pointer to write the stream to.
                fOut = GetOutputFile(args.sBaseName + "_1", currentWidth, currentHeight, (bool)!!args.iCodec);

                if (!fOut)
                {
                    Cleanup();
                    return -1;
                }
            }

            HRESULT hr = g_pEncoder->LaunchEncode(frameIDX);
            if (hr != S_OK)
            {
                fprintf(stderr, "Failed encoding via LaunchEncode frame %d\n", frameCnt);
                Cleanup();
                return -1;
            }
            hr = g_pEncoder->GetBitstream(frameIDX, fOut);
            if (hr != S_OK)
            {
                fprintf(stderr, "Failed encoding via GetBitstream frame %d\n", frameCnt);
                Cleanup();
                return -1;
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
