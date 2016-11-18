/*!
 * \brief
 * Shows how to use NvIFR to grab and encode a render frame from D10 asynchronously, 
 * rather than block the render thread.
 *
 * \file
 *
 * This DX10 sample demonstrates grabbing and encoding a frame asynchronously using 
 * the NvIFR interface, encoding it to H.264, and writing to  a video file. Rather 
 * than blocking the render thread waiting for the grab + encode to finish, it 
 * continues rendering and signals another thread when the encoded frame is ready.  
 *
 * \copyright
 * * Copyright 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma warning( disable : 4996 )
#include <windows.h>
#include <d3d10.h>
#include <d3dx10.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <process.h>
#include "resource.h"

#define SAMPLE_NAME "DX10IFR_AsyncHWEncode"

#include <NvIFR/NvIFR.h>
#include <NvIFR/NvIFRHWEnc.h>
#include <NvIFRLibrary.h>

struct SimpleVertex
{
    D3DXVECTOR3 Pos;
    D3DXVECTOR4 Color;
};

//! pointer to INvIFRToHWEncoder_v1 object
INvIFRToHWEncoder_v1 *g_pIFR=NULL;

DWORD g_dwWidth=0;
DWORD g_dwHeight=0;
BOOL  g_bUseYUV444=FALSE;

std::string g_sBaseName = "DX10IFR_AsyncHWEncode"; // Default Basename for the output stream

//! pointer to Instance and Window
HINSTANCE                   g_hInst = NULL;
HWND                        g_hWnd = NULL;
D3D10_DRIVER_TYPE           g_driverType = D3D10_DRIVER_TYPE_NULL;

//! pointer to D3D10 Device
ID3D10Device*               g_pD3DDevice = NULL;
IDXGISwapChain*             g_pSwapChain = NULL;
ID3D10RenderTargetView*     g_pRenderTargetView = NULL;

//! pointer to D3D10 Effect and Technique
ID3D10Effect*               g_pEffect = NULL;
ID3D10EffectTechnique*      g_pTechnique = NULL;
ID3D10InputLayout*          g_pVertexLayout = NULL;
ID3D10Buffer*               g_pVertexBuffer = NULL;
ID3D10Buffer*               g_pIndexBuffer = NULL;
ID3D10EffectMatrixVariable* g_pWorldVariable = NULL;
ID3D10EffectMatrixVariable* g_pViewVariable = NULL;
ID3D10EffectMatrixVariable* g_pProjectionVariable = NULL;
D3DXMATRIX                  g_World;
D3DXMATRIX                  g_View;
D3DXMATRIX                  g_Projection;

HANDLE                      g_hFileWriterThread = NULL;

//! Event handles used to pass off rendering
#define NUMFRAMESINFLIGHT   3
HANDLE g_hEncodeCompleteEvent[NUMFRAMESINFLIGHT] = {NULL, NULL, NULL};
HANDLE g_aCanRenderEvents[NUMFRAMESINFLIGHT] = {NULL, NULL, NULL};
unsigned char *g_pBitStreamBuffer[NUMFRAMESINFLIGHT] = {NULL, NULL, NULL};

HANDLE g_hThreadQuitEvent = NULL;

NV_HW_ENC_CODEC          g_Codec  = NV_HW_ENC_H264;

bool g_exitCapture = FALSE;
DWORD g_dwFrameNumber = 0;
DWORD g_dwMaxFrames = 30;
//! Forward declarations
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void InitNVIFR();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();

/*! 
 * Creates a render window.
 */
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    //! Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_ICON1 );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = SAMPLE_NAME;
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_ICON1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    //! Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 640, 480 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( SAMPLE_NAME, SAMPLE_NAME, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
        NULL );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
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
    fwrite(g_pBitStreamBuffer[bufferIndex], params.bitStreamParams.dwByteSize, 1, fileOut);
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
        hEvents[1] = g_hEncodeCompleteEvent[bufferIndex];
        //! Wait for the capture completion event for this buffer
        dwEventID = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
        if (dwEventID - WAIT_OBJECT_0 == 0)
        {
            //! The main thread has signalled us to quit.
            //! Check if there is any pending work and finish it before quitting.
            dwPendingFrames = (g_dwMaxFrames > dwCapturedFrames) ? g_dwMaxFrames - dwCapturedFrames : 0;
            for(DWORD i = 0; i < dwPendingFrames; i++)
            {
                WaitForSingleObject(g_hEncodeCompleteEvent[bufferIndex], INFINITE);
                ResetEvent(g_hEncodeCompleteEvent[bufferIndex]);       // optional

                //! Fetch bitstream from NvIFR and dump to disk
                GetBitStream(bufferIndex, fileOut);
                dwCapturedFrames++;
                //! Wait on next index for new data
                bufferIndex = (bufferIndex+1)%NUMFRAMESINFLIGHT;
            }
            break;
        }
        ResetEvent(g_hEncodeCompleteEvent[bufferIndex]);       // optional

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

