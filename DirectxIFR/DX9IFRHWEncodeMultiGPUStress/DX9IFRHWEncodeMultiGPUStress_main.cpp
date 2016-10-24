/*!
 * \brief
 * Writes to a shared surface, which further decouples rendering and 
 * allows the renderer and encoder to both run at their maximum rate. 
 *
 * \file
 *
 * This DirectX 9 sample demonstrates how to grab and encode a frame 
 * using a shared surface with asynchronous render and encode. Like 
 * the AsyncH264HwEncode sample, writing to H.264 occurs asynchronously 
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

#pragma warning(disable : 4995 4996) // disable deprecated warning
#include <Windows.h>
#include <mmsystem.h>
#include <d3dx9.h>
#include <process.h>
#include <strsafe.h>
#include <NvIFR/NvIFR.h>
#include <NvIFR/NvIFRHWEnc.h>
#include <NvIFRLibrary.h>
#include "Timer.h"

#define WIDTH           1280
#define HEIGHT          720
#define SAMPLE_NAME     "DX9IFRH264MultiGPUStress"

DWORD g_AppsPerAdapter=8;
DWORD g_dwSlaveW = WIDTH;
DWORD g_dwSlaveH = HEIGHT;
BOOL  g_bNoDump  = false;
DWORD g_dwMaxFrames = 100;
DWORD g_dwTimeOut = 30;
DWORD g_dwTotalFramesPerSession = 0;
DWORD g_dwCodec = 0;

NV_HW_ENC_CODEC          g_Codec  = NV_HW_ENC_H264;

HMODULE hNvIFRDll=NULL;
NvIFRLibrary nvIFRLib;
Timer T;

struct AdapterThreadData
{
    LPDIRECT3D9             m_pD3D;         // Used to create the D3DDevice
    LPDIRECT3DDEVICE9       m_pDevice;      // Our rendering device
    LPDIRECT3DVERTEXBUFFER9 m_pVB;          // Buffer to hold vertices
    LPDIRECT3DTEXTURE9      m_pTexture;     // Our texture

    DWORD                   m_dwDxOrdinal;
    DWORD                   m_dwIndex;
    DWORD                   m_dwFrameCount;

    HANDLE                  m_hEncoderThreadHandle;

    HANDLE                  m_hEncoderReadyEvent;
        
    IFRSharedSurfaceHandle  m_hIFRSharedSurfaceHandle;

    BOOL                    m_bQuit;
    HWND                    m_hWnd;
    HANDLE                  m_hRenderingStarted;
};


// A structure for our custom vertex type. We added texture coordinates
struct CUSTOMVERTEX
{
    D3DXVECTOR3 position; // The position
    D3DCOLOR color;    // The color
    FLOAT tu, tv;   // The texture coordinates
};

// Our custom FVF, which describes our custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)


/*! 
 * Creates the D3D9 Device and get render target.
 */
HRESULT InitD3D(HWND hWnd, AdapterThreadData * pAdapterThreadData)
{
    HRESULT hr = S_OK;
    // printf("%s[%d]: %s\n", SAMPLE_NAME, _getpid(), __FUNCTION__);
    // Create the D3D object.
    if (NULL == (pAdapterThreadData->m_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        printf("%s[%d]: %s: Couldnt initialize d3d9.\n", SAMPLE_NAME, _getpid(), __FUNCTION__);
        return E_FAIL;
    }

    // Set up the structure used to create the D3DDevice. Since we are now
    // using more complex geometry, we will create a device with a zbuffer.
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));

    d3dpp.BackBufferWidth  = g_dwSlaveW;
    d3dpp.BackBufferHeight = g_dwSlaveH;
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;  
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;    // Create the D3DDevice

    if (FAILED(hr = pAdapterThreadData->m_pD3D->CreateDevice(pAdapterThreadData->m_dwDxOrdinal, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
                                      &d3dpp, &pAdapterThreadData->m_pDevice)))
    {
        printf("%s[%d]: %s: Failed to create D3D %x. W = %d H = %d\n", SAMPLE_NAME, _getpid(), __FUNCTION__, hr, g_dwSlaveW, g_dwSlaveH);
        return hr;
    }

    // Turn off culling
    pAdapterThreadData->m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    // Turn off D3D lighting
    pAdapterThreadData->m_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

    // Turn on the zbuffer
    pAdapterThreadData->m_pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

    return S_OK;
}


