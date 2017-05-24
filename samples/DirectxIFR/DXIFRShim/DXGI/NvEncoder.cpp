

////////////////////////////////////////////////////////////////////////////
//
// Copyright 1993-2014 NVIDIA Corporation.  All rights reserved.
//
// Please refer to the NVIDIA end user license agreement (EULA) associated
// with this source code for terms and conditions that govern your use of
// this software. Any use, reproduction, disclosure, or distribution of
// this software and related documentation outside the terms of the EULA
// is strictly prohibited.
//
////////////////////////////////////////////////////////////////////////////

#include "../common/inc/nvCPUOPSys.h"
#include "../common/inc/nvEncodeAPI.h"
#include "../common/inc/nvUtils.h"
#include "NvEncoder.h"
#include "../common/inc/nvFileIO.h"
#include <new>

#include <iostream>
#include <fstream>

#define BITSTREAM_BUFFER_SIZE 2 * 1024 * 1024

std::ofstream NvEncoderLogFile;

void convertYUVpitchtoNV12(unsigned char *yuv_luma, unsigned char *yuv_cb, unsigned char *yuv_cr,
                           unsigned char *nv12_luma, unsigned char *nv12_chroma,
                           int width, int height, int srcStride, int dstStride)
{
    int y;
    int x;
    if (srcStride == 0)
        srcStride = width;
    if (dstStride == 0)
        dstStride = width;

   for (y = 0; y < height; y++)
   {
       // Just copying it over row by row directly. Simple and straightforward
       memcpy(nv12_luma + (dstStride*y), yuv_luma + (srcStride*y), width);
   }

    for (y = 0; y < height / 2; y++)
    {
        for (x = 0; x < width; x = x + 2)
        {
            nv12_chroma[(y*dstStride) + x] = yuv_cb[((srcStride / 2)*y) + (x >> 1)];
            nv12_chroma[(y*dstStride) + (x + 1)] = yuv_cr[((srcStride / 2)*y) + (x >> 1)];
        }
    }
}

void convertYUVpitchtoYUV444(unsigned char *yuv_luma, unsigned char *yuv_cb, unsigned char *yuv_cr,
    unsigned char *surf_luma, unsigned char *surf_cb, unsigned char *surf_cr, int width, int height, int srcStride, int dstStride)
{
    int h;

    for (h = 0; h < height; h++)
    {
        memcpy(surf_luma + dstStride * h, yuv_luma + srcStride * h, width);
        memcpy(surf_cb + dstStride * h, yuv_cb + srcStride * h, width);
        memcpy(surf_cr + dstStride * h, yuv_cr + srcStride * h, width);
    }
}

CNvEncoder::CNvEncoder(int index)
{
    m_pNvHWEncoder = new CNvHWEncoder(index);
    m_pDevice = NULL;
#if defined (NV_WINDOWS)
    m_pD3D = NULL;
#endif
    m_cuContext = NULL;

    m_uEncodeBufferCount = 0;
    memset(&m_stEncoderInput, 0, sizeof(m_stEncoderInput));
    memset(&m_stEOSOutputBfr, 0, sizeof(m_stEOSOutputBfr));

    memset(&m_stEncodeBuffer, 0, sizeof(m_stEncodeBuffer));
}

CNvEncoder::~CNvEncoder()
{
    if (m_pNvHWEncoder)
    {
        delete m_pNvHWEncoder;
        m_pNvHWEncoder = NULL;
    }
}

