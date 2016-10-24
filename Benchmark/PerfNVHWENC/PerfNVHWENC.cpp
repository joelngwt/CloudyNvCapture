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
 * Copyright 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <Windows.h>
#include <process.h>
#include <mmsystem.h>
#include <d3dx9.h>
#include <math.h>
#include <ddraw.h>

#pragma warning( disable : 4995 ) // disable deprecated warnings
#pragma warning( disable : 4996 )  

#include <strsafe.h>

#include <NvIFR/NvIFR.h>
#include <NvIFR/NvHWEnc.h>
#include <NvIFR/NvIFRHWEnc.h>
#include <assert.h>
#include <NvIFRLibrary.h>
#include <Util.h>

typedef HRESULT (WINAPI *pDirectDrawEnumerateExA_func)(LPDDENUMCALLBACKEXA, LPVOID, DWORD);
typedef HRESULT (WINAPI *pDirectDrawCreateEx_func)(GUID FAR *, LPVOID  *, REFIID,IUnknown FAR *);

#define MAX_ENCODERS 16
#define MIN_ENCODERS 1

INvIFRToHWEncoder_v1 * g_apIFRs[MAX_ENCODERS];

const unsigned int MAX_NUM_FRAMES_TO_BE_ENCODED = 200;
const unsigned int vidMemUsed = 64 * 1024 * 1024;

////////////// this sample demonstrates performance of NVEnc using multiple streams
////////////// this is not a typical use case. a real usage case would load one IFR object per application
////////////// the NvIFR object would be instanciated in the device of the main thread
LPDIRECT3D9EX           g_pD3DEx = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9       g_pD3DDevice = NULL; // Our texture loading device
D3DDISPLAYMODE          g_D3Ddm;
LPDIRECT3DDEVICE9       g_apD3DEncodingDevices[MAX_ENCODERS]; // Our encoding devices

DWORD g_dwNEncoders=MIN_ENCODERS;
DWORD g_dwNInputs=0;
DWORD g_bNoDump=false;
DWORD g_bNoDisp=false;
DWORD g_dwMaxFrames=-1; // Max. no of frames to grab
DWORD g_dwAvgBitRate = 0;
DWORD g_dwMaxBitRate = 0;
NV_HW_ENC_PARAMS_RC_MODE g_RCMode = NV_HW_ENC_PARAMS_RC_2_PASS_QUALITY;
NV_HW_ENC_PRESET         g_Preset = NV_HW_ENC_PRESET_LOW_LATENCY_HP;
NV_HW_ENC_CODEC          g_Codec  = NV_HW_ENC_H264;
DWORD g_EnableLossless = 0;
DWORD g_UseYUV444 = 0;
DWORD g_dwDeblockingMode = 0;
/*!
 * LowLatency if set sets VBV buffer size to 1 frame instead of 1sec
 * there by reducing the intial delay for encoding
 * but Intial frames qulity goes down 
 */
DWORD g_bLowLatency = false;
std::string g_sBaseName = ""; // Basename for the output stream

LPDIRECT3DTEXTURE9      g_apD3DTextureVid[2048]; // Our textures in vidmem
LPDIRECT3DTEXTURE9      g_apD3DTextureVidEncoder[MAX_ENCODERS][2048]; // Our textures alias vidmem
HANDLE                  g_apD3DTextureVidSharedHandles[2048];
LPDIRECT3DTEXTURE9      g_pD3DTextureSys = NULL; // Our sys mem texture

DWORD g_dwWidth=0;
DWORD g_dwHeight=0;

LONGLONG g_llBegin=0;
LONGLONG g_llPerfFrequency=0; 
BOOL g_timeInitialized=FALSE;

#define QPC(Int64)  QueryPerformanceCounter((LARGE_INTEGER*)&Int64)  
#define QPF(Int64)  QueryPerformanceFrequency((LARGE_INTEGER*)&Int64)


double GetFloatingDate()
{
    LONGLONG llNow;

    if (!g_timeInitialized)
    {
        QPC(g_llBegin);
        QPF(g_llPerfFrequency);
        g_timeInitialized = TRUE;
    }
    QPC(llNow);
    return(((double)(llNow-g_llBegin)/(double)g_llPerfFrequency) );
}

BOOL LoadBMPDims(char * fileName, DWORD * pw, DWORD * ph, DWORD * pBPP)
{
    BITMAPFILEHEADER bmfh;
    BITMAPINFOHEADER bmih;

    FILE * fin;

    if (fin=fopen(fileName,"rb"))
    {
        if ((fread(((BYTE *)(&bmfh)), sizeof(BITMAPFILEHEADER), 1, fin)!=1) || (fread(((BYTE *)(&bmih)), sizeof(BITMAPINFOHEADER), 1, fin)!=1))
        {
            fclose(fin);
            return FALSE;
        }

        *pw = bmih.biWidth;
        *ph = bmih.biHeight;
        *pBPP = bmih.biBitCount/8;

        fclose(fin);
        return TRUE;
    }
    return FALSE;
}

