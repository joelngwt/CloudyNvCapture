/*!
 * \brief
 * Writes to a shared surface, which further decouples rendering and 
 * allows the renderer and encoder to both run at their maximum rate. 
 *
 * \file
 *
 * This DirectX 9 sample demonstrates how to grab and encode a frame 
 * using a shared surface with asynchronous render and encode. Like 
 * the AsyncH264HwEncode sample, writing the output file occurs asynchronously 
 * from the main thread. However, writing to a shared surface also 
 * allows rendering to occur asynchronously from encoding, so that 
 * both render and grab run at their maximum rate.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma warning( disable : 4995 4996 ) // disable deprecated warning
#include <Windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <d3dx9.h>
#include <process.h> 
#include <strsafe.h>
#include <NvIFR/NvIFR.h>
#include <NvIFR/NvIFRHWEnc.h>
#include <NvIFRLibrary.h>

//! pointer to the INvIFRToHWEncoder_v1 object
INvIFRToHWEncoder_v1 * g_pIFR=NULL;
//! the D3D Device
LPDIRECT3D9             g_pD3D = NULL;
//! the D3D Rendering device
LPDIRECT3DDEVICE9       g_pD3DDeviceR = NULL; 
//! the D3D Encoding device
LPDIRECT3DDEVICE9       g_pD3DDeviceE = NULL; 
LPDIRECT3DVERTEXBUFFER9 g_pD3DVB = NULL; 


BOOL g_bStopEncoding=FALSE;
BOOL g_bIFRNotCreated = FALSE;

FILE * fileOut=NULL;
IFRSharedSurfaceHandle g_hIFRSharedSurface = NULL;

//! a structure for custom vertex type (pos, color, uv)
struct CUSTOMVERTEX
{
    D3DXVECTOR3 position; // The position
    D3DCOLOR color;    // The color
    FLOAT tu, tv;   // The texture coordinates
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)
#define SAMPLE_NAME "DX9IFR_SharedSurfaceHWEncode"

int g_dwWidth=1280;
int g_dwHeight=720;
DWORD g_dwMaxFrames = 30;
DWORD g_dwFrameCnt=0;

HINSTANCE g_hNvIFRDll=NULL;
NvIFRLibrary g_NvIFRlib;

NV_HW_ENC_CODEC          g_Codec  = NV_HW_ENC_H264;

NvIFR_CreateSharedSurfaceEXTFunctionType NvIFR_CreateSharedSurface_fn = NULL;
NvIFR_DestroySharedSurfaceEXTFunctionType NvIFR_DestroySharedSurface_fn = NULL;
NvIFR_CopyToSharedSurfaceEXTFunctionType NvIFR_CopyToSharedSurface_fn = NULL;
NvIFR_CopyFromSharedSurfaceEXTFunctionType NvIFR_CopyFromSharedSurface_fn = NULL;

HANDLE g_hCanRenderEvent = NULL;
HANDLE g_hCanEncodeEvent = NULL;
HANDLE g_hThreadQuitEvent = NULL;

std::string g_sBaseName = "DX9IFR_SharedSurfaceHWEncode"; // Default Basename for the output stream

/*! 
 * Initialize the NvIFR device
 */
void InitNVIFR()
{
    //! Load the NvIFR.dll library
    if(NULL == (g_hNvIFRDll = g_NvIFRlib.load()))
    {
        MessageBox( NULL, "Unable to load the NvIFR library\r\n", SAMPLE_NAME, MB_OK );
        ExitProcess(0);
    }
        
    NvIFR_CreateSharedSurface_fn = (NvIFR_CreateSharedSurfaceEXTFunctionType) GetProcAddress(g_hNvIFRDll, "NvIFR_CreateSharedSurfaceEXT");
    NvIFR_DestroySharedSurface_fn = (NvIFR_DestroySharedSurfaceEXTFunctionType) GetProcAddress(g_hNvIFRDll, "NvIFR_DestroySharedSurfaceEXT");
    NvIFR_CopyToSharedSurface_fn = (NvIFR_CopyToSharedSurfaceEXTFunctionType) GetProcAddress(g_hNvIFRDll, "NvIFR_CopyToSharedSurfaceEXT");
    NvIFR_CopyFromSharedSurface_fn = (NvIFR_CopyFromSharedSurfaceEXTFunctionType) GetProcAddress(g_hNvIFRDll, "NvIFR_CopyFromSharedSurfaceEXT");
    
    if (!NvIFR_CreateSharedSurface_fn || !NvIFR_DestroySharedSurface_fn || !NvIFR_CopyToSharedSurface_fn || !NvIFR_CopyFromSharedSurface_fn )
    {
        MessageBox( NULL, "Unable to load the NvIFR Shared Surface Entrypoints\r\n", SAMPLE_NAME, MB_OK );
        ExitProcess(0);
    }
    
    g_hCanRenderEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    g_hCanEncodeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
 }

