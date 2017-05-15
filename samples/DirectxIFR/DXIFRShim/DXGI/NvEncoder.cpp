//////////////////////////////////////////////////////////////////////////////
////
//// Copyright 1993-2014 NVIDIA Corporation.  All rights reserved.
////
//// Please refer to the NVIDIA end user license agreement (EULA) associated
//// with this source code for terms and conditions that govern your use of
//// this software. Any use, reproduction, disclosure, or distribution of
//// this software and related documentation outside the terms of the EULA
//// is strictly prohibited.
////
//////////////////////////////////////////////////////////////////////////////
//
//#include "../common/inc/nvCPUOPSys.h"
//#include "../common/inc/nvEncodeAPI.h"
//#include "../common/inc/nvUtils.h"
//#include "NvEncoder.h"
//#include "../common/inc/nvFileIO.h"
//#include <new>
//
//#define BITSTREAM_BUFFER_SIZE 2 * 1024 * 1024
//
//void convertYUVpitchtoNV12( unsigned char *yuv_luma, unsigned char *yuv_cb, unsigned char *yuv_cr,
//                            unsigned char *nv12_luma, unsigned char *nv12_chroma,
//                            int width, int height , int srcStride, int dstStride)
//{
//    int y;
//    int x;
//    if (srcStride == 0)
//        srcStride = width;
//    if (dstStride == 0)
//        dstStride = width;
//
//    for ( y = 0 ; y < height ; y++)
//    {
//        memcpy( nv12_luma + (dstStride*y), yuv_luma + (srcStride*y) , width );
//    }
//
//    for ( y = 0 ; y < height/2 ; y++)
//    {
//        for ( x= 0 ; x < width; x=x+2)
//        {
//            nv12_chroma[(y*dstStride) + x] =    yuv_cb[((srcStride/2)*y) + (x >>1)];
//            nv12_chroma[(y*dstStride) +(x+1)] = yuv_cr[((srcStride/2)*y) + (x >>1)];
//        }
//    }
//}
//
//void convertYUVpitchtoYUV444(unsigned char *yuv_luma, unsigned char *yuv_cb, unsigned char *yuv_cr,
//    unsigned char *surf_luma, unsigned char *surf_cb, unsigned char *surf_cr, int width, int height, int srcStride, int dstStride)
//{
//    int h;
//
//    for (h = 0; h < height; h++)
//    {
//        memcpy(surf_luma + dstStride * h, yuv_luma + srcStride * h, width);
//        memcpy(surf_cb + dstStride * h, yuv_cb + srcStride * h, width);
//        memcpy(surf_cr + dstStride * h, yuv_cr + srcStride * h, width);
//    }
//}
//
//CNvEncoder::CNvEncoder()
//{
//    m_pNvHWEncoder = new CNvHWEncoder;
//    m_pDevice = NULL;
//#if defined (NV_WINDOWS)
//    m_pD3D = NULL;
//#endif
//    m_cuContext = NULL;
//
//    m_uEncodeBufferCount = 0;
//    memset(&m_stEncoderInput, 0, sizeof(m_stEncoderInput));
//    memset(&m_stEOSOutputBfr, 0, sizeof(m_stEOSOutputBfr));
//
//    memset(&m_stEncodeBuffer, 0, sizeof(m_stEncodeBuffer));
//}
//
//CNvEncoder::~CNvEncoder()
//{
//    if (m_pNvHWEncoder)
//    {
//        delete m_pNvHWEncoder;
//        m_pNvHWEncoder = NULL;
//    }
//}
//
//NVENCSTATUS CNvEncoder::InitCuda(uint32_t deviceID)
//{
//    CUresult cuResult;
//    CUdevice device;
//    CUcontext cuContextCurr;
//    int  deviceCount = 0;
//    int  SMminor = 0, SMmajor = 0;
//
//#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
//    typedef HMODULE CUDADRIVER;
//#else
//    typedef void *CUDADRIVER;
//#endif
//    CUDADRIVER hHandleDriver = 0;
//    cuResult = cuInit(0, __CUDA_API_VERSION, hHandleDriver);
//    if (cuResult != CUDA_SUCCESS)
//    {
//        PRINTERR("cuInit error:0x%x\n", cuResult);
//        assert(0);
//        return NV_ENC_ERR_NO_ENCODE_DEVICE;
//    }
//
//    cuResult = cuDeviceGetCount(&deviceCount);
//    if (cuResult != CUDA_SUCCESS)
//    {
//        PRINTERR("cuDeviceGetCount error:0x%x\n", cuResult);
//        assert(0);
//        return NV_ENC_ERR_NO_ENCODE_DEVICE;
//    }
//
//    // If dev is negative value, we clamp to 0
//    if ((int)deviceID < 0)
//        deviceID = 0;
//
//    if (deviceID >(unsigned int)deviceCount - 1)
//    {
//        PRINTERR("Invalid Device Id = %d\n", deviceID);
//        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
//    }
//
//    cuResult = cuDeviceGet(&device, deviceID);
//    if (cuResult != CUDA_SUCCESS)
//    {
//        PRINTERR("cuDeviceGet error:0x%x\n", cuResult);
//        return NV_ENC_ERR_NO_ENCODE_DEVICE;
//    }
//
//    cuResult = cuDeviceComputeCapability(&SMmajor, &SMminor, deviceID);
//    if (cuResult != CUDA_SUCCESS)
//    {
//        PRINTERR("cuDeviceComputeCapability error:0x%x\n", cuResult);
//        return NV_ENC_ERR_NO_ENCODE_DEVICE;
//    }
//
//    if (((SMmajor << 4) + SMminor) < 0x30)
//    {
//        PRINTERR("GPU %d does not have NVENC capabilities exiting\n", deviceID);
//        return NV_ENC_ERR_NO_ENCODE_DEVICE;
//    }
//
//    cuResult = cuCtxCreate((CUcontext*)(&m_pDevice), 0, device);
//    if (cuResult != CUDA_SUCCESS)
//    {
//        PRINTERR("cuCtxCreate error:0x%x\n", cuResult);
//        assert(0);
//        return NV_ENC_ERR_NO_ENCODE_DEVICE;
//    }
//
//    cuResult = cuCtxPopCurrent(&cuContextCurr);
//    if (cuResult != CUDA_SUCCESS)
//    {
//        PRINTERR("cuCtxPopCurrent error:0x%x\n", cuResult);
//        assert(0);
//        return NV_ENC_ERR_NO_ENCODE_DEVICE;
//    }
//    return NV_ENC_SUCCESS;
//}
//
//#if defined(NV_WINDOWS)
//NVENCSTATUS CNvEncoder::InitD3D10(uint32_t deviceID)
//{
//    HRESULT hr;
//    IDXGIFactory * pFactory = NULL;
//    IDXGIAdapter * pAdapter;
//
//    if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory) != S_OK)
//    {
//        return NV_ENC_ERR_GENERIC;
//    }
//
//    if (pFactory->EnumAdapters(deviceID, &pAdapter) != DXGI_ERROR_NOT_FOUND)
//    {
//        hr = D3D10CreateDevice(pAdapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
//            D3D10_SDK_VERSION, (ID3D10Device**)(&m_pDevice));
//        if (FAILED(hr))
//        {
//            PRINTERR("Problem while creating %d D3d10 device \n", deviceID);
//            return NV_ENC_ERR_OUT_OF_MEMORY;
//        }
//    }
//    else
//    {
//        PRINTERR("Invalid Device Id = %d\n", deviceID);
//        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
//    }
//
//    return  NV_ENC_SUCCESS;
//}
//
//NVENCSTATUS CNvEncoder::InitD3D11(uint32_t deviceID)
//{
//    HRESULT hr;
//    IDXGIFactory * pFactory = NULL;
//    IDXGIAdapter * pAdapter;
//
//    if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory) != S_OK)
//    {
//        return NV_ENC_ERR_GENERIC;
//    }
//
//    if (pFactory->EnumAdapters(deviceID, &pAdapter) != DXGI_ERROR_NOT_FOUND)
//    {
//        hr = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0,
//            NULL, 0, D3D11_SDK_VERSION, (ID3D11Device**)(&m_pDevice), NULL, NULL);
//        if (FAILED(hr))
//        {
//            PRINTERR("Problem while creating %d D3d11 device \n", deviceID);
//            return NV_ENC_ERR_OUT_OF_MEMORY;
//        }
//    }
//    else
//    {
//        PRINTERR("Invalid Device Id = %d\n", deviceID);
//        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
//    }
//
//    return  NV_ENC_SUCCESS;
//}
//#endif
//
//NVENCSTATUS CNvEncoder::AllocateIOBuffers(uint32_t uInputWidth, uint32_t uInputHeight, uint32_t isYuv444)
//{
//    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
//
//    m_EncodeBufferQueue.Initialize(m_stEncodeBuffer, m_uEncodeBufferCount);
//    for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
//    {
//        nvStatus = m_pNvHWEncoder->NvEncCreateInputBuffer(uInputWidth, uInputHeight, &m_stEncodeBuffer[i].stInputBfr.hInputSurface, isYuv444);
//        if (nvStatus != NV_ENC_SUCCESS)
//            return nvStatus;
//
//        m_stEncodeBuffer[i].stInputBfr.bufferFmt = isYuv444 ? NV_ENC_BUFFER_FORMAT_YUV444_PL : NV_ENC_BUFFER_FORMAT_NV12_PL;
//        m_stEncodeBuffer[i].stInputBfr.dwWidth = uInputWidth;
//        m_stEncodeBuffer[i].stInputBfr.dwHeight = uInputHeight;
//
//        //Allocate output surface
//        if (m_stEncoderInput.enableMEOnly)
//        {
//            uint32_t encodeWidthInMbs = (uInputWidth + 15) >> 4;
//            uint32_t encodeHeightInMbs = (uInputHeight + 15) >> 4;
//            uint32_t dwSize = encodeWidthInMbs * encodeHeightInMbs * 64;
//            nvStatus = m_pNvHWEncoder->NvEncCreateMVBuffer(dwSize, &m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
//            if (nvStatus != NV_ENC_SUCCESS)
//            {
//                PRINTERR("nvEncCreateMVBuffer error:0x%x\n", nvStatus);
//                return nvStatus;
//            }
//            m_stEncodeBuffer[i].stOutputBfr.dwBitstreamBufferSize = dwSize;
//        }
//        else
//        {
//            nvStatus = m_pNvHWEncoder->NvEncCreateBitstreamBuffer(BITSTREAM_BUFFER_SIZE, &m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
//            if (nvStatus != NV_ENC_SUCCESS)
//                return nvStatus;
//            m_stEncodeBuffer[i].stOutputBfr.dwBitstreamBufferSize = BITSTREAM_BUFFER_SIZE;
//        }
//
//#if defined (NV_WINDOWS)
//        nvStatus = m_pNvHWEncoder->NvEncRegisterAsyncEvent(&m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
//        if (nvStatus != NV_ENC_SUCCESS)
//            return nvStatus;
//        if (m_stEncoderInput.enableMEOnly)
//        {
//            m_stEncodeBuffer[i].stOutputBfr.bWaitOnEvent = false;
//        }
//        else
//            m_stEncodeBuffer[i].stOutputBfr.bWaitOnEvent = true;
//#else
//        m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = NULL;
//#endif
//    }
//
//    m_stEOSOutputBfr.bEOSFlag = TRUE;
//
//#if defined (NV_WINDOWS)
//    nvStatus = m_pNvHWEncoder->NvEncRegisterAsyncEvent(&m_stEOSOutputBfr.hOutputEvent);
//    if (nvStatus != NV_ENC_SUCCESS)
//        return nvStatus; 
//#else
//    m_stEOSOutputBfr.hOutputEvent = NULL;
//#endif
//
//    return NV_ENC_SUCCESS;
//}
//
//NVENCSTATUS CNvEncoder::ReleaseIOBuffers()
//{
//    for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
//    {
//        m_pNvHWEncoder->NvEncDestroyInputBuffer(m_stEncodeBuffer[i].stInputBfr.hInputSurface);
//        m_stEncodeBuffer[i].stInputBfr.hInputSurface = NULL;
//
//        if (m_stEncoderInput.enableMEOnly)
//        {
//            m_pNvHWEncoder->NvEncDestroyMVBuffer(m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
//            m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer = NULL;
//        }
//        else
//        {
//            m_pNvHWEncoder->NvEncDestroyBitstreamBuffer(m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
//            m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer = NULL;
//        }
//
//#if defined(NV_WINDOWS)
//        m_pNvHWEncoder->NvEncUnregisterAsyncEvent(m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
//        nvCloseFile(m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
//        m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = NULL;
//#endif
//    }
//
//    if (m_stEOSOutputBfr.hOutputEvent)
//    {
//#if defined(NV_WINDOWS)
//        m_pNvHWEncoder->NvEncUnregisterAsyncEvent(m_stEOSOutputBfr.hOutputEvent);
//        nvCloseFile(m_stEOSOutputBfr.hOutputEvent);
//        m_stEOSOutputBfr.hOutputEvent = NULL;
//#endif
//    }
//
//    return NV_ENC_SUCCESS;
//}
//
//NVENCSTATUS CNvEncoder::FlushEncoder()
//{
//    NVENCSTATUS nvStatus = m_pNvHWEncoder->NvEncFlushEncoderQueue(m_stEOSOutputBfr.hOutputEvent);
//    if (nvStatus != NV_ENC_SUCCESS)
//    {
//        assert(0);
//        return nvStatus;
//    }
//
//    EncodeBuffer *pEncodeBufer = m_EncodeBufferQueue.GetPending();
//    while (pEncodeBufer)
//    {
//        m_pNvHWEncoder->ProcessOutput(pEncodeBufer);
//        pEncodeBufer = m_EncodeBufferQueue.GetPending();
//    }
//
//#if defined(NV_WINDOWS)
//    if (WaitForSingleObject(m_stEOSOutputBfr.hOutputEvent, 500) != WAIT_OBJECT_0)
//    {
//        assert(0);
//        nvStatus = NV_ENC_ERR_GENERIC;
//    }
//#endif
//
//    return nvStatus;
//}
//
//NVENCSTATUS CNvEncoder::Deinitialize(uint32_t devicetype)
//{
//    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
//
//    ReleaseIOBuffers();
//
//    nvStatus = m_pNvHWEncoder->NvEncDestroyEncoder();
//
//    if (m_pDevice)
//    {
//        switch (devicetype)
//        {
//#if defined(NV_WINDOWS)
//        case NV_ENC_DX9:
//            ((IDirect3DDevice9*)(m_pDevice))->Release();
//            break;
//
//        case NV_ENC_DX10:
//            ((ID3D10Device*)(m_pDevice))->Release();
//            break;
//
//        case NV_ENC_DX11:
//            ((ID3D11Device*)(m_pDevice))->Release();
//            break;
//#endif
//
//        case NV_ENC_CUDA:
//            CUresult cuResult = CUDA_SUCCESS;
//            cuResult = cuCtxDestroy((CUcontext)m_pDevice);
//            if (cuResult != CUDA_SUCCESS)
//                PRINTERR("cuCtxDestroy error:0x%x\n", cuResult);
//        }
//
//        m_pDevice = NULL;
//    }
//
//#if defined (NV_WINDOWS)
//    if (m_pD3D)
//    {
//        m_pD3D->Release();
//        m_pD3D = NULL;
//    }
//#endif
//
//    return nvStatus;
//}
//
//NVENCSTATUS loadframe(uint8_t *yuvInput[3], HANDLE hInputYUVFile, uint32_t frmIdx, uint32_t width, uint32_t height, uint32_t &numBytesRead, uint32_t isYuv444)
//{
//    uint64_t fileOffset;
//    uint32_t result;
//    //Set size depending on whether it is YUV 444 or YUV 420
//    uint32_t dwInFrameSize = isYuv444 ? width * height * 3 : width*height + (width*height) / 2;
//    fileOffset = (uint64_t)dwInFrameSize * frmIdx;
//    result = nvSetFilePointer64(hInputYUVFile, fileOffset, NULL, FILE_BEGIN);
//    if (result == INVALID_SET_FILE_POINTER)
//    {
//        return NV_ENC_ERR_INVALID_PARAM;
//    }
//    if (isYuv444)
//    {
//        nvReadFile(hInputYUVFile, yuvInput[0], width * height, &numBytesRead, NULL);
//        nvReadFile(hInputYUVFile, yuvInput[1], width * height, &numBytesRead, NULL);
//        nvReadFile(hInputYUVFile, yuvInput[2], width * height, &numBytesRead, NULL);
//    }
//    else
//    {
//        nvReadFile(hInputYUVFile, yuvInput[0], width * height, &numBytesRead, NULL);
//        nvReadFile(hInputYUVFile, yuvInput[1], width * height / 4, &numBytesRead, NULL);
//        nvReadFile(hInputYUVFile, yuvInput[2], width * height / 4, &numBytesRead, NULL);
//    }
//    return NV_ENC_SUCCESS;
//}
//
//void PrintHelp()
//{
//    printf("Usage : NvEncoder \n"
//        "-i <string>                  Specify input yuv420 file\n"
//        "-o <string>                  Specify output bitstream file\n"
//        "-size <int int>              Specify input resolution <width height>\n"
//        "\n### Optional parameters ###\n"
//        "-codec <integer>             Specify the codec \n"
//        "                                 0: H264\n"
//        "                                 1: HEVC\n"
//        "-preset <string>             Specify the preset for encoder settings\n"
//        "                                 hq : nvenc HQ \n"
//        "                                 hp : nvenc HP \n"
//        "                                 lowLatencyHP : nvenc low latency HP \n"
//        "                                 lowLatencyHQ : nvenc low latency HQ \n"
//        "                                 lossless : nvenc Lossless HP \n"
//        "-startf <integer>            Specify start index for encoding. Default is 0\n"
//        "-endf <integer>              Specify end index for encoding. Default is end of file\n"
//        "-fps <integer>               Specify encoding frame rate\n"
//        "-goplength <integer>         Specify gop length\n"
//        "-numB <integer>              Specify number of B frames\n"
//        "-bitrate <integer>           Specify the encoding average bitrate\n"
//        "-vbvMaxBitrate <integer>     Specify the vbv max bitrate\n"
//        "-vbvSize <integer>           Specify the encoding vbv/hrd buffer size\n"
//        "-rcmode <integer>            Specify the rate control mode\n"
//        "                                 0:  Constant QP\n"
//        "                                 1:  Single pass VBR\n"
//        "                                 2:  Single pass CBR\n"
//        "                                 4:  Single pass VBR minQP\n"
//        "                                 8:  Two pass frame quality\n"
//        "                                 16: Two pass frame size cap\n"
//        "                                 32: Two pass VBR\n"
//        "-qp <integer>                Specify qp for Constant QP mode\n"
//        "-i_qfactor <float>           Specify qscale difference between I-frames and P-frames\n"
//        "-b_qfactor <float>           Specify qscale difference between P-frames and B-frames\n" 
//        "-i_qoffset <float>           Specify qscale offset between I-frames and P-frames\n"
//        "-b_qoffset <float>           Specify qscale offset between P-frames and B-frames\n" 
//        "-picStruct <integer>         Specify the picture structure\n"
//        "                                 1:  Progressive frame\n"
//        "                                 2:  Field encoding top field first\n"
//        "                                 3:  Field encoding bottom field first\n"
//        "-devicetype <integer>        Specify devicetype used for encoding\n"
//        "                                 0:  DX9\n"
//        "                                 1:  DX11\n"
//        "                                 2:  Cuda\n"
//        "                                 3:  DX10\n"
//        "-yuv444 <integer>             Specify the input YUV format\n"
//        "                                 0: YUV 420\n"
//        "                                 1: YUV 444\n"
//        "-deviceID <integer>           Specify the GPU device on which encoding will take place\n"
//        "-meonly <integer>             Specify Motion estimation only(permissive value 1 and 2) to generates motion vectors and Mode information\n"
//        "                                 1: Motion estimation between startf and endf\n"
//        "                                 2: Motion estimation for all consecutive frames from startf to endf\n"
//        "-preloadedFrameCount <integer> Specify number of frame to load in memory(default value=240) with min value 2(1 frame for ref, 1 frame for input)\n"
//        "-help                         Prints Help Information\n\n"
//        );
//}
//
//int CNvEncoder::EncodeMain(int argc, char *argv[])
//{
//    HANDLE hInput;
//    DWORD fileSize;
//    uint32_t numBytesRead = 0;
//    uint8_t *yuv[3];
//    int lumaPlaneSize, chromaPlaneSize;
//    unsigned long long lStart, lEnd, lFreq;
//    int numFramesEncoded = 0;
//    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
//    bool bError = false;
//    EncodeConfig encodeConfig;
//    unsigned int preloadedFrameCount = FRAME_QUEUE;
//
//    memset(&encodeConfig, 0, sizeof(EncodeConfig));
//
//    encodeConfig.endFrameIdx = INT_MAX;
//    encodeConfig.bitrate = 5000000;
//    encodeConfig.rcMode = NV_ENC_PARAMS_RC_CONSTQP;
//    encodeConfig.gopLength = NVENC_INFINITE_GOPLENGTH;
//    encodeConfig.deviceType = NV_ENC_CUDA;
//    encodeConfig.codec = NV_ENC_H264;
//    encodeConfig.fps = 30;
//    encodeConfig.qp = 28;
//    encodeConfig.i_quant_factor = DEFAULT_I_QFACTOR;
//    encodeConfig.b_quant_factor = DEFAULT_B_QFACTOR;
//	encodeConfig.i_quant_offset = DEFAULT_I_QOFFSET;
//    encodeConfig.b_quant_offset = DEFAULT_B_QOFFSET; 
//    encodeConfig.presetGUID = NV_ENC_PRESET_DEFAULT_GUID;
//    encodeConfig.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
//    encodeConfig.isYuv444 = 0;
//
//    nvStatus = m_pNvHWEncoder->ParseArguments(&encodeConfig, argc, argv);
//    if (nvStatus != NV_ENC_SUCCESS)
//    {
//        PrintHelp();
//        return 1;
//    }
//
//    if (!encodeConfig.inputFileName || !encodeConfig.outputFileName || encodeConfig.width == 0 || encodeConfig.height == 0)
//    {
//        PrintHelp();
//        return 1;
//    }
//
//    encodeConfig.fOutput = fopen(encodeConfig.outputFileName, "wb");
//    if (encodeConfig.fOutput == NULL)
//    {
//        PRINTERR("Failed to create \"%s\"\n", encodeConfig.outputFileName);
//        return 1;
//    }
//
//    hInput = nvOpenFile(encodeConfig.inputFileName);
//    if (hInput == INVALID_HANDLE_VALUE)
//    {
//        PRINTERR("Failed to open \"%s\"\n", encodeConfig.inputFileName);
//        return 1;
//    }
//    
//    if ((encodeConfig.enableMEOnly == 1) || (encodeConfig.enableMEOnly == 2))
//    {
//        
//        if (encodeConfig.codec != NV_ENC_H264)
//        {
//            PRINTERR("\nvEncoder.exe Error: MEOnly mode is now only supported for H264. Check input params!\n");
//            return 1;
//        }
//        memcpy(&m_stEncoderInput, &encodeConfig, sizeof(encodeConfig));
//    }
//
//    switch (encodeConfig.deviceType)
//    {
//#if defined(NV_WINDOWS)
//    case NV_ENC_DX10:
//        InitD3D10(encodeConfig.deviceID);
//        break;
//
//    case NV_ENC_DX11:
//        InitD3D11(encodeConfig.deviceID);
//        break;
//#endif
//    case NV_ENC_CUDA:
//        InitCuda(encodeConfig.deviceID);
//        break;
//    }
//
//    if (encodeConfig.deviceType != NV_ENC_CUDA)
//        nvStatus = m_pNvHWEncoder->Initialize(m_pDevice, NV_ENC_DEVICE_TYPE_DIRECTX);
//    else
//        nvStatus = m_pNvHWEncoder->Initialize(m_pDevice, NV_ENC_DEVICE_TYPE_CUDA);
//
//    if (nvStatus != NV_ENC_SUCCESS)
//        return 1;
//
//    encodeConfig.presetGUID = m_pNvHWEncoder->GetPresetGUID(encodeConfig.encoderPreset, encodeConfig.codec);
//
//    printf("Encoding input           : \"%s\"\n", encodeConfig.inputFileName);
//    printf("         output          : \"%s\"\n", encodeConfig.outputFileName);
//    printf("         codec           : \"%s\"\n", encodeConfig.codec == NV_ENC_HEVC ? "HEVC" : "H264");
//    printf("         size            : %dx%d\n", encodeConfig.width, encodeConfig.height);
//    printf("         bitrate         : %d bits/sec\n", encodeConfig.bitrate);
//    printf("         vbvMaxBitrate   : %d bits/sec\n", encodeConfig.vbvMaxBitrate);
//    printf("         vbvSize         : %d bits\n", encodeConfig.vbvSize);
//    printf("         fps             : %d frames/sec\n", encodeConfig.fps);
//    printf("         rcMode          : %s\n", encodeConfig.rcMode == NV_ENC_PARAMS_RC_CONSTQP ? "CONSTQP" :
//                                              encodeConfig.rcMode == NV_ENC_PARAMS_RC_VBR ? "VBR" :
//                                              encodeConfig.rcMode == NV_ENC_PARAMS_RC_CBR ? "CBR" :
//                                              encodeConfig.rcMode == NV_ENC_PARAMS_RC_VBR_MINQP ? "VBR MINQP" :
//                                              encodeConfig.rcMode == NV_ENC_PARAMS_RC_2_PASS_QUALITY ? "TWO_PASS_QUALITY" :
//                                              encodeConfig.rcMode == NV_ENC_PARAMS_RC_2_PASS_FRAMESIZE_CAP ? "TWO_PASS_FRAMESIZE_CAP" :
//                                              encodeConfig.rcMode == NV_ENC_PARAMS_RC_2_PASS_VBR ? "TWO_PASS_VBR" : "UNKNOWN");
//    if (encodeConfig.gopLength == NVENC_INFINITE_GOPLENGTH)
//        printf("         goplength       : INFINITE GOP \n");
//    else
//        printf("         goplength       : %d \n", encodeConfig.gopLength);
//    printf("         B frames        : %d \n", encodeConfig.numB);
//    printf("         QP              : %d \n", encodeConfig.qp);
//    printf("       Input Format      : %s\n", encodeConfig.isYuv444 ? "YUV 444" : "YUV 420");
//    printf("         preset          : %s\n", (encodeConfig.presetGUID == NV_ENC_PRESET_LOW_LATENCY_HQ_GUID) ? "LOW_LATENCY_HQ" :
//                                        (encodeConfig.presetGUID == NV_ENC_PRESET_LOW_LATENCY_HP_GUID) ? "LOW_LATENCY_HP" :
//                                        (encodeConfig.presetGUID == NV_ENC_PRESET_HQ_GUID) ? "HQ_PRESET" :
//                                        (encodeConfig.presetGUID == NV_ENC_PRESET_HP_GUID) ? "HP_PRESET" :
//                                        (encodeConfig.presetGUID == NV_ENC_PRESET_LOSSLESS_HP_GUID) ? "LOSSLESS_HP" :
//                                        (encodeConfig.presetGUID == NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID) ? "LOW_LATENCY_DEFAULT" : "DEFAULT");
//    printf("  Picture Structure      : %s\n", (encodeConfig.pictureStruct == NV_ENC_PIC_STRUCT_FRAME) ? "Frame Mode" :
//                                              (encodeConfig.pictureStruct == NV_ENC_PIC_STRUCT_FIELD_TOP_BOTTOM) ? "Top Field first" :
//                                              (encodeConfig.pictureStruct == NV_ENC_PIC_STRUCT_FIELD_BOTTOM_TOP) ? "Bottom Field first" : "INVALID");
//    printf("         devicetype      : %s\n",   encodeConfig.deviceType == NV_ENC_DX9 ? "DX9" :
//                                                encodeConfig.deviceType == NV_ENC_DX10 ? "DX10" :
//                                                encodeConfig.deviceType == NV_ENC_DX11 ? "DX11" :
//                                                encodeConfig.deviceType == NV_ENC_CUDA ? "CUDA" : "INVALID");
//
//    printf("\n");
//
//    nvStatus = m_pNvHWEncoder->CreateEncoder(&encodeConfig);
//    if (nvStatus != NV_ENC_SUCCESS)
//        return 1;
//    encodeConfig.maxWidth = encodeConfig.maxWidth ? encodeConfig.maxWidth : encodeConfig.width;
//    encodeConfig.maxHeight = encodeConfig.maxHeight ? encodeConfig.maxHeight : encodeConfig.height;
//
//    if (encodeConfig.numB > 0)
//    {
//        m_uEncodeBufferCount = encodeConfig.numB + 4; // min buffers is numb + 1 + 3 pipelining
//    }
//    else
//    {
//        int numMBs = ((encodeConfig.maxHeight + 15) >> 4) * ((encodeConfig.maxWidth + 15) >> 4);
//        int NumIOBuffers;
//        if (numMBs >= 32768) //4kx2k
//            NumIOBuffers = MAX_ENCODE_QUEUE / 8;
//        else if (numMBs >= 16384) // 2kx2k
//            NumIOBuffers = MAX_ENCODE_QUEUE / 4;
//        else if (numMBs >= 8160) // 1920x1080
//            NumIOBuffers = MAX_ENCODE_QUEUE / 2;
//        else
//            NumIOBuffers = MAX_ENCODE_QUEUE;
//        m_uEncodeBufferCount = NumIOBuffers;
//    }
//    m_uPicStruct = encodeConfig.pictureStruct;
//    nvStatus = AllocateIOBuffers(encodeConfig.width, encodeConfig.height, encodeConfig.isYuv444);
//    if (nvStatus != NV_ENC_SUCCESS)
//        return 1;
//
//    if (encodeConfig.preloadedFrameCount >= 2)
//    {
//        preloadedFrameCount = encodeConfig.preloadedFrameCount;
//    }
//
//    uint32_t  chromaFormatIDC = (encodeConfig.isYuv444 ? 3 : 1);
//    lumaPlaneSize = encodeConfig.maxWidth * encodeConfig.maxHeight;
//    chromaPlaneSize = (chromaFormatIDC == 3) ? lumaPlaneSize : (lumaPlaneSize >> 2); 
//    nvGetFileSize(hInput, &fileSize);
//    int totalFrames = fileSize / (lumaPlaneSize + chromaPlaneSize + chromaPlaneSize);
//    if (encodeConfig.endFrameIdx < 0) {  
//        encodeConfig.endFrameIdx = totalFrames - 1;
//    }
//    else if (encodeConfig.endFrameIdx > totalFrames) {
//        PRINTERR("nvEncoder.exe Warning: -endf %d exceeds total video frame %d, using %d instead\n", encodeConfig.endFrameIdx, totalFrames, totalFrames);
//        encodeConfig.endFrameIdx = totalFrames - 1;
//    }
//
//    if ((encodeConfig.enableMEOnly == 1) || (encodeConfig.enableMEOnly == 2))
//    {        
//        MEOnlyConfig stMEOnly;
//        memset(&stMEOnly, 0, sizeof(stMEOnly));
//        stMEOnly.width = encodeConfig.width;
//        stMEOnly.height = encodeConfig.height;
//        stMEOnly.stride[0] = encodeConfig.width;
//        stMEOnly.stride[1] = (chromaFormatIDC == 3) ? encodeConfig.width : encodeConfig.width >> 1;
//        stMEOnly.stride[2] = (chromaFormatIDC == 3) ? encodeConfig.width : encodeConfig.width >> 1;
//
//        if (encodeConfig.enableMEOnly == 1)
//        {
//            stMEOnly.referenceFrameIndex = encodeConfig.startFrameIdx;
//            stMEOnly.inputFrameIndex = encodeConfig.endFrameIdx;
//            for (unsigned int i = 0; i < 2; i++)
//            {
//                stMEOnly.yuv[i][0] = new(std::nothrow)  unsigned char[lumaPlaneSize];
//                stMEOnly.yuv[i][1] = new(std::nothrow)  unsigned char[chromaPlaneSize];
//                stMEOnly.yuv[i][2] = new(std::nothrow)  unsigned char[chromaPlaneSize];
//                if (stMEOnly.yuv[i][0] == NULL || stMEOnly.yuv[i][1] == NULL || stMEOnly.yuv[i][2] == NULL)
//                {
//                    PRINTERR("\nvEncoder.exe Error: Failed to allocate memory for array yuvLoaded of Size = %u !\n", (lumaPlaneSize + 2 * chromaPlaneSize));
//                    return 1;
//                }
//            }
//            numBytesRead = 0;
//            loadframe(stMEOnly.yuv[0], hInput, encodeConfig.startFrameIdx, encodeConfig.width, encodeConfig.height, numBytesRead, encodeConfig.isYuv444);
//            loadframe(stMEOnly.yuv[1], hInput, encodeConfig.endFrameIdx, encodeConfig.width, encodeConfig.height, numBytesRead, encodeConfig.isYuv444);
//            RunMotionEstimationOnly(&stMEOnly, false);
//        }
//        else
//        {
//            unsigned char *yuvLoaded[3] = { NULL, NULL, NULL };
//            unsigned char *yuvScaled[3] = { NULL, NULL, NULL };
//            unsigned char *yuvInput[3] = { NULL, NULL, NULL };
//            unsigned int numFramesToEncode = MAX(1, (encodeConfig.endFrameIdx - encodeConfig.startFrameIdx));
//            yuvLoaded[0] = new(std::nothrow)  unsigned char[preloadedFrameCount * lumaPlaneSize];
//            yuvLoaded[1] = new(std::nothrow)  unsigned char[preloadedFrameCount * chromaPlaneSize];
//            yuvLoaded[2] = new(std::nothrow)  unsigned char[preloadedFrameCount * chromaPlaneSize];
//            
//            if (yuvLoaded[0] == NULL || yuvLoaded[1] == NULL || yuvLoaded[2] == NULL)
//            {
//                PRINTERR("\nvEncoder.exe Error: Failed to allocate memory for array yuvLoaded of Size = %u !\n", preloadedFrameCount*(lumaPlaneSize + 2 * chromaPlaneSize));
//                return 1;
//            }
//
//            for (unsigned int iNumFrms = encodeConfig.startFrameIdx; iNumFrms < (unsigned int)encodeConfig.endFrameIdx + 1; iNumFrms += (preloadedFrameCount-1))
//            {
//                if (iNumFrms != (MIN(iNumFrms + preloadedFrameCount - 1, (unsigned int)encodeConfig.endFrameIdx)))
//                    printf("\nLoading Frames [%d,%d] into system memory\n", iNumFrms, (MIN(iNumFrms + preloadedFrameCount-1, (unsigned int)encodeConfig.endFrameIdx)));
//
//                for (unsigned int frameCount = iNumFrms; frameCount < MIN(iNumFrms + preloadedFrameCount, (unsigned int)encodeConfig.endFrameIdx + 1); frameCount++)
//                {
//                    yuvInput[0] = &yuvLoaded[0][(frameCount % preloadedFrameCount)*lumaPlaneSize];
//                    yuvInput[1] = &yuvLoaded[1][(frameCount % preloadedFrameCount)*chromaPlaneSize];
//                    yuvInput[2] = &yuvLoaded[2][(frameCount % preloadedFrameCount)*chromaPlaneSize];
//                    loadframe(yuvInput, hInput, frameCount, encodeConfig.width, encodeConfig.height, numBytesRead, encodeConfig.isYuv444);
//                    yuvInput[0] = NULL;
//                    yuvInput[1] = NULL;
//                    yuvInput[2] = NULL;
//                }
//
//                for (unsigned int frameCount = iNumFrms; frameCount < MIN(iNumFrms + preloadedFrameCount -1, (unsigned int)encodeConfig.endFrameIdx); frameCount++)
//                {
//                    numBytesRead = 0;
//                    memset(&stMEOnly, 0, sizeof(stMEOnly));
//                    stMEOnly.width = encodeConfig.width;
//                    stMEOnly.height = encodeConfig.height;
//                    stMEOnly.stride[0] = encodeConfig.width;
//                    stMEOnly.stride[1] = (chromaFormatIDC == 3) ? encodeConfig.width : encodeConfig.width >> 1;
//                    stMEOnly.stride[2] = (chromaFormatIDC == 3) ? encodeConfig.width : encodeConfig.width >> 1;
//                    stMEOnly.inputFrameIndex = frameCount + 1;
//                    stMEOnly.referenceFrameIndex = frameCount;
//
//                    //ref
//                    stMEOnly.yuv[0][0] = &yuvLoaded[0][(frameCount % preloadedFrameCount)*lumaPlaneSize];
//                    stMEOnly.yuv[0][1] = &yuvLoaded[1][(frameCount % preloadedFrameCount)*chromaPlaneSize];
//                    stMEOnly.yuv[0][2] = &yuvLoaded[2][(frameCount % preloadedFrameCount)*chromaPlaneSize];
//                    //input
//                    stMEOnly.yuv[1][0] = &yuvLoaded[0][((frameCount + 1) % preloadedFrameCount)*lumaPlaneSize];
//                    stMEOnly.yuv[1][1] = &yuvLoaded[1][((frameCount + 1) % preloadedFrameCount)*chromaPlaneSize];
//                    stMEOnly.yuv[1][2] = &yuvLoaded[2][((frameCount + 1) % preloadedFrameCount)*chromaPlaneSize];
//                    RunMotionEstimationOnly(&stMEOnly, false);
//
//                    stMEOnly.yuv[0][0] = NULL;
//                    stMEOnly.yuv[0][1] = NULL;
//                    stMEOnly.yuv[0][2] = NULL;
//                    stMEOnly.yuv[1][0] = NULL;
//                    stMEOnly.yuv[1][1] = NULL;
//                    stMEOnly.yuv[1][2] = NULL;
//                }
//            }
//        }
//
//        if (encodeConfig.fOutput)
//        {
//            fclose(encodeConfig.fOutput);
//        }
//
//        if (hInput)
//        {
//            nvCloseFile(hInput);
//        }
//
//        Deinitialize(encodeConfig.deviceType);
//        for (unsigned int i = 0; i < 3; i++)
//        {
//            for (unsigned int j = 0; j < 2; j++)
//            {
//                 if (stMEOnly.yuv[j][i])
//                 {
//                    delete[] stMEOnly.yuv[j][i];
//                    stMEOnly.yuv[j][i] = NULL;
//                 }
//            }
//         }
//         printf("Done!! \n");
//         return bError ? 1 : 0;
//    }
//
//    
//    yuv[0] = new(std::nothrow) uint8_t[lumaPlaneSize];
//    yuv[1] = new(std::nothrow) uint8_t[chromaPlaneSize];
//    yuv[2] = new(std::nothrow) uint8_t[chromaPlaneSize];       
//    NvQueryPerformanceCounter(&lStart);
//
//    if (yuv[0] == NULL || yuv[1] == NULL || yuv[2] == NULL)
//    {
//       PRINTERR("\nvEncoder.exe Error: Failed to allocate memory for yuv array!\n");
//       return 1;
//    }
//
//    for (int frm = encodeConfig.startFrameIdx; frm <= encodeConfig.endFrameIdx; frm++)
//    {
//        numBytesRead = 0;
//        loadframe(yuv, hInput, frm, encodeConfig.width, encodeConfig.height, numBytesRead, encodeConfig.isYuv444);
//        if (numBytesRead == 0)
//            break;
//
//        EncodeFrameConfig stEncodeFrame;
//        memset(&stEncodeFrame, 0, sizeof(stEncodeFrame));
//        stEncodeFrame.yuv[0] = yuv[0];
//        stEncodeFrame.yuv[1] = yuv[1];
//        stEncodeFrame.yuv[2] = yuv[2];
//
//        stEncodeFrame.stride[0] = encodeConfig.width;
//        stEncodeFrame.stride[1] = (encodeConfig.isYuv444) ? encodeConfig.width : encodeConfig.width / 2;
//        stEncodeFrame.stride[2] = (encodeConfig.isYuv444) ? encodeConfig.width : encodeConfig.width / 2;
//        stEncodeFrame.width = encodeConfig.width;
//        stEncodeFrame.height = encodeConfig.height;
//
//        EncodeFrame(&stEncodeFrame, false, encodeConfig.width, encodeConfig.height);
//        numFramesEncoded++;
//
//        if (frm == encodeConfig.endFrameIdx / 2)
//        {
//            NV_ENC_CAPS_PARAM stCapsParam;
//            memset(&stCapsParam, 0, sizeof(NV_ENC_CAPS_PARAM));
//            SET_VER(stCapsParam, NV_ENC_CAPS_PARAM);
//            stCapsParam.capsToQuery = NV_ENC_CAPS_SUPPORT_DYN_BITRATE_CHANGE;
//            int supported = 0;
//
//            nvStatus = m_pNvHWEncoder->NvEncGetEncodeCaps(NV_ENC_CODEC_H264_GUID, &stCapsParam, &supported);
//
//            if (nvStatus != NV_ENC_SUCCESS)
//            {
//                printf("NvEncGetEncodeCaps failed. Error ID = %d\n", nvStatus);
//            }
//            else
//            {
//                if (supported == 1)
//                {
//                    printf("NV_ENC_CAPS_SUPPORT_DYN_BITRATE_CHANGE supported\n");
//                }
//                else
//                {
//                    PRINTERR("NV_ENC_CAPS_SUPPORT_DYN_BITRATE_CHANGE not supported\n");
//                    return NV_ENC_ERR_UNSUPPORTED_DEVICE;
//                }
//            }
//        }
//
//        if (frm == encodeConfig.endFrameIdx / 2)
//        {
//            NvEncPictureCommand encPicCommand;
//
//            encPicCommand.bBitrateChangePending = true;
//            encPicCommand.newVBVSize = 0;
//            encPicCommand.newBitrate = 250000;
//
//            //encPicCommand.bForceIDR = false;
//
//            //encPicCommand.bForceIntraRefresh = false;
//            //encPicCommand.intraRefreshDuration = 0;
//
//            //encPicCommand.bInvalidateRefFrames = false;
//            //encPicCommand.numRefFramesToInvalidate = 0;
//            //encPicCommand.refFrameNumbers[0] = 0;    
//
//            encPicCommand.bResolutionChangePending = false;
//            //encPicCommand.newHeight = 640;
//            //encPicCommand.newWidth = 480;
//
//            NVENCSTATUS status = m_pNvHWEncoder->NvEncReconfigureEncoder(&encPicCommand);
//            if (status == NV_ENC_SUCCESS)
//            {
//                printf("bitrate changed!\n");
//            }
//            else
//            {
//                // Getting NV_ENC_ERR_INVALID_PARAM (== 8)
//                printf("Bitrate changing failed! Error is %d\n", status);
//            }
//        }
//    }
//
//    nvStatus = EncodeFrame(NULL, true, encodeConfig.width, encodeConfig.height);
//    if (nvStatus != NV_ENC_SUCCESS)
//    {
//        bError = true;
//        goto exit;
//    }
//
//    if (numFramesEncoded > 0)
//    {
//        NvQueryPerformanceCounter(&lEnd);
//        NvQueryPerformanceFrequency(&lFreq);
//        double elapsedTime = (double)(lEnd - lStart);
//        printf("Encoded %d frames in %6.2fms\n", numFramesEncoded, (elapsedTime*1000.0)/lFreq);
//        printf("Avergage Encode Time : %6.2fms\n", ((elapsedTime*1000.0)/numFramesEncoded)/lFreq);
//    }
//
//exit:
//    if (encodeConfig.fOutput)
//    {
//        fclose(encodeConfig.fOutput);
//    }
//
//    if (hInput)
//    {
//        nvCloseFile(hInput);
//    }
//
//    Deinitialize(encodeConfig.deviceType);
//
//    for (int i = 0; i < 3; i ++)
//    {
//        if (yuv[i])
//        {
//            delete [] yuv[i];
//        }
//    }
//
//    return bError ? 1 : 0;
//}
//
//NVENCSTATUS CNvEncoder::RunMotionEstimationOnly(MEOnlyConfig *pMEOnly, bool bFlush)
//{
//    uint8_t *pInputSurface = NULL;
//    uint8_t *pInputSurfaceCh = NULL;
//    uint32_t lockedPitch = 0;
//    uint32_t dwSurfHeight = 0;
//    static unsigned int dwCurWidth = 0;
//    static unsigned int dwCurHeight = 0;
//    HRESULT hr = S_OK;
//    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
//    NV_ENC_MAP_INPUT_RESOURCE mapRes = { 0 };
//    EncodeBuffer *pEncodeBuffer[2] = {NULL};
//    static bool flipBuffer = false;
//
//    if (bFlush)
//    {
//        FlushEncoder();
//        return NV_ENC_SUCCESS;
//    }
//
//    if (!pMEOnly)
//    {
//        assert(0);
//        return NV_ENC_ERR_INVALID_PARAM;
//    }
//
//    for (int i = 0; i < 2;i++)
//    {
//        if (flipBuffer == false)
//        {
//            pEncodeBuffer[0] = m_EncodeBufferQueue.GetAvailable();
//            pEncodeBuffer[1] = m_EncodeBufferQueue.GetAvailable();
//        }
//        else
//        {
//            pEncodeBuffer[0] = m_EncodeBufferQueue.GetPending();
//            pEncodeBuffer[1] = m_EncodeBufferQueue.GetPending();
//        }
//        if (!pEncodeBuffer[0] || !pEncodeBuffer[1])
//        {
//            flipBuffer = (flipBuffer^1);
//        }
//        else
//        {
//            break;
//        }
//    }
//    if (!pEncodeBuffer[0] || !pEncodeBuffer[1])
//    {
//        assert(0);
//        return  NV_ENC_ERR_INVALID_PARAM;
//    }
//    dwCurWidth = pMEOnly->width;
//    dwCurHeight = pMEOnly->height;
//
//    for (int i = 0; i < 2; i++)
//    {
//        unsigned char *pInputSurface = NULL;
//        unsigned char *pInputSurfaceCh = NULL;
//        nvStatus = m_pNvHWEncoder->NvEncLockInputBuffer(pEncodeBuffer[i]->stInputBfr.hInputSurface, (void**)&pInputSurface, &lockedPitch);
//        if (nvStatus != NV_ENC_SUCCESS)
//            return nvStatus;
//
//        if (pEncodeBuffer[i]->stInputBfr.bufferFmt == NV_ENC_BUFFER_FORMAT_NV12_PL)
//        {
//            pInputSurfaceCh = pInputSurface + (pEncodeBuffer[i]->stInputBfr.dwHeight*lockedPitch);
//            convertYUVpitchtoNV12(pMEOnly->yuv[i][0], pMEOnly->yuv[i][1], pMEOnly->yuv[i][2], pInputSurface, pInputSurfaceCh, dwCurWidth, dwCurHeight, dwCurWidth, lockedPitch);
//        }
//        else
//        {
//            unsigned char *pInputSurfaceCb = pInputSurface + (pEncodeBuffer[i]->stInputBfr.dwHeight * lockedPitch);
//            unsigned char *pInputSurfaceCr = pInputSurfaceCb + (pEncodeBuffer[i]->stInputBfr.dwHeight * lockedPitch);
//            convertYUVpitchtoYUV444(pMEOnly->yuv[i][0], pMEOnly->yuv[i][1], pMEOnly->yuv[i][2], pInputSurface, pInputSurfaceCb, pInputSurfaceCr, dwCurWidth, dwCurHeight, dwCurWidth, lockedPitch);
//        }
//        nvStatus = m_pNvHWEncoder->NvEncUnlockInputBuffer(pEncodeBuffer[i]->stInputBfr.hInputSurface);
//        if (nvStatus != NV_ENC_SUCCESS)
//            return nvStatus;
//    }
//
//    nvStatus = m_pNvHWEncoder->NvRunMotionEstimationOnly(pEncodeBuffer, pMEOnly);
//    if (nvStatus != NV_ENC_SUCCESS)
//    {
//        PRINTERR("nvEncRunMotionEstimationOnly error:0x%x\n", nvStatus);
//        assert(0);
//    }
//    return nvStatus;
//
//}
//
//NVENCSTATUS CNvEncoder::EncodeFrame(EncodeFrameConfig *pEncodeFrame, bool bFlush, uint32_t width, uint32_t height)
//{
//    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
//    uint32_t lockedPitch = 0;
//    EncodeBuffer *pEncodeBuffer = NULL;
//
//    if (bFlush)
//    {
//        FlushEncoder();
//        return NV_ENC_SUCCESS;
//    }
//
//    if (!pEncodeFrame)
//    {
//        return NV_ENC_ERR_INVALID_PARAM;
//    }
//
//    pEncodeBuffer = m_EncodeBufferQueue.GetAvailable();
//    if(!pEncodeBuffer)
//    {
//        m_pNvHWEncoder->ProcessOutput(m_EncodeBufferQueue.GetPending());
//        pEncodeBuffer = m_EncodeBufferQueue.GetAvailable();
//    }
//
//    unsigned char *pInputSurface;
//    
//    nvStatus = m_pNvHWEncoder->NvEncLockInputBuffer(pEncodeBuffer->stInputBfr.hInputSurface, (void**)&pInputSurface, &lockedPitch);
//    if (nvStatus != NV_ENC_SUCCESS)
//        return nvStatus;
//
//    if (pEncodeBuffer->stInputBfr.bufferFmt == NV_ENC_BUFFER_FORMAT_NV12_PL)
//    {
//        unsigned char *pInputSurfaceCh = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight*lockedPitch);
//        convertYUVpitchtoNV12(pEncodeFrame->yuv[0], pEncodeFrame->yuv[1], pEncodeFrame->yuv[2], pInputSurface, pInputSurfaceCh, width, height, width, lockedPitch);
//    }
//    else
//    {
//        unsigned char *pInputSurfaceCb = pInputSurface + (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
//        unsigned char *pInputSurfaceCr = pInputSurfaceCb + (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
//        convertYUVpitchtoYUV444(pEncodeFrame->yuv[0], pEncodeFrame->yuv[1], pEncodeFrame->yuv[2], pInputSurface, pInputSurfaceCb, pInputSurfaceCr, width, height, width, lockedPitch);
//    }
//    nvStatus = m_pNvHWEncoder->NvEncUnlockInputBuffer(pEncodeBuffer->stInputBfr.hInputSurface);
//    if (nvStatus != NV_ENC_SUCCESS)
//        return nvStatus;
//
//    nvStatus = m_pNvHWEncoder->NvEncEncodeFrame(pEncodeBuffer, NULL, width, height, (NV_ENC_PIC_STRUCT)m_uPicStruct);
//    return nvStatus;
//}
//
//int main(int argc, char **argv)
//{
//    CNvEncoder nvEncoder;
//    return nvEncoder.EncodeMain(argc, argv);
//}