BOOL LoadBMP(char * fileName, unsigned char ** pp, DWORD * pw, DWORD * ph, DWORD *pBpp)
{
    BITMAPFILEHEADER bmfh;
    BITMAPINFOHEADER bmih;

    FILE * fin;

    if (fin=fopen(fileName,"rb"))
    {
        if ((fread(((BYTE *)(&bmfh)), sizeof(BITMAPFILEHEADER), 1, fin)!=1) || (fread(((BYTE *)(&bmih)), sizeof(BITMAPINFOHEADER), 1, fin)!=1))
        {
            fclose(fin);
            return FALSE;
        }

        *pw = bmih.biWidth;
        *ph = bmih.biHeight;
        *pBpp = bmih.biBitCount/8;

		if (bmih.biSizeImage == 0) {
			bmih.biSizeImage = *pw * *ph * *pBpp;
		}

        *pp = (unsigned char *) malloc(bmih.biSizeImage);

        if (!*pp)
        {
            fclose(fin);
            return FALSE;
        }

        if (fread(*pp, bmih.biSizeImage, 1, fin)!=1)
        {
            free(*pp);
            fclose(fin);
            return FALSE;
        }

        fclose(fin);
        return TRUE;
    }
    return FALSE;
}

BOOL LoadBMPWithReorderLines(char * fileName, unsigned char * p4Bytes, DWORD dwStride)
{
    unsigned char * lp;
    DWORD w, h, bpp;

    if (!LoadBMP(fileName, &lp, &w, &h, &bpp))
        return FALSE;

    if (bpp == 3)
    {
        for (DWORD j=0; j<h; j++)
        {
            unsigned char * p4 = p4Bytes + j*dwStride;
            unsigned char * p3 = lp + (h-1-j)*(w*3);

            for (DWORD i=0; i<w; i++)
            {
                p4[i*4+0] = p3[i*3+0];
                p4[i*4+1] = p3[i*3+1];
                p4[i*4+2] = p3[i*3+2];
                p4[i*4+3] = 0xff; // Opaque
            }
        }
    }
    else if (bpp == 4)
    {
        for (DWORD j=0; j<h; j++)
        {
            unsigned char * pDst = p4Bytes + j*dwStride;
            unsigned char * pSrc = lp + (h-1-j)*(w*4);
            memcpy(pDst, pSrc, w*4);
        }
    }
    else
    {
        printf("Unsupported BMP format. Only 24bit and 32bit BMPs supported.\n");
        free(lp);
        return FALSE;
    }

    free(lp);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: InitD3D()
// Desc: Initializes Direct3D
//-----------------------------------------------------------------------------
HRESULT InitD3D( HWND hWnd )
{
    // Create the D3D object.
    Direct3DCreate9Ex( D3D_SDK_VERSION, &g_pD3DEx );

    if (!g_pD3DEx)
        return E_FAIL;

    if( FAILED( g_pD3DEx->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &g_D3Ddm ) ) )
        return E_FAIL;

    // Set up the structure used to create the D3DDevice. Since we are now
    // using more complex geometry, we will create a device with a zbuffer.
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof( d3dpp ) );
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth  = g_dwWidth;
    d3dpp.BackBufferHeight = g_dwHeight;
    d3dpp.BackBufferFormat = g_D3Ddm.Format;  
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    // Create the D3DDevice
    if( FAILED( g_pD3DEx->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &d3dpp, &g_pD3DDevice ) ) )
    {
        return E_FAIL;
    }

    // Turn off culling
    g_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

    // Turn off D3D lighting
    g_pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE );

    // Turn on the zbuffer
    g_pD3DDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: Cleanup()
// Desc: Releases all previously initialized objects
//-----------------------------------------------------------------------------
VOID Cleanup()
{
    if( g_pD3DTextureSys != NULL )
        g_pD3DTextureSys->Release();

    if( g_pD3DDevice != NULL )
        g_pD3DDevice->Release();

    if( g_pD3DEx != NULL )
        g_pD3DEx->Release();
}

//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: The window's message handler
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
    case WM_DESTROY:
        Cleanup();
        PostQuitMessage( 0 );
        return 0;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}




#define NUMFRAMESINFLIGHT   3


typedef struct 
{
    DWORD                   m_dwThreadId;
    BOOL                    m_bExit;

    unsigned char *         m_apBitStreamBuffers[NUMFRAMESINFLIGHT];

    HANDLE                  m_ahEncodeCompletionEvents[NUMFRAMESINFLIGHT];
    HANDLE                  m_ahCanRenderEvents[NUMFRAMESINFLIGHT];

    HANDLE                  m_hFileWriterThreadHandle;

}SideThreadData;