/*! 
 * Creates the D3D9 Device and get render target.
 */
HRESULT InitD3D( HWND hWnd )
{
    //! Create the D3D object.
    if( NULL == ( g_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
        return E_FAIL;

    D3DDISPLAYMODE d3ddm;

    if( FAILED( g_pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm ) ) )
        return E_FAIL;

    //! Set up the structure used to create the D3DDevice.
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof( d3dpp ) );
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth  = g_dwWidth;
    d3dpp.BackBufferHeight = g_dwHeight;
    d3dpp.BackBufferFormat = d3ddm.Format;  
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    //! Create the D3DDevice
    if( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &d3dpp, &g_pD3DDeviceR ) ) )
    {
        return E_FAIL;
    }

    //! Turn off culling
    g_pD3DDeviceR->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

    //! Turn off D3D lighting
    g_pD3DDeviceR->SetRenderState( D3DRS_LIGHTING, FALSE );

    //! Turn on the zbuffer
    g_pD3DDeviceR->SetRenderState( D3DRS_ZENABLE, TRUE );

    return S_OK;
}

/*
 * Locate file
 */
std::string LocateFile ( std::string fname ) 
{
    FILE* fp;
    char filename[1024];

    // Search for a resource
    sprintf ( filename, "%s", fname.c_str() );
    if ( fp = fopen(filename, "r") ) {
        fclose(fp);
        return filename;
    }
    sprintf ( filename, "..\\media\\%s", fname.c_str() );
    if ( fp = fopen(filename, "r") )
    {
        fclose(fp);
        return filename;
    }
    sprintf ( filename, "..\\..\\DirectxIFR\\media\\%s", fname.c_str() );
    if ( fp = fopen(filename, "r") )
    {
        fclose(fp);
        return filename;
    }
    return "";
}

/*! 
 * Load a texture and generate geometry.
 */
HRESULT InitGeometry()
{
    //! Create the vertex buffer.
    if( FAILED( g_pD3DDeviceR->CreateVertexBuffer( 50 * 2 * sizeof( CUSTOMVERTEX ),
                                                  0, D3DFVF_CUSTOMVERTEX,
                                                  D3DPOOL_DEFAULT, &g_pD3DVB, NULL ) ) )
    {
        return E_FAIL;
    }

    //! Fill the vertex buffer.
    CUSTOMVERTEX* pVertices;
    if( FAILED( g_pD3DVB->Lock( 0, 0, ( void** )&pVertices, 0 ) ) )
        return E_FAIL;
    for( DWORD i = 0; i < 50; i++ )
    {
        FLOAT theta = ( 2 * D3DX_PI * i ) / ( 50 - 1 );

        pVertices[2 * i + 0].position = D3DXVECTOR3( sinf( theta ), -1.0f, cosf( theta ) );
        pVertices[2 * i + 0].color = 0xfffff000;
        pVertices[2 * i + 0].tu = ( ( FLOAT )i ) / ( 50 - 1 );
        pVertices[2 * i + 0].tv = 1.0f;

        pVertices[2 * i + 1].position = D3DXVECTOR3( sinf( theta ), 1.0f, cosf( theta ) );
        pVertices[2 * i + 1].color = 0xff808080;
        pVertices[2 * i + 1].tu = ( ( FLOAT )i ) / ( 50 - 1 );
        pVertices[2 * i + 1].tv = 0.0f;
    }
    g_pD3DVB->Unlock();

    return S_OK;
}