/*
 * Locate file
 */
std::string LocateFile ( std::string fname ) 
{
    FILE* fp;
    char filename[1024];
    
    // Search for a resource
    sprintf ( filename, "%s", fname.c_str() );
    if ( fp = fopen(filename, "r") )
    {
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
 * Initialize the NvIFR device
 */

NvIFRLibrary NvIFRLib;
void InitNVIFR()
{
    HINSTANCE g_hNvIFRDll=NULL;
    
    //! Load the NvIFR.dll library
    if(NULL == (g_hNvIFRDll = NvIFRLib.load()))
    {
        MessageBox( NULL, "Unable to load the NvIFR library\r\n", SAMPLE_NAME, MB_OK );
        ExitProcess(0);
    }

    //! Create the INvIFRToHWEncoder_v1 object
    g_pIFR = (INvIFRToHWEncoder_v1 *) NvIFRLib.create (g_pD3DDevice, NVIFR_TO_HWENCODER);
    if(NULL == g_pIFR)
    {
        MessageBox( NULL, "Failed to create INvIFRToHWEncoder_v1\r\n", SAMPLE_NAME, MB_OK);
        ExitProcess(0);
    }

    for (DWORD i = 0; i < NUMFRAMESINFLIGHT; i++)
    {
        //! Create the events for allowing rendering to continue after a capture is complete
        g_aCanRenderEvents[i] = CreateEvent(NULL, TRUE, TRUE, NULL);	
    }
    g_hThreadQuitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}


/*! 
 * Find a compatible DX10 device
 */
bool FindDX10Device ( ID3D10Device*& pDev, IDXGISwapChain*& pSwap, char* msg)
{
    HRESULT hr;
    UINT i = 0; 
    IDXGIFactory* pFactory;
    IDXGIAdapter* pAdapter; 
    DXGI_ADAPTER_DESC desc;	
    ZeroMemory ( &desc, sizeof(desc) );

	UINT createDeviceFlags = 0;

    D3D10_DRIVER_TYPE driverTypes[] =
    {
        D3D10_DRIVER_TYPE_HARDWARE,
        D3D10_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = sizeof( driverTypes ) / sizeof( driverTypes[0] );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = g_dwWidth;
    sd.BufferDesc.Height = g_dwHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;	
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    //! Create a DXGIFactory object.
    if(FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory) ,(void**)&pFactory)))
    {
		MessageBox( NULL, "Unable to create DXGI Factory.\r\n", SAMPLE_NAME, MB_OK );
        return 0x0;
    }
    //! Enumerate adapters
    char dev_name[512];
    bool bFound = false;
    int driverTypeIndex = 0;		// Search for hardware device
    std::vector <IDXGIAdapter*> vAdapters; 

    while( pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND )  
    { 
        // Get adapter description
        vAdapters.push_back(pAdapter); 
        pAdapter->GetDesc ( &desc );
        wcstombs ( dev_name, desc.Description, 512 );		
        sprintf ( msg, "> Found: #%d, %s\n", i, dev_name ); 
        OutputDebugStringA( msg );  // to debug window
 
        // Attempt to create DX device on adapter
        if ( !bFound )
        {
            g_driverType = driverTypes[driverTypeIndex];
            hr = D3D10CreateDeviceAndSwapChain( pAdapter, g_driverType, NULL, createDeviceFlags,
                D3D10_SDK_VERSION, &sd, &pSwap, &pDev );
            if( SUCCEEDED( hr ) )
            {
                bFound = true;              // Adapter succeeded
                sprintf ( msg, ">   Using this one: #%d, %s\n", i, dev_name ); 
                OutputDebugStringA( msg );  // message to debug window
            }
            else
            {
                pAdapter->Release ();       // Release failed adapters
            }
        }
        else
        {
            pAdapter->Release ();           // Release unused adapters
        }
        ++i; 
    }
    if ( !bFound ) sprintf ( msg, "Unable to find DX10 device.\r\n" );
    return bFound;
}