void fileWriterThread(void * pData)
{
    DWORD dwIndex=0;
    SideThreadData * pSideThreadData=(SideThreadData *)pData;
    FILE * fileOut=NULL;
    char fileName[256];

    if (!g_bNoDump)
    {
        if (g_sBaseName.length() == 0)
        {
            g_sBaseName = "PerfNVHWEnc";
        }

        if (g_Codec == NV_HW_ENC_HEVC)
        {
            sprintf(fileName, "%s[%d]-stream-%d-%d x %d.h265", g_sBaseName.c_str(), GetCurrentThreadId(), pSideThreadData->m_dwThreadId, g_dwWidth, g_dwHeight);
        }
        else
        {
            sprintf(fileName, "%s[%d]-stream-%d-%d x %d.h264", g_sBaseName.c_str(), GetCurrentThreadId(), pSideThreadData->m_dwThreadId, g_dwWidth, g_dwHeight);
        }
        fileOut = fopen(fileName, "wb");
    }

    while (!pSideThreadData->m_bExit)
    {   
        WaitForSingleObject(pSideThreadData->m_ahEncodeCompletionEvents[dwIndex], INFINITE);
        ResetEvent(pSideThreadData->m_ahEncodeCompletionEvents[dwIndex]);

        if (pSideThreadData->m_bExit)
            break;

        NVIFR_HW_ENC_GET_BITSTREAM_PARAMS params = {0};
        params.dwVersion = NVIFR_HW_ENC_GET_BITSTREAM_PARAMS_VER;
        params.bitStreamParams.dwVersion = NV_HW_ENC_GET_BIT_STREAM_PARAMS_VER;
        params.dwBufferIndex = dwIndex;

        NVIFRRESULT res = g_apIFRs[pSideThreadData->m_dwThreadId]->NvIFRGetStatsFromHWEncoder(&params);

        if (!g_bNoDump)
            fwrite(pSideThreadData->m_apBitStreamBuffers[dwIndex], params.bitStreamParams.dwByteSize, 1, fileOut);

        SetEvent(pSideThreadData->m_ahCanRenderEvents[dwIndex]);

        dwIndex = (dwIndex+1)%NUMFRAMESINFLIGHT;
    }

    if (!g_bNoDump)
    {
        if (fileOut)
            fclose(fileOut);
    }
}

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
INT WINAPI _tWinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    printf("*************************\nPerfNVHWEnc: Concurrent Encoders to use: %d, Max frames specified: %d\n*************************\n", g_dwNEncoders, g_dwMaxFrames);
    // Register the window class
    WNDCLASSEX wc =
    {
        sizeof( WNDCLASSEX ), CS_CLASSDC, MsgProc, 0L, 0L,
        GetModuleHandle( NULL ), NULL, NULL, NULL, NULL,
        "NVEnc Performance", NULL
    };
    RegisterClassEx( &wc );

    HRESULT hr;

    char buf[MAX_PATH];
    char pFullName[MAX_PATH];
    size_t dwCaptureSDKPathSize = 0;
    char *pCaptureSDKPath = NULL;
    char *pDatasetPath = NULL;

    if(0 != _dupenv_s(&pCaptureSDKPath, &dwCaptureSDKPathSize, "CAPTURESDK_PATH") || 0 == dwCaptureSDKPathSize)
    {
        printf("Unable to get the CAPTURESDK_PATH environment variable\n");
        return FALSE;
    }

    pDatasetPath = new char[dwCaptureSDKPathSize + 20];
    sprintf(pDatasetPath, "%s\\datasets\\msenc", pCaptureSDKPath);

    HANDLE hFind;
    WIN32_FIND_DATA FindFileData;

    sprintf(pFullName, "%s\\*.bmp", pDatasetPath);

    hFind = FindFirstFile(pFullName, &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        printf ( "Unable to find: %s\n", pFullName );
        return FALSE;
    }

    BOOL bFound=TRUE;

    sprintf(pFullName, "%s\\%s", pDatasetPath, FindFileData.cFileName);    
    DWORD dwBPP=0;
    LoadBMPDims(pFullName, &g_dwWidth, &g_dwHeight, &dwBPP);

    while (bFound)
    {
        g_dwNInputs++;
        bFound=FindNextFile(hFind, &FindFileData);
        if ((g_dwMaxFrames != -1) && (g_dwNInputs >= g_dwMaxFrames))
            break;
    }

    if (g_dwMaxFrames == -1)
    {
        g_dwMaxFrames = g_dwNInputs;
    }

    if (g_dwNInputs < g_dwMaxFrames)
    {
        printf("Found only %d files to encode. Will loop over the available inputs.\n", g_dwNInputs);
    }

    FindClose(hFind);

    // Create the application's window
    HWND hWnd = CreateWindow( "NVEnc Performance", "NVEnc Performance",
        WS_OVERLAPPEDWINDOW, 100, 100, g_dwWidth, g_dwHeight,
        NULL, NULL, wc.hInstance, NULL );

    DWORD dwNvIFRVersion=0;

    // Initialize Direct3D
    if( !SUCCEEDED( InitD3D( hWnd ) ) )
    {
        printf("Failed to Initialize D3D. Quitting\n");
        return FALSE;
    }

    printf("Calculating no of frames that can be fit in video memory.\n");
    // The logic is to find the no of frames that can be fit in with available vid mem and also with VA availble(VA is limitation for 32 bit OS)
    // The actual no of frames being fit in is half of the lower of the above two as we have to leave additional space for encoder intermediate resources.

    DWORD dwFramesFB = GetFramesFB(g_pD3DDevice, hWnd, g_dwWidth, g_dwHeight);
    printf("Found %d files to encode. Video memory can hold max %d frames.\n", g_dwNInputs, dwFramesFB);

    // Needs 16 refererecne frames and 16 pre-processing frames for NvEnc
    g_dwNInputs = dwFramesFB - 32;
    if(g_dwNInputs <= 0)
    {
        printf("\nNot enough video memory to run PerfNvHWEnc.\n");
        return FALSE;
    }

    if (g_dwNInputs >= MAX_NUM_FRAMES_TO_BE_ENCODED)
    {
        g_dwNInputs = MAX_NUM_FRAMES_TO_BE_ENCODED;
    }

    printf("Encoding %d frames.\n", g_dwNInputs);

    for (DWORD i4=0; i4< g_dwNInputs; i4++)
    {
        g_apD3DTextureVidSharedHandles[i4]=NULL;
        hr=g_pD3DDevice->CreateTexture(g_dwWidth, g_dwHeight, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_apD3DTextureVid[i4], &g_apD3DTextureVidSharedHandles[i4]);

        if (hr==D3DERR_OUTOFVIDEOMEMORY)
        {
            g_dwNInputs = i4 * 3 / 4; // Reassigning g_dwNInputs to 75% of last successful frame count. 
            DWORD dwNInputs = g_dwNInputs;
            while (dwNInputs < i4)
            {
                g_apD3DTextureVid[dwNInputs]->Release();//Releasing the unused 25% textures to free up video memory space. 
                dwNInputs++;
            }
            printf("WARNING: Unable to create Texture #%d due to D3DERR_OUTOFVIDEOMEMORY. Encoding %d frames. Perf results might not be accurate for comparisons\n", i4, g_dwNInputs);
            break;
        }
        else if (hr!=S_OK)
        {
            printf("Failed in input create texture #%d. HRESULT = %x. Quitting\n", i4, hr);
            g_dwNInputs=0;
            return FALSE;
        }
    }

    sprintf(buf, "%s\\*.bmp", pDatasetPath);    

    hFind = FindFirstFile(buf, &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE) 
        return FALSE;

    bFound=TRUE;

    unsigned char * pTexSysBuf;

    pTexSysBuf=(unsigned char *)malloc(g_dwWidth*g_dwHeight*4);

    g_pD3DDevice->CreateTexture(g_dwWidth, g_dwHeight, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &g_pD3DTextureSys, (HANDLE *)&pTexSysBuf);

    DWORD dwARGBPitch=g_dwWidth*4;
    DWORD i=0;

    printf("Loading input files.\n");

    while (bFound && i<g_dwNInputs)
    {
        printf(".");
        if (0==i%50)printf("\n");
        D3DLOCKED_RECT Rect;

        sprintf(pFullName, "%s\\%s", pDatasetPath, FindFileData.cFileName);    

        //printf("loaded file %s in vidmem\n", FindFileData.cFileName);

        hr=g_pD3DTextureSys->LockRect(0, &Rect, NULL, D3DLOCK_DISCARD);

        if (!LoadBMPWithReorderLines(pFullName, (unsigned char *)pTexSysBuf, dwARGBPitch))
        {
            printf("Failed to load BMP file %s. Quitting.\n", pFullName);
            return FALSE;
        }

        g_pD3DTextureSys->UnlockRect(0);

        hr=g_pD3DDevice->UpdateTexture(g_pD3DTextureSys, g_apD3DTextureVid[i]);

        bFound=FindNextFile(hFind, &FindFileData);

        i++;
    }
    printf("\n");
    FindClose(hFind);

    g_pD3DTextureSys->Release();

    free(pTexSysBuf);

    HINSTANCE g_hNvIFRDll=NULL;

    free(pCaptureSDKPath);
    free(pDatasetPath);

    NvIFRLibrary NvIFRlib;
    g_hNvIFRDll=NvIFRlib.load();

    if (!g_hNvIFRDll)
    {
        printf("Failed to load NvIFR. Quitting.\n");
        return FALSE;
    }

    if (!g_bNoDisp)
    {
        ShowWindow( hWnd, SW_SHOWDEFAULT );
        UpdateWindow( hWnd );
    }

    printf("Loading %d DX9 RT Textures\n", g_dwNInputs);
    for (DWORD i1=0; i1<g_dwNInputs; i1++)
    {
        LPDIRECT3DSURFACE9      g_pSurface = NULL; // where to blit the rendered data, in the A device

        IDirect3DSurface9* pCurrentRenderTarget;

        hr=g_pD3DDevice->GetRenderTarget(0, &pCurrentRenderTarget);

        hr=g_apD3DTextureVid[i1]->GetSurfaceLevel(0, &g_pSurface);

        hr=g_pD3DDevice->StretchRect(g_pSurface, NULL, pCurrentRenderTarget, NULL, D3DTEXF_NONE);

        hr=g_pSurface->Release();

        pCurrentRenderTarget->Release();

        if (!g_bNoDisp)
        {
            // Present the backbuffer contents to the display
            Sleep(30);
        }
        g_pD3DDevice->Present( NULL, NULL, NULL, NULL );
    }

    if (!g_bNoDisp)
    {
        ShowWindow( hWnd, SW_HIDE );
    }

    assert(g_dwNEncoders<=MAX_ENCODERS);
    assert(g_dwNEncoders>=MIN_ENCODERS);

    SideThreadData aSideThreadData[MAX_ENCODERS];
    printf("Creating %d Encoders\n", g_dwNEncoders);

    TCHAR windowName[64];
    for (DWORD d=0; d<g_dwNEncoders; d++)
    {
        // each encoder/NvIFR get their own dx context and window
        printf("Creating Encoder #%d\n", d);
        sprintf(windowName, "NvEnc Performance #d", d);
        // Create the application's window
        HWND hWnd = CreateWindow( windowName, windowName,
            WS_OVERLAPPEDWINDOW, 100, 100, g_dwWidth, g_dwHeight,
            NULL, NULL, wc.hInstance, NULL );

        // Set up the structure used to create the D3DDevice. Since we are now
        // using more complex geometry, we will create a device with a zbuffer.
        D3DPRESENT_PARAMETERS d3dpp;
        ZeroMemory( &d3dpp, sizeof( d3dpp ) );
        d3dpp.Windowed = TRUE;
        d3dpp.BackBufferWidth  = g_dwWidth;
        d3dpp.BackBufferHeight = g_dwHeight;
        d3dpp.BackBufferFormat = g_D3Ddm.Format;  
        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dpp.EnableAutoDepthStencil = TRUE;
        d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
        d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

        // Create the D3DDevice
        if( FAILED( hr = g_pD3DEx->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
            D3DCREATE_HARDWARE_VERTEXPROCESSING,
            &d3dpp, &g_apD3DEncodingDevices[d] ) ) )
        {
            printf("Failed to create D3D9 Device. hr = 0x%08x\n", hr);
            return FALSE;
        }

        g_apIFRs[d] = (INvIFRToHWEncoder_v1 *)NvIFRlib.create(g_apD3DEncodingDevices[d], NVIFR_TO_HWENCODER);
        if (!g_apIFRs[d])
        {
            printf("No HW encoder found. Quitting.\n");
            return FALSE;
        }