/*! 
 * Load a texture and generate geometry.
 */
HRESULT InitGeometry(AdapterThreadData * pAdapterThreadData)
{
    // printf("%s[%d]: %s\n", SAMPLE_NAME, _getpid(), __FUNCTION__);
    // Use D3DX to create a texture from a file based image
    //if (FAILED(D3DXCreateTextureFromFile(pAdapterThreadData->m_pDevice, "banana.bmp", &pAdapterThreadData->m_pTexture)))
    //{
    //    // If texture is not in current folder, try parent folder
    //    if (FAILED(D3DXCreateTextureFromFile(pAdapterThreadData->m_pDevice, "..\\banana.bmp", &pAdapterThreadData->m_pTexture)))
    //    {
    //        MessageBox(NULL, "Could not find banana.bmp", SAMPLE_NAME, MB_OK);
    //        return E_FAIL;
    //    }
    //}

    // Create the vertex buffer.
    if (FAILED(pAdapterThreadData->m_pDevice->CreateVertexBuffer(50 * 2 * sizeof(CUSTOMVERTEX),
                                                  0, D3DFVF_CUSTOMVERTEX,
                                                  D3DPOOL_DEFAULT, &pAdapterThreadData->m_pVB, NULL)))
    {
        return E_FAIL;
    }

    //! Fill the vertex buffer.
    CUSTOMVERTEX* pVertices;
    if (FAILED(pAdapterThreadData->m_pVB->Lock(0, 0, (void**)&pVertices, 0)))
        return E_FAIL;
    for(DWORD i = 0; i < 50; i++)
    {
        FLOAT theta = (2 * D3DX_PI * i) / (50 - 1);

        pVertices[2 * i + 0].position = D3DXVECTOR3(sinf(theta), -1.0f, cosf(theta));
        pVertices[2 * i + 0].color = 0xefffffff+_getpid();
        pVertices[2 * i + 0].tu = ((FLOAT)i) / (50 - 1);
        pVertices[2 * i + 0].tv = 1.0f;

        pVertices[2 * i + 1].position = D3DXVECTOR3(sinf(theta), 1.0f, cosf(theta));
        pVertices[2 * i + 1].color = 0xefff8080+_getpid();
        pVertices[2 * i + 1].tu = ((FLOAT)i) / (50 - 1);
        pVertices[2 * i + 1].tv = 0.0f;
    }
    pAdapterThreadData->m_pVB->Unlock();

    return S_OK;
}

/*! 
 * Release D3D all textures, buffes, and devices
 */
VOID Cleanup(AdapterThreadData * pAdapterThreadData)
{
    // printf("%s[%d]: %s\n", SAMPLE_NAME, _getpid(), __FUNCTION__);
    WaitForSingleObject(pAdapterThreadData->m_hEncoderThreadHandle, INFINITE);

    if (pAdapterThreadData->m_pTexture != NULL)
    {
        pAdapterThreadData->m_pTexture->Release();
        pAdapterThreadData->m_pTexture = NULL;
    }

    if (pAdapterThreadData->m_pVB != NULL)
    {
        pAdapterThreadData->m_pVB->Release();
        pAdapterThreadData->m_pVB = NULL;
    }

    if (pAdapterThreadData->m_pDevice != NULL)
    {
        pAdapterThreadData->m_pDevice->Release();
        pAdapterThreadData->m_pDevice= NULL;
    }

    if (pAdapterThreadData->m_pD3D != NULL)
    {
        pAdapterThreadData->m_pD3D->Release();
        pAdapterThreadData->m_pD3D = NULL;
    }
    
    if (pAdapterThreadData->m_hEncoderReadyEvent)
    {
        CloseHandle(pAdapterThreadData->m_hEncoderReadyEvent);
        pAdapterThreadData->m_hEncoderReadyEvent = NULL;
    }
    
    memset(pAdapterThreadData, 0, sizeof(AdapterThreadData));
}

/*! 
 * Setup world, view and projection matrices
 */
