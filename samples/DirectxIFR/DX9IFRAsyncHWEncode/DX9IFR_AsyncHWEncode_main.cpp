/*!
 * \brief
 * Shows how to use NvIFR to grab and encode a render frame from DX9 and 
 * D11 asynchronously, rather than block the render thread
 *
 * \file
 *
 * This DX9 sample demonstrates grabbing and encoding a frame asynchronously using the 
 * NvIFR interface, encoding it to H.264, and writing to a video file. Rather than 
 * blocking the render thread waiting for the grab + encode to finish, it continues 
 * rendering and signals another thread when the encoded frame is ready.  
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <Windows.h>
#include <mmsystem.h>
#include <d3dx9.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <process.h>
#include <NvIFR/NvIFR.h>
#include <NvIFR/NvIFRHWEnc.h>
#include <NvIFRLibrary.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#pragma warning( disable : 4995 )
#include <strsafe.h>

#define SAMPLE_NAME "DX9IFR_AsyncHWEncode"

//! pointer to the INvIFRToHWEncoder_v1 object
INvIFRToHWEncoder_v1 * g_pIFR=NULL;
//! handle to the NvIFR DLL
HINSTANCE g_hNvIFRDll=NULL;
//! the D3D Device
LPDIRECT3D9             g_pD3D = NULL; 
LPDIRECT3DDEVICE9       g_pD3DDevice = NULL;
LPDIRECT3DVERTEXBUFFER9 g_pD3DVB = NULL; 

NV_HW_ENC_CODEC          g_Codec  = NV_HW_ENC_H264;
bool                     g_bYUV444 = false;

//! a structure for custom vertex type (pos, color, uv)
struct CUSTOMVERTEX
{
    D3DXVECTOR3 position; // The position
    D3DCOLOR color;    // The color
#ifndef SHOW_HOW_TO_USE_TCI
    FLOAT tu, tv;   // The texture coordinates
#endif
};

//! custom FVF
#ifdef SHOW_HOW_TO_USE_TCI
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)
#else
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)
#endif

//! size of backbuffer/encoded stream
int g_dwWidth=1920;
int g_dwHeight=1080;
DWORD g_dwMaxFrames = 30;

#define NUMFRAMESINFLIGHT   3

//! event handles used to pass off rendering
unsigned char * g_apBitStreamBuffers[NUMFRAMESINFLIGHT];
HANDLE g_aEncodeCompletionEvents[NUMFRAMESINFLIGHT];
HANDLE g_aCanRenderEvents[NUMFRAMESINFLIGHT];
HANDLE g_hThreadQuitEvent = NULL;
HANDLE g_hFileWriterThreadHandle = NULL;

DWORD g_dwFrameNumber = 0;

std::string g_sBaseName = "DX9IFR_AsyncHWEncode"; // Default Basename for the output stream

/*! 
 * Initialize the NvIFR device
 */