/*! 
 * Release D3D all textures, buffes, and devices
 */
VOID Cleanup()
{

    if( g_pD3DVB )
    {
        g_pD3DVB->Release();
        g_pD3DVB = NULL;
    }

    if( g_pD3DDeviceR)
    {
        g_pD3DDeviceR->Release();
        g_pD3DDeviceR = NULL;
    }

    if( g_pD3D)
    {
        g_pD3D->Release();
        g_pD3D = NULL;
    }
        
    if (fileOut)
    {
        fclose(fileOut);
        fileOut = NULL;
    }
}

/*! 
 * Setup world, view and projection matrices
 */
VOID SetupMatrices()
{
    // Set up world matrix
    D3DXMATRIXA16 matWorld;
    D3DXMatrixIdentity( &matWorld );
    static float xrotation = 0.0f; 
    xrotation+=0.05f;    // an ever-increasing float value
    // build a matrix to rotate the model based on the increasing float value
    D3DXMatrixRotationX( &matWorld, xrotation );
    g_pD3DDeviceR->SetTransform( D3DTS_WORLD, &matWorld );

    // Set up our view matrix. A view matrix can be defined given an eye point,
    // a point to lookat, and a direction for which way is up. Here, we set the
    // eye five units back along the z-axis and up three units, look at the
    // origin, and define "up" to be in the y-direction.
    D3DXVECTOR3 vEyePt( 0.0f, 3.0f,-5.0f );
    D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
    g_pD3DDeviceR->SetTransform( D3DTS_VIEW, &matView );

    // For the projection matrix, we set up a perspective transform (which
    // transforms geometry from 3D view space to 2D viewport space, with
    // a perspective divide making objects smaller in the distance). To build
    // a perpsective transform, we need the field of view (1/4 pi is common),
    // the aspect ratio, and the near and far clipping planes (which define at
    // what distances geometry should be no longer be rendered).
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f );
    g_pD3DDeviceR->SetTransform( D3DTS_PROJECTION, &matProj );
}



unsigned char * pBitStreamBuffer = NULL;
HANDLE EncodeCompletionEvent=NULL;

/*! 
 * Windows messaging handler
 */
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
		case WM_KEYDOWN:
        case WM_DESTROY:
            Cleanup();
            PostQuitMessage( 0 );
            return 0;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}

/*!
 * Setup Encoding Thread onjects
 */
void SetupEncodeThreadObjects()
{
    //! Register the window class
    WNDCLASSEX wc =
    {
        sizeof( WNDCLASSEX ), CS_CLASSDC, DefWindowProc, 0L, 0L,
        GetModuleHandle( NULL ), NULL, NULL, NULL, NULL,
        "encoding", NULL
    };
    RegisterClassEx( &wc );

    //! Create the application's window
    HWND hWnd = CreateWindow( "encoding", "shared texture sample",
                              WS_OVERLAPPEDWINDOW, 100, 100, g_dwWidth, g_dwHeight,
                              NULL, NULL, wc.hInstance, NULL );


    D3DDISPLAYMODE d3ddm;

    if( FAILED( g_pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm ) ) )
        exit(-9);

    //! Set up the structure used to create the D3DDevice. 
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof( d3dpp ) );
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth  = g_dwWidth;
    d3dpp.BackBufferHeight = g_dwHeight;
    d3dpp.BackBufferFormat = d3ddm.Format;  
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    //! Create the D3DDevice
    if( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &d3dpp, &g_pD3DDeviceE ) ) )
    {
        exit(-10);
    }

    g_pD3DDeviceE->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    g_pD3DDeviceE->SetRenderState( D3DRS_LIGHTING, FALSE );
    g_pD3DDeviceE->SetRenderState( D3DRS_ZENABLE, FALSE );

    //! Create the INvIFRToHWEncoder_v1 object
    g_pIFR = (INvIFRToHWEncoder_v1 *)g_NvIFRlib.create(g_pD3DDeviceE, NVIFR_TO_HWENCODER);
    
    g_hThreadQuitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}


/*! 
 * Render the scene
 */