/*!
 * Create Direct3D device and swap chain
 */
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    g_dwWidth = width;
    g_dwHeight = height;

    //! Find and create device with DX10 hardware
    char error_msg[1024];	
    if (! FindDX10Device ( g_pD3DDevice, g_pSwapChain, error_msg ) )
    {
        MessageBox( NULL, error_msg, "NVIFR_DX10.exe", MB_OK );
        return E_FAIL;
    }

    //! Create a render target view
    ID3D10Texture2D* pBuffer;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = g_pD3DDevice->CreateRenderTargetView( pBuffer, NULL, &g_pRenderTargetView );
    pBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    g_pD3DDevice->OMSetRenderTargets( 1, &g_pRenderTargetView, NULL );

    //! Setup the viewport
    D3D10_VIEWPORT vp;
    vp.Width = width;
    vp.Height = height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pD3DDevice->RSSetViewports( 1, &vp );

    //! Create an Effect from fx file
    DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    //dwShaderFlags |= D3D10_SHADER_DEBUG;
#endif

    std::string fxfile = LocateFile ( "Simple.fx" );
    if ( fxfile.empty() )
    {
        MessageBox( NULL, "The FX file cannot be located. Please ensure that Simple.fx is present in the \\media directory or the current project directory.", "Error", MB_OK );
        return hr;
    }
    hr = D3DX10CreateEffectFromFile( fxfile.c_str(), NULL, NULL, "fx_4_0", dwShaderFlags, 0, g_pD3DDevice, NULL, NULL, &g_pEffect, NULL, NULL );
    if( FAILED( hr ) )
    {
        MessageBox( NULL, "FX file found. Unable to create effect.", "Error", MB_OK );
        return hr;
    }

    //! Obtain the technique
    g_pTechnique = g_pEffect->GetTechniqueByName( "Render" );

    //! Obtain the variables
    g_pWorldVariable = g_pEffect->GetVariableByName( "World" )->AsMatrix();
    g_pViewVariable = g_pEffect->GetVariableByName( "View" )->AsMatrix();
    g_pProjectionVariable = g_pEffect->GetVariableByName( "Projection" )->AsMatrix();

    //! Define the input layout
    D3D10_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = sizeof( layout ) / sizeof( layout[0] );

    //! Create the input layout
    D3D10_PASS_DESC PassDesc;
    g_pTechnique->GetPassByIndex( 0 )->GetDesc( &PassDesc );

    hr = g_pD3DDevice->CreateInputLayout( layout, numElements, PassDesc.pIAInputSignature,
        PassDesc.IAInputSignatureSize, &g_pVertexLayout );
    if( FAILED( hr ) )
        return hr;

    //! Set the input layout
    g_pD3DDevice->IASetInputLayout( g_pVertexLayout );

    //! Create a vertex buffer with geometry
    SimpleVertex vertices[] =
    {
        { D3DXVECTOR3( -1.0f, 1.0f, -1.0f ), D3DXVECTOR4( 0.0f, 0.0f, 1.0f, 1.0f ) },
        { D3DXVECTOR3( 1.0f, 1.0f, -1.0f ), D3DXVECTOR4( 0.0f, 1.0f, 0.0f, 1.0f ) },
        { D3DXVECTOR3( 1.0f, 1.0f, 1.0f ), D3DXVECTOR4( 0.0f, 1.0f, 1.0f, 1.0f ) },
        { D3DXVECTOR3( -1.0f, 1.0f, 1.0f ), D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ) },
        { D3DXVECTOR3( -1.0f, -1.0f, -1.0f ), D3DXVECTOR4( 1.0f, 0.0f, 1.0f, 1.0f ) },
        { D3DXVECTOR3( 1.0f, -1.0f, -1.0f ), D3DXVECTOR4( 1.0f, 1.0f, 0.0f, 1.0f ) },
        { D3DXVECTOR3( 1.0f, -1.0f, 1.0f ), D3DXVECTOR4( 1.0f, 1.0f, 1.0f, 1.0f ) },
        { D3DXVECTOR3( -1.0f, -1.0f, 1.0f ), D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 1.0f ) },
    };

    D3D10_BUFFER_DESC bd;
    bd.Usage = D3D10_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( SimpleVertex ) * 8;
    bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    D3D10_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = vertices;

    hr = g_pD3DDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
    if( FAILED( hr ) )
        return hr;

    //! Set vertex buffer
    UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;
    g_pD3DDevice->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );

    //! Create an index buffer
    DWORD indices[] =
    {
        3,1,0,
        2,1,3,

        0,5,4,
        1,5,0,

        3,4,7,
        0,4,3,

        1,6,5,
        2,6,1,

        2,7,6,
        3,7,2,

        6,4,5,
        7,4,6,
    };

    bd.Usage = D3D10_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( DWORD ) * 36;        // 36 vertices needed for 12 triangles in a triangle list
    bd.BindFlags = D3D10_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    InitData.pSysMem = indices;

    hr = g_pD3DDevice->CreateBuffer( &bd, &InitData, &g_pIndexBuffer );
    if( FAILED( hr ) )
        return hr;

    //! Set index buffer
    g_pD3DDevice->IASetIndexBuffer( g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );

    //! Set primitive topology
    g_pD3DDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    //! Initialize the world, view and projection matricies
    D3DXMatrixIdentity( &g_World );

    D3DXVECTOR3 Eye( 0.0f, 1.0f, -5.0f );
    D3DXVECTOR3 At( 0.0f, 1.0f, 0.0f );
    D3DXVECTOR3 Up( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtLH( &g_View, &Eye, &At, &Up );

    D3DXMatrixPerspectiveFovLH( &g_Projection, ( float )D3DX_PI * 0.5f, width / ( FLOAT )height, 0.1f, 100.0f );

    return S_OK;
}


