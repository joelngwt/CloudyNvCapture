/*!
 * \brief
 * Shows how to use NvIFR with DX9, and how to capture a 
 * render target to a BMP file.
 *
 * \file
 *
 * This is a simple DX9 sample showing how to use NVIFR to capture a 
 * render target to a file. It shows how to load the NvIFR dll, initialize 
 * the function pointers, and create the NvIFRToSys object.
 * 
 * It then goes on to show how to set up the target buffers and events in 
 * the NvIFRToSys object and then calls TransferRenderTargetToSys to transfer 
 * the render target to system memory.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma warning( disable : 4995 4996 )

#include <Windows.h>
#include <mmsystem.h>
#include <d3dx9.h>
#include <strsafe.h>
#include <string>
#include <process.h>

#include <math.h>
#include <NvIFR/NvIFR.h>
#include <NvIFR/NvIFRToSys.h>
#include <NvIFRLibrary.h>
#include "Bitmap.h"

#include "resource.h"

#define SAMPLE_NAME "DX9IFR_SimpleSample"

//! pointer to the NvIFRToSys object
NvIFRToSys * g_pIFR=NULL;
NVIFR_BUFFER_FORMAT g_IFRFormat = NVIFR_FORMAT_ARGB;
//! the D3D Device
LPDIRECT3D9             g_pD3D = NULL; 
//! the D3D Rendering device
LPDIRECT3DDEVICE9       g_pD3DDevice = NULL; 
//! a vertex buffer to hold vertices
LPDIRECT3DVERTEXBUFFER9 g_pD3DVB = NULL;  

DWORD g_dwWidth=0;
DWORD g_dwHeight=0;
DWORD g_dwMaxFrames = 30; // Max. no of frames to grab

//! a structure for custom vertex type (pos, color, uv)
struct CUSTOMVERTEX
{
    D3DXVECTOR3 position; 
    D3DCOLOR color;   
    FLOAT tu, tv; 
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)

#define NUMFRAMESINFLIGHT   3
//! event handles used to pass off rendering
HANDLE g_hCaptureCompleteEvent[NUMFRAMESINFLIGHT] = {NULL, NULL, NULL};
HANDLE g_aCanRenderEvents[NUMFRAMESINFLIGHT] = {NULL, NULL, NULL};
HANDLE g_hThreadQuitEvent = NULL;
HANDLE g_hFileWriterThreadHandle = NULL;
unsigned char *g_pMainBuffer[NUMFRAMESINFLIGHT] = {NULL, NULL, NULL};
DWORD g_dwFrameNumber = 0;

void SaveOutput(const char *pName, BYTE *pBuf, DWORD dwWidth, DWORD dwHeight, DWORD dwStride)
{
    switch(g_IFRFormat)
    {
    default:
    case NVIFR_FORMAT_ARGB:
        SaveARGB(pName, pBuf, dwWidth, dwHeight, dwStride);
        fprintf (stderr, "Grab succeeded. Wrote %s as ARGB.\n", pName );
        break;

    case NVIFR_FORMAT_RGB:
        SaveRGB(pName, pBuf, dwWidth, dwHeight, dwStride);
        fprintf (stderr, "Grab succeeded. Wrote %s as ARGB.\n", pName );
        break;

    case NVIFR_FORMAT_YUV_444:
        SaveYUV444(pName, pBuf, dwWidth, dwHeight);
        fprintf (stderr, "Grab succeeded. Wrote %s as YUV444.\n", pName );
        break;

    case NVIFR_FORMAT_YUV_420:
        SaveYUV420(pName, pBuf, dwWidth, dwHeight);
        fprintf (stderr, "Grab succeeded. Wrote %s as YUV420p.\n", pName );
    break;

    case NVIFR_FORMAT_RGB_PLANAR:
        SaveRGBPlanar(pName, pBuf, dwWidth, dwHeight);
        fprintf (stderr, "Grab succeeded. Wrote %s as RGB_PLANAR.\n", pName );
    break;
    }
}

/*! 
 * Captures incoming buffers and saves them to BMP files
 */