VOID SetupMatrices(AdapterThreadData * pAdapterThreadData)
{
    // printf("%s[%d]: %s\n", SAMPLE_NAME, _getpid(), __FUNCTION__);
    // Set up world matrix
    D3DXMATRIXA16 matWorld;
    D3DXMatrixIdentity(&matWorld);
    static float xrotation = 0.0f; 
    xrotation+=0.05f;    // an ever-increasing float value
    // build a matrix to rotate the model based on the increasing float value
    D3DXMatrixRotationX(&matWorld, xrotation);
    pAdapterThreadData->m_pDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // Set up our view matrix. A view matrix can be defined given an eye point,
    // a point to lookat, and a direction for which way is up. Here, we set the
    // eye five units back along the z-axis and up three units, look at the
    // origin, and define "up" to be in the y-direction.
    D3DXVECTOR3 vEyePt(0.0f, 3.0f,-5.0f);
    D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
    pAdapterThreadData->m_pDevice->SetTransform(D3DTS_VIEW, &matView);

    // For the projection matrix, we set up a perspective transform(which
    // transforms geometry from 3D view space to 2D viewport space, with
    // a perspective divide making objects smaller in the distance). To build
    // a perpsective transform, we need the field of view(1/4 pi is common),
    // the aspect ratio, and the near and far clipping planes(which define at
    // what distances geometry should be no longer be rendered).
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f);
    pAdapterThreadData->m_pDevice->SetTransform(D3DTS_PROJECTION, &matProj);
}