VOID Render()
{
    //! Clear the backbuffer and the zbuffer
    g_pD3DDeviceR->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                         D3DCOLOR_XRGB( 0, 0, 255 ), 1.0f, 0 );

    //! Begin the scene
    if( SUCCEEDED( g_pD3DDeviceR->BeginScene() ) )
    {
	//! Setup the world, view, and projection matrices
        SetupMatrices();

        //! Render the vertex buffer contents
        g_pD3DDeviceR->SetStreamSource( 0, g_pD3DVB, 0, sizeof( CUSTOMVERTEX ) );
        g_pD3DDeviceR->SetFVF( D3DFVF_CUSTOMVERTEX );

        g_pD3DDeviceR->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 * 50 - 2 );

        //! End the scene
        g_pD3DDeviceR->EndScene();
    }

    IDirect3DSurface9* pRenderTarget;
  
	//! Get the render target
    g_pD3DDeviceR->GetRenderTarget( 0, & pRenderTarget );

    //! Wait for encoding thread to complete data copy operation from the shared surface.
    //! This sync is required to guarantee that each presented frame is fed to the encoder.
    //! Allocate multiple shared surfaces for better perf\less waiting.
    WaitForSingleObject(g_hCanRenderEvent, INFINITE);
    ResetEvent(g_hCanRenderEvent);
    
	//! Copy the render buffer into a shared surface
    BOOL bRet = NvIFR_CopyToSharedSurface_fn( g_pD3DDeviceR, g_hIFRSharedSurface, pRenderTarget);

    pRenderTarget->Release();
    g_dwFrameCnt++;
    
    //! Wake up the encoding thread to grab the shared surface.
    SetEvent(g_hCanEncodeEvent);
    
    //! Present the backbuffer contents to the display
    g_pD3DDeviceR->Present( NULL, NULL, NULL, NULL );
}

/*!
 * Encode shared image.
 */
 
 void encodeFrame(FILE *fileOut)
 {
    IDirect3DSurface9* pRenderTarget;
    NVIFRRESULT res = NVIFR_SUCCESS;
    g_pD3DDeviceE->GetRenderTarget( 0, & pRenderTarget );

    //! Copy from the shared surface (not directly from main thread)
    BOOL bRet = NvIFR_CopyFromSharedSurface_fn( g_pD3DDeviceE, g_hIFRSharedSurface, pRenderTarget);

    SetEvent(g_hCanRenderEvent);
    pRenderTarget->Release();

    //! Transfer the shared surface to the H.264 Encoder
    NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS params = {0};
    params.dwVersion = NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS_VER;
    params.dwBufferIndex = 0;
    params.encodePicParams.dwVersion = NV_HW_ENC_PIC_PARAMS_VER;
    res = g_pIFR->NvIFRTransferRenderTargetToHWEncoder(&params);
    
    if (res == NVIFR_SUCCESS)    
    {
        //! Save frame to disk
        //! the wait here is just an example, in the case of disk IO, 
        //! it forces the current transfer to be done, not taking advantage of CPU/GPU concurrency
        WaitForSingleObject(EncodeCompletionEvent, INFINITE);
        ResetEvent(EncodeCompletionEvent);

        NVIFR_HW_ENC_GET_BITSTREAM_PARAMS params = {0};
        params.dwVersion = NVIFR_HW_ENC_GET_BITSTREAM_PARAMS_VER;
        params.dwBufferIndex = 0;
        params.bitStreamParams.dwVersion = NV_HW_ENC_GET_BIT_STREAM_PARAMS_VER;

        res = g_pIFR->NvIFRGetStatsFromHWEncoder(&params);
        if (res == NVIFR_SUCCESS)
            fwrite(pBitStreamBuffer, params.bitStreamParams.dwByteSize, 1, fileOut);
        
        //! (see other sdk sample for asynchronous encoding, so the GPU and CPU would not stall at all)
    }
 }
 
/*! 
 * Create an H.264 encoding thread
 */