void CaptureToBMP(void *data)
{
    HANDLE hEvents[2];
    hEvents[0] = g_hThreadQuitEvent;
    unsigned int bufferIndex = 0;
    unsigned int outIndex = 0;
    unsigned int pendingFrames = 0;
    unsigned int dwStride = (g_dwWidth+3)&~3;
    DWORD dwEventID = 0;
    char buf[100];
    //! While the render loop is still running
    while (1)	
    {
        hEvents[1] = g_hCaptureCompleteEvent[bufferIndex];
        //! Wait for the capture completion event for this buffer
        dwEventID = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
        if (dwEventID - WAIT_OBJECT_0 == 0)
        {
            //! The main thread has signalled us to quit.
            //! Finish pending work and quit.
            pendingFrames = (g_dwMaxFrames > outIndex) ? g_dwMaxFrames-outIndex : 0;
            for (unsigned int i = 0; i < pendingFrames; i++)
            {
                WaitForSingleObject(g_hCaptureCompleteEvent[bufferIndex], INFINITE);

                //! Create a unique filename for this output file
                sprintf(buf, ".\\nvIFR_DX9_ToSys.%d.bmp", outIndex++);
                //! Save as a bitmap
                SaveOutput(buf, (BYTE *)g_pMainBuffer[bufferIndex], g_dwWidth, g_dwHeight, dwStride);
                ResetEvent(g_hCaptureCompleteEvent[bufferIndex]);       // optional
                //! Update buffer index
                bufferIndex = (++bufferIndex)%NUMFRAMESINFLIGHT;
            }            
            break;
        }

        //! Create a unique filename for this output file
        sprintf(buf, ".\\nvIFR_DX9_ToSys.%d.bmp", outIndex++);
        //! Save as a bitmap
        SaveOutput(buf, (BYTE *)g_pMainBuffer[bufferIndex], g_dwWidth, g_dwHeight, dwStride);
        //! Set the render event for this buffer index so rendering can resume
        ResetEvent(g_hCaptureCompleteEvent[bufferIndex]);       // optional
        SetEvent(g_aCanRenderEvents[bufferIndex]);
        //! Update buffer index
        bufferIndex = (++bufferIndex)%NUMFRAMESINFLIGHT;
    }
    _endthread();
}

/*! 
 * Initialize the NvIFR device
 */