/*!
 * Clean up the objects created
 */
void CleanupDevice()
{
    if( g_pD3DDevice ) g_pD3DDevice->ClearState();
    if( g_pVertexBuffer ) g_pVertexBuffer->Release();
    if( g_pIndexBuffer ) g_pIndexBuffer->Release();
    if( g_pVertexLayout ) g_pVertexLayout->Release();
    if( g_pEffect ) g_pEffect->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pD3DDevice ) g_pD3DDevice->Release();
}


/*! 
 * Windows messaging handler
 */
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
    case WM_PAINT:
        hdc = BeginPaint( hWnd, &ps );
        EndPaint( hWnd, &ps );
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}

/*! 
 * Render the scene. 
 */
void Render()
{
    //! Update cube rotation angle
    static float t = 0.0f;
    t += ( float )D3DX_PI * 0.0025f;

    //! Animate a cube    
    D3DXMatrixRotationY( &g_World, t );

    //! Clear the back buffer    
    float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red,green,blue,alpha
    g_pD3DDevice->ClearRenderTargetView( g_pRenderTargetView, ClearColor );

    //! Update view matrices    
    g_pWorldVariable->SetMatrix( ( float* )&g_World );
    g_pViewVariable->SetMatrix( ( float* )&g_View );
    g_pProjectionVariable->SetMatrix( ( float* )&g_Projection );

    //! Render the scene
    D3D10_TECHNIQUE_DESC techDesc;
    g_pTechnique->GetDesc( &techDesc );
    for( UINT p = 0; p < techDesc.Passes; ++p )
    {
        g_pTechnique->GetPassByIndex( p )->Apply( 0 );
        g_pD3DDevice->DrawIndexed( 36, 0, 0 );        // 36 vertices needed for 12 triangles in a triangle list
    }

    //! Set up the H.264 encoder and target buffers 
    if(0 == g_pBitStreamBuffer[0])
    {
        DWORD dwBitRate720p = 5000000;
        double  dBitRate = double(dwBitRate720p)*(double(g_dwWidth*g_dwHeight)/double(1280*720));           // scaling bitrate with resolution

        NVIFR_HW_ENC_SETUP_PARAMS params = {0};
        params.dwVersion = NVIFR_HW_ENC_SETUP_PARAMS_VER;
        params.dwNBuffers = NUMFRAMESINFLIGHT;
        params.dwBSMaxSize = 2048*1024;
        params.ppPageLockedBitStreamBuffers = g_pBitStreamBuffer;
        params.ppEncodeCompletionEvents = g_hEncodeCompleteEvent;
        
        params.configParams.dwVersion = NV_HW_ENC_CONFIG_PARAMS_VER;
        params.configParams.eStereoFormat = NV_HW_ENC_STEREO_NONE;
        params.configParams.dwProfile = 100;
        params.configParams.dwAvgBitRate = (DWORD)dBitRate;
        params.configParams.dwFrameRateDen = 1;
        params.configParams.dwFrameRateNum = 30;
        params.configParams.dwPeakBitRate = (params.configParams.dwAvgBitRate * 12/10);   // +20%
        params.configParams.dwGOPLength = 75;
        params.configParams.dwQP = 26 ;
        params.configParams.eRateControl = NV_HW_ENC_PARAMS_RC_CBR;
        params.configParams.eCodec = g_Codec;
        params.configParams.bEnableYUV444Encoding = g_bUseYUV444;

        NVIFRRESULT res = g_pIFR->NvIFRSetUpHWEncoder(&params);

        if (res != NVIFR_SUCCESS)
        {
            if (res == NVIFR_ERROR_INVALID_PARAM || res != NVIFR_ERROR_INVALID_PTR)
                MessageBox(NULL, "NvIFR Buffer creation failed due to invalid params.\n", SAMPLE_NAME, MB_OK );
            else
                MessageBox( NULL, "Something is wrong with the driver, cannot initialize IFR buffers\n", SAMPLE_NAME, MB_OK );
        }

        //! Kick off the thread that will save the H.264 for the frame once the NvIFR capture is complete
        g_hFileWriterThread = (HANDLE)_beginthread(fileWriterThread, 0, NULL);
    }

    unsigned int bufferIndex = g_dwFrameNumber%NUMFRAMESINFLIGHT;

    //! Wait for this buffer to finish saving before initiating a new capture
    WaitForSingleObject(g_aCanRenderEvents[bufferIndex], INFINITE);
    ResetEvent(g_aCanRenderEvents[bufferIndex]);

    //! Transfer the render target to the H.264 encoder asynchronously
    NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS params = {0};
    params.dwVersion = NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS_VER;
    params.dwBufferIndex = bufferIndex;
    params.encodePicParams.dwVersion = NV_HW_ENC_PIC_PARAMS_VER;
    NVIFRRESULT res = g_pIFR->NvIFRTransferRenderTargetToHWEncoder(&params);

    g_dwFrameNumber++;
	//! Present the back buffer 
    g_pSwapChain->Present( 0, 0 );
}