NVENCSTATUS CNvEncoder::InitCuda(uint32_t deviceID)
{
    CUresult cuResult;
    CUdevice device;
    CUcontext cuContextCurr;
    int  deviceCount = 0;
    int  SMminor = 0, SMmajor = 0;

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    typedef HMODULE CUDADRIVER;
#else
    typedef void *CUDADRIVER;
#endif
    CUDADRIVER hHandleDriver = 0;
    cuResult = cuInit(0, __CUDA_API_VERSION, hHandleDriver);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuInit error:0x%x\n", cuResult);
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "cuResult != CUDA_SUCCESS.\n";
        NvEncoderLogFile.close();
        assert(0);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuDeviceGetCount(&deviceCount);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuDeviceGetCount error:0x%x\n", cuResult);
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "cuDeviceGetCount error.\n";
        NvEncoderLogFile.close();
        assert(0);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    // If dev is negative value, we clamp to 0
    if ((int)deviceID < 0)
        deviceID = 0;

    if (deviceID >(unsigned int)deviceCount - 1)
    {
        PRINTERR("Invalid Device Id = %d\n", deviceID);
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "Invalid Device Id.\n";
        NvEncoderLogFile.close();
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    cuResult = cuDeviceGet(&device, deviceID);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuDeviceGet error:0x%x\n", cuResult);
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "cuDeviceGet error.\n";
        NvEncoderLogFile.close();
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuDeviceComputeCapability(&SMmajor, &SMminor, deviceID);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuDeviceComputeCapability error:0x%x\n", cuResult);
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "cuDeviceComputeCapability error.\n";
        NvEncoderLogFile.close();
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    if (((SMmajor << 4) + SMminor) < 0x30)
    {
        PRINTERR("GPU %d does not have NVENC capabilities exiting\n", deviceID);
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "GPU does not have NVENC capabilities exiting.\n";
        NvEncoderLogFile.close();
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuCtxCreate((CUcontext*)(&m_pDevice), 0, device);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuCtxCreate error:0x%x\n", cuResult);
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "cuCtxCreate error.\n";
        NvEncoderLogFile.close();
        assert(0);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuCtxPopCurrent(&cuContextCurr);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuCtxPopCurrent error:0x%x\n", cuResult);
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "cuCtxPopCurrent error.\n";
        NvEncoderLogFile.close();
        assert(0);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }
    return NV_ENC_SUCCESS;
}

#if defined(NV_WINDOWS)
NVENCSTATUS CNvEncoder::InitD3D9(uint32_t deviceID)
{
    D3DPRESENT_PARAMETERS d3dpp;
    D3DADAPTER_IDENTIFIER9 adapterId;
    unsigned int iAdapter = NULL; // Our adapter
    HRESULT hr = S_OK;

    m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (m_pD3D == NULL)
    {
        assert(m_pD3D);
        return NV_ENC_ERR_OUT_OF_MEMORY;;
    }

    if (deviceID >= m_pD3D->GetAdapterCount())
    {
        PRINTERR("Invalid Device Id = %d\n. Please use DX10/DX11 to detect headless video devices.\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    hr = m_pD3D->GetAdapterIdentifier(deviceID, 0, &adapterId);
    if (hr != S_OK)
    {
        PRINTERR("Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferWidth = 640;
    d3dpp.BackBufferHeight = 480;
    d3dpp.BackBufferCount = 1;
    d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    d3dpp.Flags = D3DPRESENTFLAG_VIDEO;//D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    DWORD dwBehaviorFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING;

    hr = m_pD3D->CreateDevice(deviceID,
        D3DDEVTYPE_HAL,
        GetDesktopWindow(),
        dwBehaviorFlags,
        &d3dpp,
        (IDirect3DDevice9**)(&m_pDevice));

    if (FAILED(hr))
        return NV_ENC_ERR_OUT_OF_MEMORY;

    return  NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::InitD3D10(uint32_t deviceID)
{
    HRESULT hr;
    IDXGIFactory * pFactory = NULL;
    IDXGIAdapter * pAdapter;

    if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory) != S_OK)
    {
        return NV_ENC_ERR_GENERIC;
    }

    if (pFactory->EnumAdapters(deviceID, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        hr = D3D10CreateDevice(pAdapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
            D3D10_SDK_VERSION, (ID3D10Device**)(&m_pDevice));
        if (FAILED(hr))
        {
            PRINTERR("Problem while creating %d D3d10 device \n", deviceID);
            return NV_ENC_ERR_OUT_OF_MEMORY;
        }
    }
    else
    {
        PRINTERR("Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    return  NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::InitD3D11(uint32_t deviceID)
{
    HRESULT hr;
    IDXGIFactory * pFactory = NULL;
    IDXGIAdapter * pAdapter;

    if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory) != S_OK)
    {
        return NV_ENC_ERR_GENERIC;
    }

    if (pFactory->EnumAdapters(deviceID, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        hr = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0,
            NULL, 0, D3D11_SDK_VERSION, (ID3D11Device**)(&m_pDevice), NULL, NULL);
        if (FAILED(hr))
        {
            PRINTERR("Problem while creating %d D3d11 device \n", deviceID);
            return NV_ENC_ERR_OUT_OF_MEMORY;
        }
    }
    else
    {
        PRINTERR("Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    return  NV_ENC_SUCCESS;
}
#endif

NVENCSTATUS CNvEncoder::AllocateIOBuffers(uint32_t uInputWidth, uint32_t uInputHeight, uint32_t isYuv444)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    m_EncodeBufferQueue.Initialize(m_stEncodeBuffer, m_uEncodeBufferCount);
    for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
    {
        nvStatus = m_pNvHWEncoder->NvEncCreateInputBuffer(uInputWidth, uInputHeight, &m_stEncodeBuffer[i].stInputBfr.hInputSurface, isYuv444);
        if (nvStatus != NV_ENC_SUCCESS)
        {
            NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
            NvEncoderLogFile << "m_pNvHWEncoder->NvEncCreateInputBuffer error.\n";
            NvEncoderLogFile.close();
            return nvStatus;
        }

        m_stEncodeBuffer[i].stInputBfr.bufferFmt = isYuv444 ? NV_ENC_BUFFER_FORMAT_YUV444_PL : NV_ENC_BUFFER_FORMAT_NV12_PL;
        //m_stEncodeBuffer[i].stInputBfr.bufferFmt = NV_ENC_BUFFER_FORMAT_IYUV_PL;
        m_stEncodeBuffer[i].stInputBfr.dwWidth = uInputWidth;
        m_stEncodeBuffer[i].stInputBfr.dwHeight = uInputHeight;

        //Allocate output surface
        nvStatus = m_pNvHWEncoder->NvEncCreateBitstreamBuffer(BITSTREAM_BUFFER_SIZE, &m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
        if (nvStatus != NV_ENC_SUCCESS)
        {
            NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
            NvEncoderLogFile << "NvEncCreateBitstreamBuffer failed.\n";
            NvEncoderLogFile.close();
            return nvStatus;
        }
        m_stEncodeBuffer[i].stOutputBfr.dwBitstreamBufferSize = BITSTREAM_BUFFER_SIZE;

#if defined (NV_WINDOWS)
        nvStatus = m_pNvHWEncoder->NvEncRegisterAsyncEvent(&m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
        if (nvStatus != NV_ENC_SUCCESS)
        {
            NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
            NvEncoderLogFile << "NvEncRegisterAsyncEvent failed.\n";
            NvEncoderLogFile.close();
            return nvStatus;
        }
        if (m_stEncoderInput.enableMEOnly)
        {
            m_stEncodeBuffer[i].stOutputBfr.bWaitOnEvent = false;
        }
        else
            m_stEncodeBuffer[i].stOutputBfr.bWaitOnEvent = true;
#else
        m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = NULL;
#endif
    }

    m_stEOSOutputBfr.bEOSFlag = TRUE;

#if defined (NV_WINDOWS)
    nvStatus = m_pNvHWEncoder->NvEncRegisterAsyncEvent(&m_stEOSOutputBfr.hOutputEvent);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "NvEncRegisterAsyncEvent failed.\n";
        NvEncoderLogFile.close();
        return nvStatus;
    }
#else
    m_stEOSOutputBfr.hOutputEvent = NULL;
#endif

    return NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::ReleaseIOBuffers()
{
    for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
    {
        m_pNvHWEncoder->NvEncDestroyInputBuffer(m_stEncodeBuffer[i].stInputBfr.hInputSurface);
        m_stEncodeBuffer[i].stInputBfr.hInputSurface = NULL;

        if (m_stEncoderInput.enableMEOnly)
        {
            m_pNvHWEncoder->NvEncDestroyMVBuffer(m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
            m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer = NULL;
        }
        else
        {
            m_pNvHWEncoder->NvEncDestroyBitstreamBuffer(m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
            m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer = NULL;
        }

#if defined(NV_WINDOWS)
        m_pNvHWEncoder->NvEncUnregisterAsyncEvent(m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
        nvCloseFile(m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
        m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = NULL;
#endif
    }

    if (m_stEOSOutputBfr.hOutputEvent)
    {
#if defined(NV_WINDOWS)
        m_pNvHWEncoder->NvEncUnregisterAsyncEvent(m_stEOSOutputBfr.hOutputEvent);
        nvCloseFile(m_stEOSOutputBfr.hOutputEvent);
        m_stEOSOutputBfr.hOutputEvent = NULL;
#endif
    }

    return NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::FlushEncoder(int index)
{
    NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
    NvEncoderLogFile << "FlushEncoder() error.\n";
    NvEncoderLogFile.close();

    NVENCSTATUS nvStatus = m_pNvHWEncoder->NvEncFlushEncoderQueue(m_stEOSOutputBfr.hOutputEvent);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "m_pNvHWEncoder->NvEncFlushEncoderQueue error.\n";
        NvEncoderLogFile.close();
        return nvStatus;
    }

    EncodeBuffer *pEncodeBufer = m_EncodeBufferQueue.GetPending();
    while (pEncodeBufer)
    {
        m_pNvHWEncoder->ProcessOutput(pEncodeBufer, index);
        pEncodeBufer = m_EncodeBufferQueue.GetPending();
    }

#if defined(NV_WINDOWS)
    if (WaitForSingleObject(m_stEOSOutputBfr.hOutputEvent, 500) != WAIT_OBJECT_0)
    {
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "WaitForSingleObject(m_stEOSOutputBfr.hOutputEvent, 500) error.\n";
        NvEncoderLogFile.close();
        assert(0);
        nvStatus = NV_ENC_ERR_GENERIC;
    }
#endif

    return nvStatus;
}

NVENCSTATUS CNvEncoder::Deinitialize(uint32_t devicetype)
{
    NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
    NvEncoderLogFile << "Deinitialize() error.\n";
    NvEncoderLogFile.close();

    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    ReleaseIOBuffers();

    nvStatus = m_pNvHWEncoder->NvEncDestroyEncoder();

    if (m_pDevice)
    {
        switch (devicetype)
        {
#if defined(NV_WINDOWS)
        case NV_ENC_DX9:
            ((IDirect3DDevice9*)(m_pDevice))->Release();
            break;

        case NV_ENC_DX10:
            ((ID3D10Device*)(m_pDevice))->Release();
            break;

        case NV_ENC_DX11:
            ((ID3D11Device*)(m_pDevice))->Release();
            break;
#endif

        case NV_ENC_CUDA:
            CUresult cuResult = CUDA_SUCCESS;
            cuResult = cuCtxDestroy((CUcontext)m_pDevice);
            if (cuResult != CUDA_SUCCESS)
            {
                NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
                NvEncoderLogFile << "cuCtxDestroy() error.\n";
                NvEncoderLogFile.close();
                PRINTERR("cuCtxDestroy error:0x%x\n", cuResult);
            }
        }

        m_pDevice = NULL;
    }

#if defined (NV_WINDOWS)
    if (m_pD3D)
    {
        m_pD3D->Release();
        m_pD3D = NULL;
    }
#endif

    return nvStatus;
}

NVENCSTATUS loadframe(uint8_t *yuvInput[3], HANDLE hInputYUVFile, uint32_t frmIdx, uint32_t width, uint32_t height, uint32_t &numBytesRead, uint32_t isYuv444)
{
    uint64_t fileOffset;
    uint32_t result;
    //Set size depending on whether it is YUV 444 or YUV 420
    uint32_t dwInFrameSize = isYuv444 ? width * height * 3 : width*height + (width*height) / 2;
    fileOffset = (uint64_t)dwInFrameSize * frmIdx;
    result = nvSetFilePointer64(hInputYUVFile, fileOffset, NULL, FILE_BEGIN);
    if (result == INVALID_SET_FILE_POINTER)
    {
        return NV_ENC_ERR_INVALID_PARAM;
    }
    if (isYuv444)
    {
        nvReadFile(hInputYUVFile, yuvInput[0], width * height, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[1], width * height, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[2], width * height, &numBytesRead, NULL);
    }
    else
    {
        nvReadFile(hInputYUVFile, yuvInput[0], width * height, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[1], width * height / 4, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[2], width * height / 4, &numBytesRead, NULL);
    }
    return NV_ENC_SUCCESS;
}
int lumaPlaneSize, chromaPlaneSize;

int CNvEncoder::EncodeMain(int index, int width, int height, int fps, int initialBitrate)
{
    uint8_t *yuv[3];
    
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::trunc);
    NvEncoderLogFile.close();

    memset(&encodeConfig, 0, sizeof(EncodeConfig));

    encodeConfig.endFrameIdx = INT_MAX;
    encodeConfig.bitrate = initialBitrate;
    encodeConfig.rcMode = NV_ENC_PARAMS_RC_VBR;
    encodeConfig.gopLength = NVENC_INFINITE_GOPLENGTH;
    encodeConfig.deviceType = NV_ENC_CUDA;
    encodeConfig.codec = NV_ENC_H264;
    encodeConfig.fps = fps;
    encodeConfig.qp = 28;
    encodeConfig.i_quant_factor = DEFAULT_I_QFACTOR;
    encodeConfig.b_quant_factor = DEFAULT_B_QFACTOR;
    encodeConfig.i_quant_offset = DEFAULT_I_QOFFSET;
    encodeConfig.b_quant_offset = DEFAULT_B_QOFFSET;
    encodeConfig.presetGUID = NV_ENC_PRESET_LOW_LATENCY_HP_GUID;
    encodeConfig.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    encodeConfig.isYuv444 = 0;
    encodeConfig.width = width;
    encodeConfig.height = height;
    encodeConfig.vbvSize = 0;
    encodeConfig.numB = 0;

    switch (encodeConfig.deviceType)
    {
#if defined(NV_WINDOWS)
    case NV_ENC_DX9:
        InitD3D9(encodeConfig.deviceID);
        break;

    case NV_ENC_DX10:
        InitD3D10(encodeConfig.deviceID);
        break;

    case NV_ENC_DX11:
        InitD3D11(encodeConfig.deviceID);
        break;
#endif
    case NV_ENC_CUDA:
        InitCuda(encodeConfig.deviceID);
        break;
    }

    if (encodeConfig.deviceType != NV_ENC_CUDA)
        nvStatus = m_pNvHWEncoder->Initialize(m_pDevice, NV_ENC_DEVICE_TYPE_DIRECTX);
    else
        nvStatus = m_pNvHWEncoder->Initialize(m_pDevice, NV_ENC_DEVICE_TYPE_CUDA);

    if (nvStatus != NV_ENC_SUCCESS)
    {
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "m_pNvHWEncoder->Initialize failed.\n";
        NvEncoderLogFile.close();
        return 1;
    }

    encodeConfig.presetGUID = m_pNvHWEncoder->GetPresetGUID(encodeConfig.encoderPreset, encodeConfig.codec);

    nvStatus = m_pNvHWEncoder->CreateEncoder(&encodeConfig, index);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "m_pNvHWEncoder->CreateEncoder failed.\n";
        NvEncoderLogFile.close();
        return 1;
    }
    encodeConfig.maxWidth = encodeConfig.maxWidth ? encodeConfig.maxWidth : encodeConfig.width;
    encodeConfig.maxHeight = encodeConfig.maxHeight ? encodeConfig.maxHeight : encodeConfig.height;

    if (encodeConfig.numB > 0)
    {
        m_uEncodeBufferCount = encodeConfig.numB + 4; // min buffers is numb + 1 + 3 pipelining
    }
    else
    {
        int numMBs = ((encodeConfig.maxHeight + 15) >> 4) * ((encodeConfig.maxWidth + 15) >> 4);
        int NumIOBuffers;
        if (numMBs >= 32768) //4kx2k
            NumIOBuffers = MAX_ENCODE_QUEUE / 8;
        else if (numMBs >= 16384) // 2kx2k
            NumIOBuffers = MAX_ENCODE_QUEUE / 4;
        else if (numMBs >= 8160) // 1920x1080
            NumIOBuffers = MAX_ENCODE_QUEUE / 2;
        else
            NumIOBuffers = MAX_ENCODE_QUEUE;
        m_uEncodeBufferCount = 1;
        //m_uEncodeBufferCount = NumIOBuffers;
    }
    m_uPicStruct = encodeConfig.pictureStruct;
    nvStatus = AllocateIOBuffers(encodeConfig.width, encodeConfig.height, encodeConfig.isYuv444);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "AllocateIOBuffers failed.\n";
        NvEncoderLogFile.close();
        return 1;
    }

    uint32_t  chromaFormatIDC = (encodeConfig.isYuv444 ? 3 : 1);
    lumaPlaneSize = encodeConfig.maxWidth * encodeConfig.maxHeight;
    chromaPlaneSize = (chromaFormatIDC == 3) ? lumaPlaneSize : (lumaPlaneSize >> 2);

    yuv[0] = new(std::nothrow) uint8_t[lumaPlaneSize];
    yuv[1] = new(std::nothrow) uint8_t[chromaPlaneSize];
    yuv[2] = new(std::nothrow) uint8_t[chromaPlaneSize];

    if (yuv[0] == NULL || yuv[1] == NULL || yuv[2] == NULL)
    {
        PRINTERR("\nvEncoder.exe Error: Failed to allocate memory for yuv array!\n");
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "Error: Failed to allocate memory for yuv array.\n";
        NvEncoderLogFile.close();
        return 1;
    }

    return 0;
}

void CNvEncoder::ShutdownNvEncoder()
{
    if (encodeConfig.fOutput)
    {
        fclose(encodeConfig.fOutput);
    }

    Deinitialize(encodeConfig.deviceType);
}

void CNvEncoder::EncodeFrameLoop(uint8_t *buffer, bool isReconfiguringBitrate, int index, int targetBitrate)
{
    //numBytesRead = 0;
    //loadframe(yuv, hInput, frm, encodeConfig.width, encodeConfig.height, numBytesRead, encodeConfig.isYuv444);
    //if (numBytesRead == 0)
    //    break;

    EncodeFrameConfig stEncodeFrame;
    memset(&stEncodeFrame, 0, sizeof(stEncodeFrame));
    
    stEncodeFrame.stride[0] = encodeConfig.width;
    stEncodeFrame.stride[1] = (encodeConfig.isYuv444) ? encodeConfig.width : encodeConfig.width / 2;
    stEncodeFrame.stride[2] = (encodeConfig.isYuv444) ? encodeConfig.width : encodeConfig.width / 2;
    stEncodeFrame.width = encodeConfig.width;
    stEncodeFrame.height = encodeConfig.height;

    stEncodeFrame.yuv[0] = buffer;//yuv[0];
    stEncodeFrame.yuv[1] = buffer + (stEncodeFrame.stride[0] * encodeConfig.height);//yuv[1];
    stEncodeFrame.yuv[2] = buffer + (stEncodeFrame.stride[0] * encodeConfig.height * 5 / 4);//yuv[2];

    EncodeFrame(&stEncodeFrame, index, false, encodeConfig.width, encodeConfig.height);

    if (isReconfiguringBitrate == true)
    {
        NvEncPictureCommand encPicCommand;
    
        encPicCommand.bBitrateChangePending = true;
        encPicCommand.newVBVSize = 0;
        encPicCommand.newBitrate = targetBitrate;
    
        encPicCommand.bResolutionChangePending = false;
        //encPicCommand.newHeight = 640;
        //encPicCommand.newWidth = 480;
    
        NVENCSTATUS status = m_pNvHWEncoder->NvEncReconfigureEncoder(&encPicCommand);
        if (status != NV_ENC_SUCCESS)
        {
            // Common error: NV_ENC_ERR_INVALID_PARAM (== 8)
            printf("Bitrate changing failed! Error is %d\n", status);
            NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
            NvEncoderLogFile << "Bitrate changing failed! Error is " << status << "\n";
            NvEncoderLogFile.close();
        }
    }
}

NVENCSTATUS CNvEncoder::EncodeFrame(EncodeFrameConfig *pEncodeFrame, int index, bool bFlush, uint32_t width, uint32_t height)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    uint32_t lockedPitch = 0;
    EncodeBuffer *pEncodeBuffer = NULL;

    if (bFlush)
    {
        // Does not run
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "if (bFlush).\n";
        NvEncoderLogFile.close();
        FlushEncoder(index);
        return NV_ENC_SUCCESS;
    }

    if (!pEncodeFrame)
    {
        // Does not run
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "pEncodeFrame is NULL. NV_ENC_ERR_INVALID_PARAM.\n";
        NvEncoderLogFile.close();
        return NV_ENC_ERR_INVALID_PARAM;
    }

    pEncodeBuffer = m_EncodeBufferQueue.GetAvailable();
    if (!pEncodeBuffer)
    {
        // This does run
        m_pNvHWEncoder->ProcessOutput(m_EncodeBufferQueue.GetPending(), index);
        pEncodeBuffer = m_EncodeBufferQueue.GetAvailable();
    }

    unsigned char *pInputSurface;

    nvStatus = m_pNvHWEncoder->NvEncLockInputBuffer(pEncodeBuffer->stInputBfr.hInputSurface, (void**)&pInputSurface, &lockedPitch);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        // Does not run
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "m_pNvHWEncoder->NvEncLockInputBuffer.\n";
        NvEncoderLogFile.close();
        return nvStatus;
    }

    if (pEncodeBuffer->stInputBfr.bufferFmt == NV_ENC_BUFFER_FORMAT_NV12_PL)
    {
        unsigned char *pInputSurfaceCh = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight*lockedPitch);
        convertYUVpitchtoNV12(pEncodeFrame->yuv[0], pEncodeFrame->yuv[1], pEncodeFrame->yuv[2], pInputSurface, pInputSurfaceCh, width, height, width, lockedPitch);
    }
    else
    {
        // Does not run
        unsigned char *pInputSurfaceCb = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
        unsigned char *pInputSurfaceCr = pInputSurfaceCb + (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
        convertYUVpitchtoYUV444(pEncodeFrame->yuv[0], pEncodeFrame->yuv[1], pEncodeFrame->yuv[2], pInputSurface, pInputSurfaceCb, pInputSurfaceCr, width, height, width, lockedPitch);
    }
    nvStatus = m_pNvHWEncoder->NvEncUnlockInputBuffer(pEncodeBuffer->stInputBfr.hInputSurface);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        // Does not run
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "m_pNvHWEncoder->NvEncUnlockInputBuffer.\n";
        NvEncoderLogFile.close();
        return nvStatus;
    }
    nvStatus = m_pNvHWEncoder->NvEncEncodeFrame(pEncodeBuffer, NULL, width, height, (NV_ENC_PIC_STRUCT)m_uPicStruct);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        NvEncoderLogFile.open("NvEncoderLogFile.txt", std::ios::app);
        NvEncoderLogFile << "m_pNvHWEncoder->NvEncEncodeFrame\n";
        NvEncoderLogFile.close();
    }
    return nvStatus;
}