void InitNVIFR()
{
    NvIFRLibrary NvIFRlib;
    //! Next, load the NvIFR.dll library
    if(NULL == (g_hNvIFRDll = NvIFRlib.load()))
    {
        MessageBox( NULL, "Unable to load the NvIFR library\r\n", SAMPLE_NAME, MB_OK );
        exit(1);
    }

    //! Create the INvIFRToHWEncoder_v1 object
    g_pIFR = (INvIFRToHWEncoder_v1 *)NvIFRlib.create(g_pD3DDevice, NVIFR_TO_HWENCODER);

    if(NULL == g_pIFR)
    {
        MessageBox( NULL, "Failed to create INvIFRToHWEncoder_v1\r\n", SAMPLE_NAME, MB_OK);
        exit(1);
    }

    for (DWORD i = 0; i < NUMFRAMESINFLIGHT; i++)
    {
        //! Create the events for allowing rendering to continue after a capture is complete
        g_aCanRenderEvents[i] = CreateEvent(NULL, TRUE, TRUE, NULL);	
    }
    g_hThreadQuitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

/*! 
 * Creates the D3D9 Device.
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
                                      &d3dpp, &g_pD3DDevice ) ) )
    {
        return E_FAIL;
    }

    //! Turn off culling
    g_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

    //! Turn off D3D lighting
    g_pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE );

    //! Turn on the zbuffer
    g_pD3DDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

    return S_OK;
}




/*! 
 * Initialize Geometry
 * Load a texture and generate vertices.
 */
HRESULT InitGeometry()
{
    // Create the vertex buffer.
    if( FAILED( g_pD3DDevice->CreateVertexBuffer( 50 * 2 * sizeof( CUSTOMVERTEX ),
                                                  0, D3DFVF_CUSTOMVERTEX,
                                                  D3DPOOL_DEFAULT, &g_pD3DVB, NULL ) ) )
    {
        return E_FAIL;
    }

    // Fill the vertex buffer. We are setting the tu and tv texture
    // coordinates, which range from 0.0 to 1.0
    CUSTOMVERTEX* pVertices;
    if( FAILED( g_pD3DVB->Lock( 0, 0, ( void** )&pVertices, 0 ) ) )
        return E_FAIL;
    for( DWORD i = 0; i < 50; i++ )
    {
        FLOAT theta = ( 2 * D3DX_PI * i ) / ( 50 - 1 );

        pVertices[2 * i + 0].position = D3DXVECTOR3( sinf( theta ), -1.0f, cosf( theta ) );
        pVertices[2 * i + 0].color = 0xfffff000;
#ifndef SHOW_HOW_TO_USE_TCI
        pVertices[2 * i + 0].tu = ( ( FLOAT )i ) / ( 50 - 1 );
        pVertices[2 * i + 0].tv = 1.0f;
#endif

        pVertices[2 * i + 1].position = D3DXVECTOR3( sinf( theta ), 1.0f, cosf( theta ) );
        pVertices[2 * i + 1].color = 0xff808080;
#ifndef SHOW_HOW_TO_USE_TCI
        pVertices[2 * i + 1].tu = ( ( FLOAT )i ) / ( 50 - 1 );
        pVertices[2 * i + 1].tv = 0.0f;
#endif
    }
    g_pD3DVB->Unlock();

    return S_OK;
}

/*! 
 * Release D3D all textures, buffes, and devices
 */
VOID Cleanup()
{
    if( g_pD3DVB != NULL )
        g_pD3DVB->Release();

    if( g_pD3DDevice != NULL )
        g_pD3DDevice->Release();

    if( g_pD3D != NULL )
        g_pD3D->Release();
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
    g_pD3DDevice->SetTransform( D3DTS_WORLD, &matWorld );

    // Set up our view matrix. A view matrix can be defined given an eye point,
    // a point to lookat, and a direction for which way is up. Here, we set the
    // eye five units back along the z-axis and up three units, look at the
    // origin, and define "up" to be in the y-direction.
    D3DXVECTOR3 vEyePt( 0.0f, 3.0f,-5.0f );
    D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
    g_pD3DDevice->SetTransform( D3DTS_VIEW, &matView );

    // For the projection matrix, we set up a perspective transform (which
    // transforms geometry from 3D view space to 2D viewport space, with
    // a perspective divide making objects smaller in the distance). To build
    // a perpsective transform, we need the field of view (1/4 pi is common),
    // the aspect ratio, and the near and far clipping planes (which define at
    // what distances geometry should be no longer be rendered).
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f );
    g_pD3DDevice->SetTransform( D3DTS_PROJECTION, &matProj );
}

/*! 
 * Render the scene. If there is no target buffer, one is created and
 * we kick off a thread that will save recorded frame buffers to BMP.
 * On every frame, we wait for the render to finish, and then 
 * call TransferRenderTargetToSys to grab the frame and send to 
 * the recording thread.
 */
VOID Render()
{
    //! Clear the backbuffer and the zbuffer
    g_pD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                         D3DCOLOR_XRGB( 0, 0, 255 ), 1.0f, 0 );

    //! Render the scene
    if( SUCCEEDED( g_pD3DDevice->BeginScene() ) )
    {
        //! Setup the world, view, and projection matrices
        SetupMatrices();

        //! Render the vertex buffer contents
        g_pD3DDevice->SetStreamSource( 0, g_pD3DVB, 0, sizeof( CUSTOMVERTEX ) );
        g_pD3DDevice->SetFVF( D3DFVF_CUSTOMVERTEX );

        for (DWORD i=0; i<100; i++)
            g_pD3DDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 * 50 - 2 );

        //! End the scene
        g_pD3DDevice->EndScene();
    }

    static DWORD dwBufferIndex=0;

	//! Now wait for the render to complete
    WaitForSingleObject(g_aCanRenderEvents[dwBufferIndex], INFINITE);
    ResetEvent(g_aCanRenderEvents[dwBufferIndex]);

	//! Asynchronously transfer the render target frame to the H.264 encoder
	NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS params = {0};
	params.dwVersion = NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS_VER;
	params.dwBufferIndex = dwBufferIndex;
	params.encodePicParams.dwVersion = NV_HW_ENC_PIC_PARAMS_VER;
    NVIFRRESULT res = g_pIFR->NvIFRTransferRenderTargetToHWEncoder(&params);

    //! Present the backbuffer contents to the display
    g_pD3DDevice->Present( NULL, NULL, NULL, NULL );

    dwBufferIndex = (dwBufferIndex+1)%NUMFRAMESINFLIGHT;
}