#if _DEBUG
        g_apD3DEncodingDevices[d]->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 0, 255 ), 1.0f, 0 );
#else
        g_apD3DEncodingDevices[d]->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 0, 0 ), 1.0f, 0 );
#endif


        DWORD dwBitRate720p = 5000000;
        double  dBitRate = 0;
        if (g_dwAvgBitRate > 0)
            dBitRate = double(g_dwAvgBitRate);
        else
            dBitRate = double((double)(dwBitRate720p)*((double)(g_dwWidth*g_dwHeight)/(double)(1280*720)));           // scaling bitrate with resolution, precision is not important.

        NVIFR_HW_ENC_SETUP_PARAMS params = {0};
        params.dwVersion = NVIFR_HW_ENC_SETUP_PARAMS_VER;
        params.eStreamStereoFormat = NV_HW_ENC_STEREO_NONE;
        params.dwNBuffers = NUMFRAMESINFLIGHT;
        params.dwBSMaxSize = (NvU32)(g_dwWidth * g_dwHeight * (g_UseYUV444 ? 3 : 1.5));
        params.ppPageLockedBitStreamBuffers = aSideThreadData[d].m_apBitStreamBuffers;
        params.ppEncodeCompletionEvents = aSideThreadData[d].m_ahEncodeCompletionEvents;

        params.configParams.dwVersion      = NV_HW_ENC_CONFIG_PARAMS_VER;
        params.configParams.eCodec         = g_Codec;
        params.configParams.dwProfile      = 100;
        params.configParams.dwFrameRateDen = 1;
        params.configParams.dwFrameRateNum = 30;

        if (g_dwDeblockingMode != NV_HW_ENC_DEBLOCKING_FILTER_MODE_DEFAULT)
        {
            params.configParams.eSlicingMode        = NV_HW_ENC_SLICING_MODE_FIXED_NUM_SLICES;
            params.configParams.dwSlicingModeParam  = 4;
            params.configParams.eDeblockingMode     = static_cast<NV_HW_ENC_DEBLOCKING_FILTER_MODE>(g_dwDeblockingMode);
        }

        if (g_UseYUV444)
        {
            params.configParams.bEnableYUV444Encoding = TRUE;
            dBitRate += (dBitRate/3);
        }
        if(g_EnableLossless)
        {
            params.configParams.eRateControl   = NV_HW_ENC_PARAMS_RC_CONSTQP;
            params.configParams.ePresetConfig  = NV_HW_ENC_PRESET_LOSSLESS_HP;
        }
        else
        {
            params.configParams.dwAvgBitRate   = (DWORD)dBitRate;
            params.configParams.dwPeakBitRate  = (g_dwMaxBitRate > 0) ? g_dwMaxBitRate : (DWORD)((double)params.configParams.dwAvgBitRate*1.2);
            params.configParams.eRateControl   = g_RCMode;
            params.configParams.ePresetConfig  = g_Preset;
            params.configParams.dwQP           = 26 ;
        }

 
        // Set GOP and VBV buffer size (1 sec for default, and 1 frame for low latency)
		// Note: 1-frame-sized VBV buffer shouldn't be combined with finite GOP length, otherwise bad encoding quality follows.
		params.configParams.dwGOPLength = g_bLowLatency ? 0xFFFFFFFF : 75;
        params.configParams.dwVBVBufferSize = g_bLowLatency ? params.configParams.dwAvgBitRate / params.configParams.dwFrameRateNum 
			: params.configParams.dwAvgBitRate;
		params.configParams.dwVBVInitialDelay = params.configParams.dwVBVBufferSize;

		params.dwTargetWidth = g_dwWidth / 16 * 16;
		params.dwTargetHeight = g_dwHeight / 8 * 8;

        NVIFRRESULT res = g_apIFRs[d]->NvIFRSetUpHWEncoder(&params);
        if (res != NVIFR_SUCCESS)
        {
            if (res == NVIFR_ERROR_INVALID_PARAM || res != NVIFR_ERROR_INVALID_PTR)
                MessageBox(NULL, "NvIFR Buffer creation failed due to invalid params.\n", "NVEnc Performance", MB_OK );
            else
                MessageBox( NULL, "Setting up HW encoder failed", "NVEnc Performance", MB_OK );

            return FALSE;
        }

        if (res != NVIFR_SUCCESS)
        {
            if (res == NVIFR_ERROR_INVALID_PARAM || res != NVIFR_ERROR_INVALID_PTR)
                MessageBox(NULL, "NvIFR Buffer creation failed due to invalid params.\n", "NVEnc Performance", MB_OK );
            else
                MessageBox( NULL, "Something is wrong with the driver, cannot initialize NVIFR\n", "NVEnc Performance", MB_OK );

            return FALSE;
        }

        for (DWORD i5=0; i5<g_dwNInputs; i5++)
        {
            hr=g_apD3DEncodingDevices[d]->CreateTexture(g_dwWidth, g_dwHeight, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_apD3DTextureVidEncoder[d][i5], &g_apD3DTextureVidSharedHandles[i5]);

            if (hr!=S_OK)
            {
                printf("Failed to create video memory textures to feed the encoder. hr = 0x%08x\n", hr);
                return FALSE;
            }
        }

        for (DWORD i3=0; i3<NUMFRAMESINFLIGHT; i3++)
        {
            aSideThreadData[d].m_ahCanRenderEvents[i3] = CreateEvent(NULL, TRUE, TRUE, NULL);
        }

        aSideThreadData[d].m_dwThreadId=d;
        aSideThreadData[d].m_bExit=FALSE;
        aSideThreadData[d].m_hFileWriterThreadHandle=(HANDLE)_beginthread(fileWriterThread, 0, (void *)&aSideThreadData[d]);
    }

    NVIFRRESULT res = NVIFR_SUCCESS;
    LPDIRECT3DSURFACE9      g_pSurface = NULL; // where to blit the rendered data, in the A device

    double dStart = GetFloatingDate();
    double dNow = 0;
    DWORD dwBufferIndex=0;
    printf("Starting Encode\n");
    //Sleep(5000);
    for (DWORD i2=0; i2<g_dwMaxFrames; i2++)
    {
        DWORD k = i2%g_dwNInputs;

        for (DWORD d=0; d<g_dwNEncoders; d++)
        {
            //printf("Input# %d encoded by Encoder# %d\n", i, d);
            WaitForSingleObject(aSideThreadData[d].m_ahCanRenderEvents[dwBufferIndex], INFINITE);
            ResetEvent(aSideThreadData[d].m_ahCanRenderEvents[dwBufferIndex]);

            IDirect3DSurface9 * pCurrentRenderTarget = NULL;
            hr=g_apD3DEncodingDevices[d]->GetRenderTarget(0, &pCurrentRenderTarget);
            hr=g_apD3DTextureVidEncoder[d][k]->GetSurfaceLevel(0, &g_pSurface);
            hr=g_apD3DEncodingDevices[d]->StretchRect(g_pSurface, NULL, pCurrentRenderTarget, NULL, D3DTEXF_NONE);

            ResetEvent(aSideThreadData[d].m_ahEncodeCompletionEvents[dwBufferIndex]);

            NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS params = {0};
            params.dwVersion = NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS_VER;
            params.encodePicParams.dwVersion = NV_HW_ENC_PIC_PARAMS_VER;
            params.dwBufferIndex = dwBufferIndex;
            res = g_apIFRs[d]->NvIFRTransferRenderTargetToHWEncoder(&params);

            hr=g_pSurface->Release();
            pCurrentRenderTarget->Release();

            if (res != NVIFR_SUCCESS)
            {
                MessageBox( NULL, "NvIFRTransferRenderTargetToHWEncoder failed.\n", "NVEnc Performance", MB_OK );
                break;
            }

            // g_apD3DEncodingDevices[d]->Present( NULL, NULL, NULL, NULL );
        }

        if (i2 && !(i2%20))
        {
            //printf("%d streams, %dx%d, frame %d FPS=%f\n", g_dwNEncoders, g_dwWidth, g_dwHeight, i2, double(i2)/(GetFloatingDate() - dStart));
        }


        dwBufferIndex = (dwBufferIndex+1)%NUMFRAMESINFLIGHT;
    }

    double dEnd = GetFloatingDate();
    printf("total time %f sec, FPS=%f\n", dEnd-dStart, double(g_dwMaxFrames)/(dEnd-dStart));

    MSG msg;
    ZeroMemory( &msg, sizeof( msg ) );
    if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    // we don't encode the last frame, whatever.

    for (DWORD d=0; d<g_dwNEncoders; d++)
    {
        aSideThreadData[d].m_bExit=TRUE;
        SetEvent(aSideThreadData[d].m_ahEncodeCompletionEvents[dwBufferIndex]);

        WaitForSingleObject(aSideThreadData[d].m_hFileWriterThreadHandle, INFINITE);
        g_apIFRs[d]->NvIFRRelease();
        g_apD3DEncodingDevices[d]->Release();
    }

    UnregisterClass( "NVEnc Performance", wc.hInstance );
    return TRUE;
}