/*! 
 * Windows messaging handler
 */
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_DESTROY:
            AdapterThreadData * pAdapterThreadData = (AdapterThreadData *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            pAdapterThreadData->m_bQuit=TRUE;
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

WNDCLASSEX wc =
{
    sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
    GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
    SAMPLE_NAME, NULL
};

/*!
 * Setup Encoding Thread onjects
 */
void NvIFREncodingThread(void * pData)
{
    // printf("%s[%d]: %s\n", SAMPLE_NAME, _getpid(), __FUNCTION__);
    AdapterThreadData * pAdapterThreadData =(AdapterThreadData *)pData;

    // Create the application's window
    HWND hWnd = CreateWindow(SAMPLE_NAME, SAMPLE_NAME,
                              WS_OVERLAPPEDWINDOW, rand()%512, rand()%512, 300, 300,
                              NULL, NULL, wc.hInstance, NULL);
    // Show the window
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);


    LPDIRECT3D9             pD3D = NULL; // Used to create the D3DDevice
    LPDIRECT3DDEVICE9       pD3DDevice = NULL; // Our rendering device

    // Create the D3D object.
    if (NULL == (pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        return;
    }

    // Set up the structure used to create the D3DDevice. Since we are now
    // using more complex geometry, we will create a device with a zbuffer.
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth = g_dwSlaveW;
    d3dpp.BackBufferHeight = g_dwSlaveH;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    // Create the D3DDevice
    if (FAILED(pD3D->CreateDevice(pAdapterThreadData->m_dwDxOrdinal, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
                                      &d3dpp, &pD3DDevice)))
    {
        printf("%s[%d]: Failed to init D3D.\n", SAMPLE_NAME, _getpid());
        return;
    }

    // Turn off culling
    pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    // Turn off D3D lighting
    pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    // Turn off the zbuffer
    pD3DDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

    pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 0, 255, 0), 0 , 0);

    INvIFRToHWEncoder_v1 * pIFRToHWEncoder = NULL;

    NVIFR_CREATE_PARAMS dIFRCREATEPARAMS = {0};
    dIFRCREATEPARAMS.dwVersion = NVIFR_CREATE_PARAMS_VER;
    dIFRCREATEPARAMS.pDevice = pD3DDevice;
    dIFRCREATEPARAMS.dwInterfaceType = NVIFR_TO_HWENCODER;
    
    NVIFRRESULT result = nvIFRLib.createEx(&dIFRCREATEPARAMS);

     if (result != NVIFR_SUCCESS)
     {
        printf("%s[%d]: Failed to create NvIFR instance with error: %d\n", SAMPLE_NAME, _getpid(), result);
        exit(result);
     }

     pIFRToHWEncoder =(INvIFRToHWEncoder_v1 *)dIFRCREATEPARAMS.pNvIFR;

    if (!pIFRToHWEncoder)
    {
        printf("%s[%d]: NvIFR Object not returned by NvIFR_CreateEx.\n", SAMPLE_NAME, _getpid());
        exit(-1);
    }

    DWORD dwKpbs = 5000;
    DWORD bitrate720p = dwKpbs*1000;
    double  dBitRate = double(bitrate720p)*(double(g_dwSlaveW*g_dwSlaveH)/double(1280*720));

    NV_HW_ENC_CONFIG_PARAMS encodeConfig;
    memset(&encodeConfig, 0, sizeof(NV_HW_ENC_CONFIG_PARAMS));

    encodeConfig.dwProfile = 100;
    encodeConfig.dwAvgBitRate =(DWORD)dBitRate;
    encodeConfig.dwFrameRateDen = 1;
    encodeConfig.dwFrameRateNum = 60;
    encodeConfig.dwPeakBitRate =(encodeConfig.dwAvgBitRate * 12/10);   // +20%
    encodeConfig.dwGOPLength = 0xFFFFFFFF;
    encodeConfig.eRateControl = NV_HW_ENC_PARAMS_RC_2_PASS_QUALITY;

    unsigned char * pBitStreamBuffer;
    HANDLE hEncoderEvent=NULL;

    NVIFR_HW_ENC_SETUP_PARAMS params = {0};
    params.dwVersion = NVIFR_HW_ENC_SETUP_PARAMS_VER;
    params.dwNBuffers = 1;
    params.dwBSMaxSize = 2048*1024;
    params.ppPageLockedBitStreamBuffers =  &pBitStreamBuffer;
    params.ppEncodeCompletionEvents = &hEncoderEvent;

    params.configParams.dwProfile = 100;
    params.configParams.eStereoFormat = NV_HW_ENC_STEREO_NONE;
    params.configParams.dwAvgBitRate =(DWORD)dBitRate;
    params.configParams.dwFrameRateDen = 1;
    params.configParams.dwFrameRateNum = 60;
    params.configParams.dwPeakBitRate =(params.configParams.dwAvgBitRate * 12/10);   // +20%
    params.configParams.dwGOPLength = 0xFFFFFFFF;
    params.configParams.eRateControl = NV_HW_ENC_PARAMS_RC_2_PASS_QUALITY;
    params.configParams.eCodec = g_Codec;
    result = pIFRToHWEncoder->NvIFRSetUpHWEncoder(&params);
 
    if (result!=NVIFR_SUCCESS)
    {
        printf("%s[%d]: Failed to setup NvIFR HW Encoder with error %d.\n", SAMPLE_NAME, _getpid(), result);
        exit(result);
    }

    SetEvent(pAdapterThreadData->m_hEncoderReadyEvent);


    FILE * fileOut = NULL;
    if (!g_bNoDump)
    {
        char buf[256] = {0};
        if (g_Codec == NV_HW_ENC_HEVC)
        {
            sprintf(buf, "testIFRdx9-%d-%d.h265", GetCurrentProcessId(), GetCurrentThreadId());
        }
        else
        {
            sprintf(buf, "testIFRdx9-%d-%d.h264", GetCurrentProcessId(), GetCurrentThreadId());
        }
        fileOut = fopen(buf, "wb");
        if (!fileOut)
        {
            printf("%s[%d]: Cannot create h264 output file %s, exiting\n", SAMPLE_NAME, _getpid(), buf);
            exit(1);
        }  
    }
    
    DOUBLE dEnd= 0;
    DOUBLE dStart = 0;
    DOUBLE dEncodeTime = 0;

    WaitForSingleObject(pAdapterThreadData->m_hRenderingStarted, INFINITE); // This wait ensures first frame is rendered, here onwards rendering and encoding will run asynchronously.

    while(!pAdapterThreadData->m_bQuit)
    {
        IDirect3DSurface9* pRenderTarget;

        pD3DDevice->GetRenderTarget(0, &pRenderTarget);

        nvIFRLib.CopyFromSharedSurface(pD3DDevice, pAdapterThreadData->m_hIFRSharedSurfaceHandle, pRenderTarget);

        pRenderTarget->Release();
        dStart=T.now();

        NVIFRRESULT eRet = NVIFR_ERROR_GENERIC;

        NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS params = {0};
        params.dwVersion = NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS_VER;
        params.dwBufferIndex = 0;
        params.encodePicParams.dwVersion = NV_HW_ENC_PIC_PARAMS_VER;
        params.encodePicParams.ulCaptureTimeStamp = (long)dStart;
                
        eRet=pIFRToHWEncoder->NvIFRTransferRenderTargetToHWEncoder(&params);
        
        if (eRet == NVIFR_SUCCESS)    // save to disk
        {
            // the wait here is just an example, in the case of disk IO, 
            // it forces the currect transfer to be done, not taking advantage of CPU/GPU concurrency
            WaitForSingleObject(hEncoderEvent, INFINITE);
            ResetEvent(hEncoderEvent);

            dEnd=T.now();
            dEncodeTime += (dEnd - dStart);

            NVIFR_HW_ENC_GET_BITSTREAM_PARAMS getBitStreamParams = {0};
            getBitStreamParams.dwVersion = NVIFR_HW_ENC_GET_BITSTREAM_PARAMS_VER;
            getBitStreamParams.dwBufferIndex = 0;
            getBitStreamParams.bitStreamParams.dwVersion = NV_HW_ENC_GET_BIT_STREAM_PARAMS_VER;

            eRet=pIFRToHWEncoder->NvIFRGetStatsFromHWEncoder(&getBitStreamParams);
            
            if (fileOut)
                fwrite(pBitStreamBuffer, getBitStreamParams.bitStreamParams.dwByteSize, 1, fileOut);
            
            // see other sdk sample for asynchronous encoding, so the GPU and CPU would not stall at all
        }
        pAdapterThreadData->m_dwFrameCount ++;
    }

    dEnd = T.now();
    printf("%s[%d]: Process#%d on Adapter#%d: Frames : %d, Avg Encode time: %f ms\n", SAMPLE_NAME, _getpid(), pAdapterThreadData->m_dwIndex, pAdapterThreadData->m_dwDxOrdinal, pAdapterThreadData->m_dwFrameCount, dEncodeTime/pAdapterThreadData->m_dwFrameCount);

    if (fileOut)
        fclose(fileOut);

    if (pAdapterThreadData->m_hIFRSharedSurfaceHandle)
    {
        // Unmap shared surface from the client device, and release the shared memory.
        // This needs to be done while both, the owner d3d9 device and the client d3d9 device are alive.
        nvIFRLib.DestroySharedSurface(pAdapterThreadData->m_pDevice, pAdapterThreadData->m_hIFRSharedSurfaceHandle);
        pAdapterThreadData->m_hIFRSharedSurfaceHandle = NULL;
    }
    
    if (pIFRToHWEncoder)
    {
        pIFRToHWEncoder->NvIFRRelease();
        pIFRToHWEncoder = NULL;
    }
}