void encodingThread(void * pData)
{
    if(NULL == g_pIFR)
    {
        g_bStopEncoding = TRUE;
        g_bIFRNotCreated = TRUE;
        _endthread();
    }

	 //! Setup the H.264 encoder
    DWORD dwBitRate720p = 5000000;
    double  dBitRate = double(dwBitRate720p)*(double(g_dwWidth*g_dwHeight)/double(1280*720));           // scaling bitrate with resolution

    std::string fileName;
    if (g_Codec == NV_HW_ENC_HEVC)
    {
        fileName = g_sBaseName + ".h265";
        fileOut = fopen(fileName.c_str(), "wb");
    }
    else
    {
        fileName = g_sBaseName + ".h264";
        fileOut = fopen(fileName.c_str(), "wb");
    }

    if (!fileOut)
    {
        MessageBox(NULL, "Failed to create output file.\n", "Error", MB_OK);
    }

    NVIFR_HW_ENC_SETUP_PARAMS params = {0};
    params.dwVersion = NVIFR_HW_ENC_SETUP_PARAMS_VER;
    params.dwNBuffers = 1;
    params.dwBSMaxSize = 2048*1024;
    params.ppPageLockedBitStreamBuffers = &pBitStreamBuffer;
    params.ppEncodeCompletionEvents = &EncodeCompletionEvent;
    
    params.configParams.dwVersion = NV_HW_ENC_CONFIG_PARAMS_VER;
    params.configParams.eCodec    = g_Codec;
    params.configParams.eStereoFormat = NV_HW_ENC_STEREO_NONE;
    params.configParams.dwProfile = 100;
    params.configParams.dwAvgBitRate = (DWORD)dBitRate;
    params.configParams.dwFrameRateDen = 1;
    params.configParams.dwFrameRateNum = 30;
    params.configParams.dwPeakBitRate = (params.configParams.dwAvgBitRate * 12/10);   // +20%
    params.configParams.dwGOPLength = 75;
    params.configParams.dwQP = 26 ;
    params.configParams.eRateControl = NV_HW_ENC_PARAMS_RC_CBR;
    params.configParams.ePresetConfig= NV_HW_ENC_PRESET_LOW_LATENCY_DEFAULT;

    NVIFRRESULT res = g_pIFR->NvIFRSetUpHWEncoder(&params);
    if (res != NVIFR_SUCCESS)
    {
        if (res == NVIFR_ERROR_INVALID_PARAM || res != NVIFR_ERROR_INVALID_PTR)
            MessageBox(NULL, "NvIFR Buffer creation failed due to invalid params.\n", SAMPLE_NAME, MB_OK );
        else
            MessageBox( NULL, "Something is wrong with the driver, cannot initialize IFR buffers\n", SAMPLE_NAME, MB_OK );

        g_bStopEncoding = TRUE;
    }

    DWORD dwFramesDone = 0;
    DWORD dwEventID = 0;
    DWORD dwPendingFrames = 0;
    HANDLE pEvents[2] = {g_hThreadQuitEvent, g_hCanEncodeEvent};
    BOOL bQuit = FALSE;

    //! Loop while encoding
    while(1)
    {
        dwEventID = WaitForMultipleObjects(2, pEvents, FALSE, INFINITE);
        if (dwEventID - WAIT_OBJECT_0 == 0)
        {
            //! The main thread has signalled us to quit.
            //! Check if there is a frame pending, else break out.
            dwPendingFrames = g_dwFrameCnt - dwFramesDone;
            
            // dwPending should at most be 1 for this sample, since there are no multiple in-flight NvIFR buffers.
            for (DWORD i =0; i < dwPendingFrames; i++)
            {
                WaitForSingleObject(g_hCanEncodeEvent, INFINITE);
                ResetEvent(g_hCanEncodeEvent);
                encodeFrame(fileOut);
                dwFramesDone++;
            }
            break;
        }
        
        ResetEvent(g_hCanEncodeEvent);
        encodeFrame(fileOut);
        dwFramesDone++;
    }

    if (fileOut)
        fclose(fileOut);
        
    _endthread();
}

/*! 
 * WinMain
 */