// Prints the help message
void printHelp()
{
    printf("Usage: PerNVHWENC [options]\n");
    printf("  -frames framecnt           The number of frames to grab.\n");
    printf("  -encoders encodercnt       The number of concurrent encoders to run, default: 1, Max: %d.\n", MAX_ENCODERS);
    printf("  -noDisp                    Silent mode. No frames displayed, only essential logging spew.\n");
    printf("  -noDump                    No output bitstream is written to disk.\n");
    printf("  -lossless                  Will enable lossless encoding. \n");
    printf("  -yuv444                    Will enable yuv444 encoding. \n");
    printf("  -avgBitrate n              The average bitrate (in bits\\sec).\n");
    printf("  -maxBitrate n              The peak bitrate (in bits\\sec).\n");
    printf("  -deblockingMode mode       The debloking mode to be used [0, 2], default: 0.\n");
	printf("  -codec codectype           The codec to use for encode. 0 = H.264, 1 = HEVC\n");
	printf("  -lowLatency                Low latency encoding.\n");
	printf("  -preset n                  Encoder preset: 0 = HP, 1 = HQ, 2 = Default, 4 = lossless HP\n");
	printf("  -rcMode n                  Rate control mode: 0 = CONSTQP, 1 = VBR, 2 = CBR, 8 = 2_PASS_QUALITY, 16 = 2_PASS_FRAMESIZE_CAP, 32 = 2_PASS_VBR\n");
    printf("  -output                    The name of output file\n");
}