/*! 
 * Render the scene
 */
VOID Render(AdapterThreadData * pAdapterThreadData)
{
    //printf("%s[%d]: %s\n", SAMPLE_NAME, _getpid(), __FUNCTION__);
    if (!pAdapterThreadData->m_pDevice)
    {
        printf("%s[%d]: %s : returning. No d3d device.\n", SAMPLE_NAME, _getpid(), __FUNCTION__);
        return;
    }

    HRESULT hr=S_OK;
    // Clear the backbuffer and the zbuffer
    pAdapterThreadData->m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                         D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

    // Begin the scene
    if (SUCCEEDED(pAdapterThreadData->m_pDevice->BeginScene()))
    {
        // Setup the world, view, and projection matrices
        SetupMatrices(pAdapterThreadData);

        // Setup our texture. Using textures introduces the texture stage states,
        // which govern how textures get blended together(in the case of multiple
        // textures) and lighting information. In this case, we are modulating
        // (blending) our texture with the diffuse color of the vertices.

        // Render the vertex buffer contents
        hr=pAdapterThreadData->m_pDevice->SetStreamSource(0, pAdapterThreadData->m_pVB, 0, sizeof(CUSTOMVERTEX));
        hr=pAdapterThreadData->m_pDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
        hr=pAdapterThreadData->m_pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2 * 50 - 2);

        // End the scene
        pAdapterThreadData->m_pDevice->EndScene();

        // Present the backbuffer contents to the display
        hr=pAdapterThreadData->m_pDevice->Present(NULL, NULL, pAdapterThreadData->m_hWnd, NULL);
    }

    IDirect3DSurface9* pRenderTarget;
  
    pAdapterThreadData->m_pDevice->GetRenderTarget(0, & pRenderTarget);

    nvIFRLib.CopyToSharedSurface(pAdapterThreadData->m_pDevice, pAdapterThreadData->m_hIFRSharedSurfaceHandle, pRenderTarget);

    pRenderTarget->Release();

}