/*! 
 * Windows messaging handler
 */
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


/*! 
 * Write the H.264 stream to disk
 */
void GetBitStream(unsigned int bufferIndex, FILE *fileOut)
{
    NVIFR_HW_ENC_GET_BITSTREAM_PARAMS params = {0};
    params.dwVersion = NVIFR_HW_ENC_GET_BITSTREAM_PARAMS_VER;
    params.dwBufferIndex = bufferIndex;
    params.bitStreamParams.dwVersion = NV_HW_ENC_GET_BIT_STREAM_PARAMS_VER;
    BOOL bRet=g_pIFR->NvIFRGetStatsFromHWEncoder(&params);

    //! Write frame to disk
    fwrite(g_apBitStreamBuffers[bufferIndex], params.bitStreamParams.dwByteSize, 1, fileOut);
}

/*! 
 * Run a thread for writing the video file to disk
 */
void fileWriterThread(void *data)
{
    DWORD bufferIndex = 0;
    FILE *fileOut = NULL;
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

    if(!fileOut)
        return;

    HANDLE hEvents[2];
    hEvents[0] = g_hThreadQuitEvent;
    DWORD dwEventID = 0;
    DWORD dwPendingFrames = 0;
    DWORD dwCapturedFrames = 0;
    

    //! While the render loop is still running
    while (1)	
    {
        hEvents[1] = g_aEncodeCompletionEvents[bufferIndex];
        //! Wait for the capture completion event for this buffer
        dwEventID = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
        if (dwEventID - WAIT_OBJECT_0 == 0)
        {
            //! The main thread has signalled us to quit.
            //! Check if there is any pending work and finish it before quitting.
            dwPendingFrames = (g_dwMaxFrames > dwCapturedFrames) ? (g_dwMaxFrames - dwCapturedFrames) : 0;
            for (DWORD i = 0; i < dwPendingFrames; i++)
            {
                WaitForSingleObject(g_aEncodeCompletionEvents[bufferIndex], INFINITE);
                ResetEvent(g_aEncodeCompletionEvents[bufferIndex]);       // optional

                //! Fetch bitstream from NvIFR and dump to disk
                GetBitStream(bufferIndex, fileOut);
                dwCapturedFrames++;
                //! Wait on next index for new data
                bufferIndex = (bufferIndex+1)%NUMFRAMESINFLIGHT;
            }
            break;
        }
        ResetEvent(g_aEncodeCompletionEvents[bufferIndex]);       // optional

        //! Fetch bitstream from NvIFR and dump to disk
        GetBitStream(bufferIndex, fileOut);
        dwCapturedFrames++;
        
        //! Continue rendering on this index
        SetEvent(g_aCanRenderEvents[bufferIndex]);

        //! Wait on next index for new data
        bufferIndex = (bufferIndex+1)%NUMFRAMESINFLIGHT;
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
    HANDLE hFileWriterThreadHandle=NULL;

    //! Register the window class
    WNDCLASSEX wc =
    {
        sizeof( WNDCLASSEX ), CS_CLASSDC, MsgProc, 0L, 0L,
        GetModuleHandle( NULL ), NULL, NULL, NULL, NULL,
        SAMPLE_NAME, NULL
    };
    RegisterClassEx( &wc );

    //! Create the application's window
    HWND hWnd = CreateWindow( SAMPLE_NAME, SAMPLE_NAME,
                              WS_OVERLAPPEDWINDOW, 100, 100, 640, 480,
                              NULL, NULL, wc.hInstance, NULL );

    DWORD dwNvIFRVersion=0;

    //! Initialize Direct3D
    if( SUCCEEDED( InitD3D( hWnd ) ) )
    {
        InitNVIFR();

        DWORD dwBitRate720p = 5000000;
        double  dBitRate = double(dwBitRate720p)*(double(g_dwWidth*g_dwHeight)/double(1280*720));           // scaling bitrate with resolution
        NV_HW_ENC_CONFIG_PARAMS encodeConfig = {0};


        NVIFR_HW_ENC_SETUP_PARAMS params = {0};
        params.dwVersion = NVIFR_HW_ENC_SETUP_PARAMS_VER;
        params.dwNBuffers = NUMFRAMESINFLIGHT;
        params.dwBSMaxSize = 2048*1024;
        params.ppPageLockedBitStreamBuffers = g_apBitStreamBuffers;
        params.ppEncodeCompletionEvents = g_aEncodeCompletionEvents;

        params.configParams.dwVersion = NV_HW_ENC_CONFIG_PARAMS_VER;
        params.configParams.eCodec    = g_Codec;
        params.configParams.dwProfile = 100;
        params.configParams.dwAvgBitRate = (DWORD)dBitRate;
        params.configParams.dwFrameRateDen = 1;
        params.configParams.dwFrameRateNum = 30;
        params.configParams.dwPeakBitRate = (params.configParams.dwAvgBitRate * 12/10);   // +20%
        params.configParams.dwGOPLength = 75;
        params.configParams.dwQP = 26 ;
        params.configParams.eRateControl = NV_HW_ENC_PARAMS_RC_2_PASS_QUALITY;
        params.configParams.ePresetConfig= NV_HW_ENC_PRESET_LOW_LATENCY_HP;
        params.configParams.bEnableYUV444Encoding = g_bYUV444 ? 1 : 0;
        // Set Single frame VBV buffer size
        params.configParams.dwVBVBufferSize = (params.configParams.dwAvgBitRate)/(params.configParams.dwFrameRateNum/params.configParams.dwFrameRateDen);
        NVIFRRESULT res = g_pIFR->NvIFRSetUpHWEncoder(&params);
        if (res != NVIFR_SUCCESS)
        {
            if (res == NVIFR_ERROR_INVALID_PARAM || res != NVIFR_ERROR_INVALID_PTR)
                MessageBox(NULL, "NvIFR Buffer creation failed due to invalid params.\n", SAMPLE_NAME, MB_OK );
            else
                MessageBox( NULL, "Something is wrong with the driver, cannot initialize IFR buffers\n", SAMPLE_NAME, MB_OK );
            goto exit;
        }

        g_hFileWriterThreadHandle=(HANDLE)_beginthread(fileWriterThread, 0, 0);

        //! Create the scene geometry
        if( SUCCEEDED( InitGeometry() ) )
        {
            //! Show the window
            ShowWindow( hWnd, SW_SHOWDEFAULT );
            UpdateWindow( hWnd );

            //! Enter the message loop
            MSG msg;
            ZeroMemory( &msg, sizeof( msg ) );
            while( msg.message != WM_QUIT && msg.message != WM_DESTROY )
            {
                if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
                {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                }
                else
                {
                    if (g_dwFrameNumber < g_dwMaxFrames)
                    {
                        Render();
                        g_dwFrameNumber++;
                    }
                    else
                    {
                        SetEvent(g_hThreadQuitEvent);
                        //! Free everything after the side thread finishes up
                        WaitForSingleObject(g_hFileWriterThreadHandle, INFINITE);
                        PostQuitMessage(0);
                    }
                }
            }
        }
    }

exit:
    if (g_pIFR)
    {
        g_pIFR->NvIFRRelease();
        g_pIFR = NULL;
    }

    if (g_hNvIFRDll)
        FreeLibrary(g_hNvIFRDll);

    UnregisterClass( "Textures HWEncode Async", wc.hInstance );
    return 0;
}

// Prints the help message
void printHelp()
{
    printf("Usage: %s [options]\n", SAMPLE_NAME);
    printf("  -frames framecnt           The number of frames to grab, defaults to %d.\n", g_dwMaxFrames);
    printf("  -codec codectype           The codec to use for encode. 0 = H.264, 1 = HEVC");
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
        else if (0 == _stricmp(argv[i], "-yuv444"))
        {
            g_bYUV444 = true;
        }
        else
        {
            printf("Unknown parameter %s\n", argv[i]);
        }
    }

    return wWinMain(inst, NULL, NULL, SW_SHOWNORMAL);
}

