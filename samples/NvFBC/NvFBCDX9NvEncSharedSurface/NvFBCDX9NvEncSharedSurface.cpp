/*!
 * \brief
 * Demonstrates using the NvFBCToDx9 interface to copy the desktop
 * into a DX9 buffer and then send it to NVENC for hardware encode
 * where capture and encode are using separate DX9 contexs
 *
 * \file
 * This sample demonstrates using the NvFBCToDx9 interface to copy the desktop
 * into a shared DX9  buffer. The shared buffer is opened for use as input
 * to NVENC running on a separate context, for the NVENC HW video encoder to encode the stream.
 *
 * Encoder.h: Declares the Encoder class, this class is simply a wrapper around
 * the NVENC encoder API.
 *
 * Encoder.cpp: Defines the Encoder class, wraps up the details when using the
 * NVENC video encoder so it don't distract from the NvFBCDX9 example.
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
#define FRAME_COUNT 120

//! Starting encoder bitrate
#define ENCODER_BITRATE 8000000
#define ENCODER_WIDTH  1920
#define ENCODER_HEIGHT 1080

#define SAMPLE_EXE_NAME "NvFBCDX9NvEncSharedSurface"

//! Command line arguments
typedef struct
{
    int         iFrameCnt; // Number of frames to encode
    int         iBitrate;  // Bitrate to use
	bool        bYUV444;   // Use 4:4:4 YUV encode
	int         iCodec;    // Codec format (H.264 or H.265)
	std::string sBaseName; // Basename for the output stream
}AppArguments;

inline void
createOutputFilename(char *baseFileName, const char *baseName, DWORD width, DWORD height, bool bHEVC)
{
	if (bHEVC)
	{
		sprintf(baseFileName, "%s-%dx%d.h265", baseName, width, height);
	}
	else
	{
		sprintf(baseFileName, "%s-%dx%d.h264", baseName, width, height);
	}
}


FILE *GetOutputFile(std::string baseName, DWORD width, DWORD height, bool bHEVC)
{
    FILE *output = NULL;

	char baseFileName[256];

	createOutputFilename(baseFileName, baseName.c_str(), width, height, bHEVC);

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
	args.iCodec     = 0; // 0=H264, 1=HEVC
	args.sBaseName = SAMPLE_EXE_NAME;

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
IDirect3DDevice9Ex  *g_pD3D9DeviceEncode = NULL;
IDirect3DSurface9   *g_pD3D9Surf = NULL;
IDirect3DSurface9	*g_pCommitRenderTarget = NULL;
IDirect3DSurface9	*g_pCommitSystemSurface = NULL;
HANDLE g_hSharedSurface = NULL;
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

	if (g_pCommitSystemSurface) {
		g_pCommitSystemSurface->Release();
		g_pCommitSystemSurface = 0;
	}

	if (g_pCommitRenderTarget) {
		g_pCommitRenderTarget->Release();
		g_pCommitRenderTarget = 0;
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

	if (g_pD3D9DeviceEncode) 
	{
		g_pD3D9DeviceEncode->Release();
		g_pD3D9DeviceEncode = NULL;
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

	hr = g_pD3D9Device->CreateRenderTarget(1, 1, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &g_pCommitRenderTarget, NULL);
	if (FAILED(hr)) {
		printf("CreateRenderTarget() failed in line %d.\n", __LINE__);
		return E_FAIL;
	}

	hr = g_pD3D9Device->CreateOffscreenPlainSurface(1, 1, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &g_pCommitSystemSurface, NULL);
	if (FAILED(hr)) {
		printf("CreateOffscreenPlainSurface() failed.\n");
		return E_FAIL;
	}

	hr = g_pD3DEx->CreateDeviceEx(
		deviceID,
		D3DDEVTYPE_HAL,
		NULL,
		dwBehaviorFlags,
		&d3dpp,
		NULL,
		&g_pD3D9DeviceEncode);

	assert(SUCCEEDED(hr));

    return hr;
}

HRESULT InitD3D9Surfaces()
{
    HRESULT hr = E_FAIL;
    if (g_pD3D9Device)
    {
        hr = g_pD3D9Device->CreateOffscreenPlainSurface(ENCODER_WIDTH, ENCODER_HEIGHT, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_pD3D9Surf, &g_hSharedSurface);
        if (FAILED(hr))
        {
            fprintf(stderr, "Failed to create D3D9 surfaces for output. Error 0x%08x\n", hr);
        }
    }
    return hr;
}

static void CommitUpdate(IDirect3DDevice9 *pDevice, IDirect3DSurface9 *pOffscreenSurface, IDirect3DSurface9 *pCommitRenderTarget, IDirect3DSurface9 *pCommitSystemSurface) {
	RECT rc = { 0, 0, 1, 1 };
	HRESULT hr = pDevice->StretchRect(pOffscreenSurface, &rc, pCommitRenderTarget, &rc, D3DTEXF_NONE);
	if (FAILED(hr)) {
		printf("GetRenderTargetData() failed.\n");
		return;
	}

	hr = pDevice->GetRenderTargetData(pCommitRenderTarget, pCommitSystemSurface);
	if (FAILED(hr)) {
		printf("GetRenderTargetData() failed.\n");
		return;
	}

	D3DLOCKED_RECT lockedRect;
	pCommitSystemSurface->LockRect(&lockedRect, NULL, D3DLOCK_READONLY);
	pCommitSystemSurface->UnlockRect();
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
    IDirect3DSurface9 *pYUVBuffer = NULL;
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
    // Demonstrate how to use NVENC on separate DX9 device
    if (S_OK != g_pEncoder->Init(g_pD3D9DeviceEncode))
    {
        fprintf(stderr, "Failed to initialize NVENC video encoder\n");
        Cleanup();
        return -1;
    }

    g_bEncoderInitDone = true;

    NvFBC_OutBuf[0].pPrimary = g_pD3D9Surf;

    NVFBC_TODX9VID_SETUP_PARAMS DX9SetupParams = {0};
    DX9SetupParams.dwVersion     = NVFBC_TODX9VID_SETUP_PARAMS_V2_VER;
    DX9SetupParams.bWithHWCursor = 0;
    DX9SetupParams.bStereoGrab   = 0;
    DX9SetupParams.bDiffMap      = 0;
    DX9SetupParams.ppDiffMap     = NULL;
    DX9SetupParams.ppBuffer	     = NvFBC_OutBuf;
    DX9SetupParams.eMode         = NVFBC_TODX9VID_ARGB;
    DX9SetupParams.dwNumBuffers  = 1;

    if (NVFBC_SUCCESS != NvFBCDX9->NvFBCToDx9VidSetUp(&DX9SetupParams))
    {
        fprintf(stderr, "Failed when calling NvFBCDX9->NvFBCToDX9VidSetup()\n");
        Cleanup();
        return -1;
    }

	fOut = GetOutputFile(args.sBaseName, ENCODER_WIDTH, ENCODER_HEIGHT, (bool)!!args.iCodec);
    // Setup the NVENC for capture from NvFBC
	if (S_OK != g_pEncoder->SetupEncoder(ENCODER_WIDTH, ENCODER_HEIGHT, args.iBitrate, maxDisplayWidth, maxDisplayHeight, 
											(unsigned int)args.iCodec, g_hSharedSurface, args.bYUV444, args.iFrameCnt, fOut))
    {
        fprintf(stderr, "Failed when calling Encoder::SetupEncoder()\n");
    }

	int frameCnt = 0;
    while (!g_pEncoder->IsDone())
    {
		double grabTime;

        // Setup NvFBC the DX9 Video Grab Parameters
        NVFBC_TODX9VID_GRAB_FRAME_PARAMS fbcDX9GrabParams = { 0 };
		NvFBCFrameGrabInfo frameGrabInfo = { 0 };
        NVFBCRESULT fbcRes = NVFBC_SUCCESS;
        {
            fbcDX9GrabParams.dwVersion      = NVFBC_TODX9VID_GRAB_FRAME_PARAMS_VER;
            fbcDX9GrabParams.dwFlags        = NVFBC_TODX9VID_NOFLAGS;
            fbcDX9GrabParams.eGMode         = NVFBC_TODX9VID_SOURCEMODE_SCALE;
            fbcDX9GrabParams.dwTargetWidth  = ENCODER_WIDTH;
            fbcDX9GrabParams.dwTargetHeight = ENCODER_HEIGHT;
            fbcDX9GrabParams.pNvFBCFrameGrabInfo = &frameGrabInfo;

 			g_pEncoder->EnterCS();
			grabTimer.reset();
			fbcRes = NvFBCDX9->NvFBCToDx9VidGrabFrame(&fbcDX9GrabParams);
			grabTime = grabTimer.now();
			CommitUpdate(g_pD3D9Device, g_pD3D9Surf, g_pCommitRenderTarget, g_pCommitSystemSurface);
			g_pEncoder->LeaveCS();
        }

        if (fbcRes == NVFBC_SUCCESS)
        {
            printf("Capture %d: frame %d, grab time %.2f\n",
                frameCnt, frameCnt, grabTime);
			frameCnt++;
        }
        else
        {
            fprintf(stderr, "Grab frame failed. NvFBCDX9GrabFrame returned: %d\n", fbcRes);
            Cleanup();
            return -1;
        }
    }

	char videoOutput[256];
	createOutputFilename(videoOutput, SAMPLE_EXE_NAME, ENCODER_WIDTH, ENCODER_HEIGHT, (bool)!!args.iCodec);
	printf("NvFBC Captured Video File: %s\n", videoOutput);

    Cleanup();
    return 0;
}