/*! 
 * Windows messaging handler
 */
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
    case WM_DESTROY:
        CleanupDevice();
        PostQuitMessage( 0 );
        return 0;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}


/*! 
 * WinMain
 */
INT WINAPI wWinMain( HINSTANCE hInst, HINSTANCE, LPWSTR cmdline, INT )
{
	//! Create a window
    if( FAILED( InitWindow( hInst, SW_SHOWDEFAULT ) ) )
        return -1;

    //! Initialize Direct3D
    if( SUCCEEDED( InitDevice()))
    {
        //! Initialize NVIFR
        InitNVIFR();

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
                }
                else
                { 
                    g_exitCapture = true;
                    SetEvent(g_hThreadQuitEvent);
                    //! Free everything after the side thread finishes up
                    WaitForSingleObject(g_hFileWriterThread, INFINITE);
                    PostQuitMessage(0);
                }
            }
        }
    }
    
    CloseHandle(g_hThreadQuitEvent);
    if (g_pIFR)
    {
        g_pIFR->NvIFRRelease();
        g_pIFR = NULL;
    }
    CleanupDevice();

    //UnregisterClass( "Sample", wc.hInstance );
    return 0;
}

// Prints the help message
void printHelp()
{
    printf("Usage: %s [options]\n", SAMPLE_NAME);
    printf("  -frames framecnt           The number of frames to grab, defaults to %d.\n", g_dwMaxFrames);
    printf("  -codec codectype           The codec to use for encode. 0 = H.264, 1 = HEVC");
    printf("  -yuv444                    Encoded output uses YUV444 (no chroma subsampling.\n");
    printf("  -output OutputFileName     Custom name for bitstream output without file extension.\n");
    printf("  -help                      Prints this help message.\n");
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
        else if(0 == _stricmp(argv[i], "-yuv444"))
        {
            g_bUseYUV444 = true;
        }
        else if (0 == _stricmp(argv[i], "-help"))
        {
            printHelp();
            return 0;
        }
    }

    return wWinMain(inst, NULL, NULL, SW_SHOWNORMAL);
}