void InitNVIFR()
{
    HINSTANCE g_hNvIFRDll=NULL;
    NvIFRLibrary NvIFRlib;
    //! Load the NvIFR.dll library
    if(NULL == (g_hNvIFRDll = NvIFRlib.load()))
    {
        MessageBox( NULL, "Unable to load the NvIFR library\r\n", SAMPLE_NAME, MB_OK );
        exit(1);
    }
    g_pIFR = (NvIFRToSys *)NvIFRlib.create(g_pD3DDevice, NVIFR_TOSYS);
    if (!g_pIFR)
    {
        MessageBox( NULL, "Failed to create the NvIFRToSys Interface\r\n", SAMPLE_NAME, MB_OK);
        ExitProcess(1);
    }

    for (DWORD i = 0; i < NUMFRAMESINFLIGHT; i++)
    {
        //Create the events for allowing rendering to continue after a capture is complete
        g_aCanRenderEvents[i] = CreateEvent(NULL, TRUE, TRUE, NULL);	
    }
    g_hThreadQuitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
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
    d3dpp.BackBufferWidth  = 1680;
    d3dpp.BackBufferHeight = 1050;
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

    //! Get a pointer to the render target
    IDirect3DSurface9* pRenderTarget = NULL;
    g_pD3DDevice->GetRenderTarget( 0, & pRenderTarget );

    if (pRenderTarget) //if we have a valid render target and a valid buffer
    {
        //Get the surface description for the render target
        D3DSURFACE_DESC Desc;
        pRenderTarget->GetDesc(&Desc);
        pRenderTarget->Release();

        g_dwWidth = Desc.Width;
        g_dwHeight = Desc.Height;
    }

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
 * Load a texture and generate geometry.
 */
HRESULT InitGeometry()
{
    //! Create the vertex buffer.
    if( FAILED( g_pD3DDevice->CreateVertexBuffer( 50 * 2 * sizeof( CUSTOMVERTEX ),
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
    if (!g_pIFR)
        return;
    //! Clear the backbuffer and the zbuffer
    g_pD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
    D3DCOLOR_XRGB( 0, 0, 255 ), 1.0f, 0 );

    //! Begin the scene
    if( SUCCEEDED( g_pD3DDevice->BeginScene() ) )
    {
        //! Setup the world, view, and projection matrices
        SetupMatrices();

        //! Render the vertex buffer contents
        g_pD3DDevice->SetStreamSource( 0, g_pD3DVB, 0, sizeof( CUSTOMVERTEX ) );
        g_pD3DDevice->SetFVF( D3DFVF_CUSTOMVERTEX );

        g_pD3DDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 * 50 - 2 );

        //! End the scene
        g_pD3DDevice->EndScene();
    }

    if(NULL == g_pMainBuffer[0])
    {
        //! Set up the target buffers and events in the NvIFRToSys object
        NVIFR_TOSYS_SETUP_PARAMS params = {0};
        params.dwVersion = NVIFR_TOSYS_SETUP_PARAMS_VER;
        params.eFormat = g_IFRFormat;
        params.eSysStereoFormat = NVIFR_SYS_STEREO_NONE;
        params.dwNBuffers = NUMFRAMESINFLIGHT;
        params.ppPageLockedSysmemBuffers = g_pMainBuffer;
        params.ppTransferCompletionEvents = g_hCaptureCompleteEvent;

        NVIFRRESULT res = g_pIFR->NvIFRSetUpTargetBufferToSys(&params);
        if (res != NVIFR_SUCCESS)
        {
            if (res == NVIFR_ERROR_INVALID_PARAM || res != NVIFR_ERROR_INVALID_PTR)
                MessageBox(NULL, "NvIFR Buffer creation failed due to invalid params.\n", SAMPLE_NAME, MB_OK );
            else
                MessageBox( NULL, "Something is wrong with the driver, cannot initialize IFR buffers\n", SAMPLE_NAME, MB_OK );
        }

        //! Kick off the threads that will save the BMP file for the frame once the NvIFR capture is complete
        g_hFileWriterThreadHandle = (HANDLE)_beginthread(CaptureToBMP, 0, NULL);
    }

    unsigned int bufferIndex = g_dwFrameNumber%NUMFRAMESINFLIGHT;

    //! Wait for this buffer to finish saving before initiating a new capture
    WaitForSingleObject(g_aCanRenderEvents[bufferIndex], INFINITE);
    ResetEvent(g_aCanRenderEvents[bufferIndex]);

    //! Transfer the render target to the system memory buffer
    NVIFRRESULT res = g_pIFR->NvIFRTransferRenderTargetToSys(bufferIndex);     // grab whole rendertarget, no resize

    g_dwFrameNumber++;

    //! Present the backbuffer contents to the display
    g_pD3DDevice->Present( NULL, NULL, NULL, NULL );
}


/*! 
 * Windows messaging handler
 */
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
	case WM_KEYDOWN:
    case WM_DESTROY:

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
    HINSTANCE hInstance =  GetModuleHandle( NULL );
    WNDCLASSEX wc =
    {
        sizeof( WNDCLASSEX ), CS_CLASSDC, MsgProc, 0L, 0L,
        hInstance, LoadIcon( hInstance, ( LPCTSTR )IDI_ICON1 ), NULL, 
        NULL, NULL, SAMPLE_NAME, LoadIcon( hInstance, ( LPCTSTR )IDI_ICON1 )
    };
    //! Register the window class
    RegisterClassEx( &wc );

    //! Create the application's window
    HWND hWnd = CreateWindow( SAMPLE_NAME, SAMPLE_NAME,
        WS_OVERLAPPEDWINDOW, 100, 100, 640, 480,
        NULL, NULL, wc.hInstance, NULL );

    // Initialize Direct3D
    if( SUCCEEDED( InitD3D( hWnd ) ) )
    {
        InitNVIFR();

        // Create the scene geometry
        if( SUCCEEDED( InitGeometry() ) )
        {
            // Show the window
            ShowWindow( hWnd, SW_SHOWDEFAULT );
            UpdateWindow( hWnd );

            // Enter the message loop
            MSG msg;
            ZeroMemory( &msg, sizeof( msg ) );
            while( msg.message != WM_QUIT && msg.message != WM_DESTROY)
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
                        SetEvent(g_hThreadQuitEvent);
                        //! Free everything after the side thread finishes up
                        WaitForSingleObject(g_hFileWriterThreadHandle, INFINITE);
                        PostQuitMessage(0);
                    }
                }
            }
        }
    }

    if(g_pIFR)
    {
        g_pIFR->NvIFRRelease();
        g_pIFR = NULL;
    }
        
    Cleanup();

    UnregisterClass( "Sample", wc.hInstance );
    return 0;
}

// Prints the help message
void printHelp()
{
    printf("Usage: %s [options]\n", SAMPLE_NAME);
    printf("  -frames framecnt           The number of frames to grab, defaults to %d.\n", g_dwMaxFrames);
    printf("  -format <ARGB|RGB|YUV420|YUV444|PLANAR|XOR>, deafults to ARGB.\n");
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
        else if(0 == _stricmp(argv[i], "-format"))
        {
            ++i;

            if(i >= argc)
            {
                printf("Missing -format option\n");
                printHelp();
                return false;
            }
            if(0 == _stricmp(argv[i], "ARGB"))
                g_IFRFormat = NVIFR_FORMAT_ARGB;
            else if(0 == _stricmp(argv[i], "RGB"))
                g_IFRFormat = NVIFR_FORMAT_RGB;
            else if(0 == _stricmp(argv[i], "YUV420"))
                g_IFRFormat = NVIFR_FORMAT_YUV_420;
            else if(0 == _stricmp(argv[i], "YUV444"))
                g_IFRFormat = NVIFR_FORMAT_YUV_444;
            else if(0 == _stricmp(argv[i], "PLANAR"))
                g_IFRFormat = NVIFR_FORMAT_RGB_PLANAR;
            else
            {
                printf("Unexpected -format option %s\n", argv[i]);
                printHelp();
                return false;
            }
        }
    }

    return wWinMain(inst, NULL, NULL, SW_SHOWNORMAL);
}