INT WINAPI wWinMain( HINSTANCE hInst, HINSTANCE, LPWSTR cmdline, INT )
{
    //! Register the window class
    WNDCLASSEX wc =
    {
        sizeof( WNDCLASSEX ), CS_CLASSDC, MsgProc, 0L, 0L,
        GetModuleHandle( NULL ), NULL, NULL, NULL, NULL,
        "Textures HW Encode with shared surface", NULL
    };
    RegisterClassEx( &wc );

    //! Create the application's window
    HWND hWnd = CreateWindow( "Textures HW Encode with shared surface", "Textures HW Encode with shared surface",
                              WS_OVERLAPPEDWINDOW, 100, 100, g_dwWidth, g_dwHeight,
                              NULL, NULL, wc.hInstance, NULL );

    DWORD dwNvIFRVersion=0;
    NvIFRLibrary NvIFRlib;
    BOOL bRet = TRUE;
    //! Initialize Direct3D
    if( SUCCEEDED( InitD3D( hWnd ) ) )
    {
        InitNVIFR();
        SetupEncodeThreadObjects();
        bRet = NvIFR_CreateSharedSurface_fn(g_pD3DDeviceR, g_dwWidth, g_dwHeight, &g_hIFRSharedSurface);
        if (!bRet)
        {
            printf("Something is wrong with the driver. cannot initialize NvIFR surface sharing.\n");
            exit(-1);
        }
    
        HANDLE hEncodeThread = (HANDLE)_beginthread(encodingThread, 0, 0);

        // Create the scene geometry
        if( SUCCEEDED( InitGeometry() ) )
        {
            //! Show the window
            ShowWindow( hWnd, SW_SHOWDEFAULT );
            UpdateWindow( hWnd );

            //! Enter the message loop
            MSG msg = {0};
            while( msg.message != WM_QUIT && msg.message != WM_DESTROY )
            {
                if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
                {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                }
                else
                {
                    if (g_dwFrameCnt < g_dwMaxFrames)
                    {
                        Render();
                    }
                    else
                    {
                        SetEvent(g_hThreadQuitEvent);
                        WaitForSingleObject(hEncodeThread, INFINITE);
                        PostQuitMessage(0);
                    }
                }
            }
        }

        if (g_bIFRNotCreated)
        {
            MessageBox( NULL, "Failed to create INvIFRToHWEncoder_v1\r\n", SAMPLE_NAME, MB_OK);
        }
        if (g_pIFR && g_pD3DDeviceR && g_hIFRSharedSurface)
        {
            bRet = NvIFR_DestroySharedSurface_fn(g_pD3DDeviceR, g_hIFRSharedSurface);
        }
    }
	
	//! Free everything
	if (g_pIFR)
	{
        g_pIFR->NvIFRRelease();
        g_pIFR = NULL;
    }
    
    if (g_hNvIFRDll)
    {
        FreeLibrary(g_hNvIFRDll);
    }    

    Cleanup();

    UnregisterClass( "Textures HW Encode with shared surface", wc.hInstance );

    return 0;
}

// Prints the help message
void printHelp()
{
    printf("Usage: %s [options]\n", SAMPLE_NAME);
    printf("  -frames framecnt           The number of frames to grab, defaults to %d.\n", g_dwMaxFrames);
    printf("  -codec codectype           The codec to use for encode. 0 = H.264, 1 = HEVC\n");
	printf("  -width width               Width of the surface to capture\n");
	printf("  -height height             Height of the surface to capture\n");
    printf("  -output OutputFileName     Custom name for bitstream output without file extension.\n");
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

            // Must grab at least one frame.
            if(g_dwMaxFrames < 1)
                g_dwMaxFrames = 1;
        }        
        else if(0 == _stricmp(argv[i], "-output"))
        {
            ++i;
            if(i >= argc)
            {
                printf("Missing -output option\n");
                printHelp();
                return -1;
            }
            g_sBaseName = argv[i];
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
		else if (0 == _stricmp(argv[i], "-width"))
        {
            ++i;
            if(i >= argc)
            {
                printf("Missing -width option\n");
                printHelp();
                return -1;
            }
            g_dwWidth = atoi(argv[i]);
        }
		else if (0 == _stricmp(argv[i], "-height"))
        {
            ++i;
            if(i >= argc)
            {
                printf("Missing -height option\n");
                printHelp();
                return -1;
            }
            g_dwHeight = atoi(argv[i]);
        }
    }

    return wWinMain(inst, NULL, NULL, SW_SHOWNORMAL);
}