void D3DRenderingMasterThread(DWORD dwDxOrdinal, DWORD dwAppIndex)
{
    // printf("%s[%d]: %s\n", SAMPLE_NAME, _getpid(), __FUNCTION__);
    AdapterThreadData dAdapterThreadData;
    memset(&dAdapterThreadData, 0, sizeof(dAdapterThreadData));
    dAdapterThreadData.m_dwDxOrdinal = dwDxOrdinal;
    dAdapterThreadData.m_dwIndex = dwAppIndex;
    dAdapterThreadData.m_bQuit = FALSE;
    dAdapterThreadData.m_dwFrameCount = 0;

    // Create the application's window
    HWND hWnd = CreateWindow(SAMPLE_NAME, SAMPLE_NAME,
                              WS_OVERLAPPEDWINDOW, 100, 100, g_dwSlaveW/2, g_dwSlaveH/2,
                              NULL, NULL, wc.hInstance, NULL);

    if (hWnd == NULL)
    {
        printf("%s[%d]: Create Window failed. Exit.\n", SAMPLE_NAME, _getpid());
        exit(1);
    }  

    char buf[256];

    sprintf(buf, "%s[%d]: app %d using dx ordinal %d", SAMPLE_NAME, _getpid(), dAdapterThreadData.m_dwIndex, dAdapterThreadData.m_dwDxOrdinal);

    SetWindowText(hWnd, buf);

    double dStart = T.now();
        
    SetWindowLongPtr(hWnd, GWLP_USERDATA,(LONG_PTR)&dAdapterThreadData);
    dAdapterThreadData.m_hWnd = hWnd;

    // Initialize Direct3D
    if (SUCCEEDED(InitD3D(hWnd, &dAdapterThreadData)))
    {
        // Create the scene geometry
        if (SUCCEEDED(InitGeometry(&dAdapterThreadData)))
        {
            // Show the window
            ShowWindow(hWnd, SW_NORMAL);
            UpdateWindow(hWnd);
          
            if (!nvIFRLib.CreateSharedSurface(dAdapterThreadData.m_pDevice, g_dwSlaveW, g_dwSlaveH, &dAdapterThreadData.m_hIFRSharedSurfaceHandle))
            {
                printf("%s[%d]: Failed to create NvIFR Shared Surface.\n", SAMPLE_NAME, _getpid());
                exit(2);
            }

            dAdapterThreadData.m_hEncoderReadyEvent = CreateEvent(0, TRUE, FALSE, 0);
            dAdapterThreadData.m_hRenderingStarted = CreateEvent(0, TRUE, FALSE, 0);

            dAdapterThreadData.m_hEncoderThreadHandle =(HANDLE)_beginthread(NvIFREncodingThread, 0, &dAdapterThreadData);

            WaitForSingleObject(dAdapterThreadData.m_hEncoderReadyEvent, INFINITE);			

            // Enter the message loop
            MSG msg;
            ZeroMemory(&msg, sizeof(msg));
            int count = 0;
            BOOL bFirstFrameRendered = FALSE;
            while(msg.message != WM_QUIT)
            {
                if (PeekMessage(&msg, hWnd, 0U, 0U, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                else
                {
                    Render(&dAdapterThreadData);
                    if(!bFirstFrameRendered)
                    {
                        bFirstFrameRendered = TRUE;
                        SetEvent(dAdapterThreadData.m_hRenderingStarted);
                    }
                }

                double dnow = T.now();
                if ( dAdapterThreadData.m_dwFrameCount > g_dwMaxFrames) 
                {
                    dAdapterThreadData.m_bQuit=TRUE;
                }
                if (dAdapterThreadData.m_bQuit)
                    break;
            }
        }
    }
    Cleanup(&dAdapterThreadData);
}


INT WINAPI _WinMain(HINSTANCE hInst, HINSTANCE, LPSTR pCMD, INT)
{
    UNREFERENCED_PARAMETER(hInst);
    // printf("%s[%d]: %s\n", SAMPLE_NAME, _getpid(), __FUNCTION__);
    // Register the window class
    char pName[256];
    char pCommandLine[1024]; 

    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};

    GetModuleFileName(GetModuleHandle(NULL), pName, 256);

    HANDLE ahSlaveProcesses[256];
    DWORD dwSlaveProcesses=0;

    LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (NULL == pD3D)
    {
        printf("%s: Failed to initialize D3D9_%d\n", SAMPLE_NAME, _getpid(), D3D_SDK_VERSION);
        return -1;
    }
   
    DWORD dwNumDxAdapters=pD3D->GetAdapterCount();
    DWORD dwNGRIDAdapters=0;
    DWORD dwTotalAppIndex=0;

    g_dwTotalFramesPerSession = g_dwTimeOut*60/(dwNumDxAdapters*g_AppsPerAdapter);

    for(DWORD a=0; a<dwNumDxAdapters; a++)
    {
        D3DADAPTER_IDENTIFIER9 Identifier;
        pD3D->GetAdapterIdentifier(a, 0, &Identifier);
        if (strstr(Identifier.Description, "NVIDIA"))
        {
            printf("%s[%d]: Launching master thread on display adapter %s with dx ordinal %d\n", SAMPLE_NAME, _getpid(), Identifier.Description, a);
            for(DWORD dwAppIndex=0; dwAppIndex<g_AppsPerAdapter; dwAppIndex++, dwTotalAppIndex++)
            {
                //  sprintf(pCommandLine, "%s -slave -adapter %d -appIdx %d -w %d -h %d %s", pName, a, dwTotalAppIndex, g_dwSlaveW, g_dwSlaveH, (g_bNoDump)?"-noDump":"");
                sprintf(pCommandLine, "%s -slave -adapter %d -appIdx %d %s -frames %d", pName, a, dwTotalAppIndex, pCMD, g_dwTotalFramesPerSession);
                printf("Launching %s\n", pCommandLine);
                if (CreateProcess(NULL,
                             pCommandLine,
                             NULL,
                             NULL,
                             TRUE,
                             NORMAL_PRIORITY_CLASS,
                             GetEnvironmentStrings(),
                             NULL,     // currentdir
                             &si,
                             &pi))
                {
                    ahSlaveProcesses[dwSlaveProcesses]=pi.hProcess; 
                    printf("%s[%d]: Slave Process %d launched\n", SAMPLE_NAME, _getpid(), dwTotalAppIndex);
	            }
                else
                {
                    printf("%s[%d]: Failed to create Slave process\n", SAMPLE_NAME, _getpid());
                    exit(-1);
                }
                dwSlaveProcesses++;
            }
        }
    }
        
    pD3D->Release();
    Sleep(2000);
        
    for(int i=0; i<dwSlaveProcesses; i++)
    {
        HANDLE hTest;
        hTest=OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetProcessId(ahSlaveProcesses[i]));
        if (!hTest)
        {
            printf("%s[%d]: One of the slave processes did not start correctly, aborting...\n", SAMPLE_NAME, _getpid());
            printf("%s[%d]: Please close all remaining windows..(cmd> taskkill /F /IM DX9IFRH264MultiGPUStress.exe).\n", SAMPLE_NAME, _getpid());
            exit(-1);
        }
        else
        {
            CloseHandle(hTest);
        }
    }

    for(int i=0; i<dwSlaveProcesses; i++)
    {
        WaitForSingleObject(ahSlaveProcesses[i], INFINITE);
    }

    return 0;
}