int main(int argc, char * argv[])
{
    int i;
    HINSTANCE inst;

    inst=(HINSTANCE)GetModuleHandle(NULL);

    for(i=1; i<argc; i++)
    {
        if(0 == _stricmp(argv[i], "-frames"))
        {
            ++i;
            if(i >= argc)
            {
                printf("Missing -frames option\n");
                printHelp();
                return -1;
            }

            g_dwMaxFrames = atoi(argv[i]);

            if(g_dwMaxFrames < 1)
            {
                // If invalid Frame count is passed, grab all that is possible.
                g_dwMaxFrames = -1;
            }
        }
        else if(0 == _stricmp(argv[i], "-noDump"))
        {
            g_bNoDump = true;
        }
        else if(0 == _stricmp(argv[i], "-noDisp"))
        {
            g_bNoDisp = true;
        }
        else if(0 == _stricmp(argv[i], "-encoders"))
        {
            ++i;
            if(i >= argc)
            {
                printf("Missing -encoders option\n");
                printHelp();
                return -1;
            }

            g_dwNEncoders = atoi(argv[i]);
            // Must grab at least one frame.
            if(g_dwNEncoders >= MAX_ENCODERS)
                g_dwNEncoders = MAX_ENCODERS;
            if(g_dwNEncoders <= 1)
                g_dwNEncoders = 1;
        }
        else if(0 == _stricmp(argv[i], "-preset"))
        {
            ++i;
            if(i >= argc)
            {
                printf("Missing -preset option\n");
                printHelp();
                return -1;
            }
            int iPreset = atoi(argv[i]);
            if ((iPreset >= NV_HW_ENC_PRESET_LOW_LATENCY_HP) && (iPreset < NV_HW_ENC_PRESET_LAST))
            {
                g_Preset = (NV_HW_ENC_PRESET)iPreset;
            }
        }
        else if(0 == _stricmp(argv[i], "-rcMode"))
        {
            ++i;
            if(i >= argc)
            {
                printf("Missing -rcMode option\n");
                printHelp();
                return -1;
            }
            int iRCMode = atoi(argv[i]);
            if ((iRCMode >= NV_HW_ENC_PARAMS_RC_CONSTQP) && (iRCMode < NV_HW_ENC_PARAMS_RC_2_PASS_VBR))
            {
                g_RCMode = (NV_HW_ENC_PARAMS_RC_MODE)iRCMode;
            }
        }
        else if(0 == _stricmp(argv[i], "-avgBitrate"))
        {
            ++i;
            if(i >= argc)
            {
                printf("Missing -avgBitrate option\n");
                printHelp();
                return -1;
            }
            g_dwAvgBitRate = atoi(argv[i]);
        }
        else if(0 == _stricmp(argv[i], "-maxBitrate"))
        {
            ++i;
            if(i >= argc)
            {
                printf("Missing -maxBitrate option\n");
                printHelp();
                return -1;
            }
            g_dwMaxBitRate = atoi(argv[i]);
        }
        else if(0 == _stricmp(argv[i], "-lossless"))
        {
            g_EnableLossless = 1;
        }
        else if(0 == _stricmp(argv[i], "-yuv444"))
        {
            g_UseYUV444 = 1;
        }
        else if(0 == _stricmp(argv[i], "-output"))
        {
            ++i;

            if(i >= argc)
            {
                printf("Missing -output option\n");
                printHelp();
                return false;
            }

            g_sBaseName = argv[i];
        }
        else if (0 == _stricmp(argv[i], "-deblockingMode"))
        {
            ++i;
            if (i >= argc)
            {
                printf("Missing -deblockingMode option\n");
                printHelp();
                return -1;
            }

            g_dwDeblockingMode = atoi(argv[i]);
            // Must grab at least one frame.
            if (g_dwDeblockingMode > 2 || g_dwDeblockingMode < 0)
            {
                printf("Invalid -deblockingMode option\n");
                printHelp();
                return -1;
            }
        }
        else if (0 == _stricmp(argv[i], "-codec"))
        {
            ++i;
            if (i >= argc)
            {
                printf("Missing -codec option\n");
                printHelp();
                return -1;
            }

            if (atoi(argv[i]) ==1)
            {
                g_Codec = NV_HW_ENC_HEVC;
                printf("Using HEVC for encoding\n");
            }
            else if (atoi(argv[i]) ==0)
            {
                g_Codec = NV_HW_ENC_H264;
                printf("Using H.264 for encoding\n");
            }
            else
            {
                g_Codec = NV_HW_ENC_H264;
                printf("Unknown parameter for -codec. Using H.264 for encoding\n");
            }
        }
		else if (0 == _stricmp(argv[i], "-lowLatency"))
		{
			g_bLowLatency = true;
		}
        else
        {
            printf("Unexpected argument %s\n", argv[i]);
            printHelp();
            return -1;
        }
    }

    if (TRUE == _tWinMain(inst, NULL, NULL, SW_SHOWNORMAL))
        return 0;
    else
        return -1;
}