// Prints the help message
void printHelp()
{
    printf("Usage: Dx9IFRMultiGPUStress  [options]\n");
    printf("  -w                         Target Width. Default : %d\n", WIDTH);
    printf("  -h                         Target Height. Default : %d\n", HEIGHT);
    printf("  -time                      Time (in seconds) for which to run the stress test. Default : \n", g_dwTimeOut);
    // printf("  -frames framecnt           The number of frames to grab.\n");
    printf("  -appsPerAdapter encodercnt The number of concurrent app instances to run. Default: %d\n", g_AppsPerAdapter);
    printf("  -noDump                    Silent mode. No Output files generated.\n");
    printf("  -codec codectype           The codec to use for encode. 0 = H.264, 1 = HEVC");
    printf("  -help                      Prints this help menu.\n");

}

int main(int argc, char * argv[])
{
    // printf("%s[%d]: %s\n", SAMPLE_NAME, _getpid(), __FUNCTION__);
    HINSTANCE inst;
    char buf[1024];
    inst=(HINSTANCE)GetModuleHandle(NULL);
    bool bSlave = false;
    DWORD dwAdapter = 0;
    DWORD dwAppIdx = 0;

    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-help"))
        {
            printHelp();
            return 0;
        }
        else if (!strcmp(argv[i], "-slave"))
        {
            bSlave = true;
            continue;
        }
        else if (!strcmp(argv[i], "-w"))
        {
            g_dwSlaveW = atoi(argv[++i]);
            continue;
        }
        else if (!strcmp(argv[i], "-h"))
        {
            g_dwSlaveH = atoi(argv[++i]);
            continue;
        }
        else if (!strcmp(argv[i], "-noDump"))
        {
            g_bNoDump = true;
            continue;
        }
        else if (!strcmp(argv[i], "-frames"))
        {
            g_dwMaxFrames = atoi(argv[++i]);
            continue;
        }
        else if (!strcmp(argv[i], "-time"))
        {
            g_dwTimeOut = atoi(argv[++i]);
            continue;
        }
        if (!strcmp(argv[i], "-appsPerAdapter"))
        {
            g_AppsPerAdapter = atoi(argv[++i]);
            continue;
        }
        else if (!strcmp(argv[i], "-adapter"))
        {
            dwAdapter = atoi(argv[++i]);
            continue;
        }
        else if (!strcmp(argv[i], "-appIdx"))
        {
            dwAppIdx = atoi(argv[++i]);
            continue;
        }
        else if (!strcmp(argv[i], "-codec"))
        {
            if (i+1 >= argc)
            {
                printf("Missing -codec option\n");
                printHelp();
                return -1;
            }
            g_dwCodec = atoi(argv[++i]);
            if (g_dwCodec < 0 || g_dwCodec > 1)
            {
                printf("Invalid -codec option\n");
                printHelp();
                return -1;
            }
        }
        else
        {
            printf("%s[%d]: Unrecognized arg %s.\n", SAMPLE_NAME, _getpid(), argv[i]);
            printHelp();
        }
    }


    if (bSlave)        // we are a slave process, translate the codec parameter
    {
        if (g_dwCodec == 1)
        {
            g_Codec = NV_HW_ENC_HEVC;
            printf("Using HEVC for encoding\n");
        }
        else if (g_dwCodec == 0)
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


    if (bSlave)        // we are a slave process
    {
        RegisterClassEx(&wc);

        hNvIFRDll = nvIFRLib.load();
        if (sizeof(void *) != 4)
            hNvIFRDll=::LoadLibrary("NvIFR64.dll");
        else
            hNvIFRDll=::LoadLibrary("NvIFR.dll");
    
        if (!hNvIFRDll)
        {
            printf("%s[%d]: could not load NvIFR.dll\n", SAMPLE_NAME, _getpid());
            return -1;
        }

        printf("%s[%d]: Process #%d on adapter #%d.\n", SAMPLE_NAME, _getpid(), dwAppIdx, dwAdapter);
        D3DRenderingMasterThread(dwAdapter, dwAppIdx);
        UnregisterClass(SAMPLE_NAME, wc.hInstance);
        return 0;
    }
    else
    {
        buf[0]=0;
        for(int i=1; i<argc; i++)
        {
            strcat(buf, argv[i]);
            strcat(buf, " ");
        }
        printf("%s[%d]: Start Master process with %d apps per adapter\n", SAMPLE_NAME, _getpid(), g_AppsPerAdapter);
        return _WinMain(inst, NULL, (LPSTR)buf, SW_SHOWNORMAL);
    }
}
