/*!
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <stdio.h>
#include <stdlib.h>
#include <d3d9.h>
#include <windows.h>
#include <cuda.h>
#include "helper_cuda_drvapi.h"

#include "Encoder.h"

Encoder::Encoder()
{
    // Init members
    m_hinstLib = NULL;
    m_hEncoder = NULL;
    m_pEncodeAPI = NULL;
    m_cuContext = NULL;
    m_dwEncodeGUIDCount = 0;
    m_stEncodeGUID = NV_ENC_CODEC_H264_GUID;
    m_dwCodecProfileGUIDCount = 0;
    m_stCodecProfileGUID = NV_ENC_CODEC_PROFILE_AUTOSELECT_GUID;
    m_dwPresetGUIDCount = 0;
    m_stPresetGUID = NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID;
    m_dwInputFmtCount = 0;
    m_pAvailableSurfaceFmts = NULL;
    m_dwInputFormat = NV_ENC_BUFFER_FORMAT_NV12_PL;
    memset(&m_stInitEncParams, 0, sizeof(m_stInitEncParams));
    memset(&m_stEncodeConfig, 0, sizeof(m_stEncodeConfig));
    memset(&m_stPresetConfig, 0, sizeof(m_stPresetConfig));
    memset(&m_stEncodePicParams, 0, sizeof(m_stEncodePicParams));
    memset(&m_stReconfigureParam, 0, sizeof(m_stReconfigureParam));
    m_dwWidth = 0;
    m_dwHeight = 0;
    m_dwBufferWidth = 0;
    m_dwFrameCount = 0;
    m_bUseMappedResource = false;
    m_dwMaxBufferSize = 0;
    for (unsigned int i = 0; i < MAX_BUF_QUEUE; i++)
    {
        memset(&m_stInputSurface[i], 0, sizeof(EncodeInputSurfaceInfo));
        memset(&m_stBitstreamBuffer[i], 0, sizeof(EncodeOutputBuffer));
    }
}

Encoder::~Encoder()
{
    // Release EncodeAPI DLL
    if (m_pEncodeAPI)
    {
        delete m_pEncodeAPI;
        m_pEncodeAPI = NULL;
    }
    if (m_hinstLib)
    {
        FreeLibrary (m_hinstLib);
        m_hinstLib = NULL;
    }
    m_cuContext = NULL;
    m_hEncoder = NULL;
    m_dwEncodeGUIDCount = 0;
    m_dwWidth = 0;
    m_dwHeight = 0;
    m_dwBufferWidth = 0;
    m_dwPresetGUIDCount = 0;
    m_dwEncodeGUIDCount = 0;
    m_dwInputFmtCount = 0;
    m_bUseMappedResource = false;
    m_dwMaxBufferSize = 0;
}

HRESULT Encoder::Init(CUcontext cuCtx, unsigned int dwMaxBufferSize)
{
    HRESULT hr = S_OK;
    m_cuContext = cuCtx;
    m_dwMaxBufferSize = dwMaxBufferSize;
    // Load EncodeAPI DLL, based on bit-ness
    hr = LoadEncAPI();
    if (SUCCEEDED(hr))
    {
        // Open Encode Session
        NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS param = {0};
        param.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
        param.apiVersion = NVENCAPI_VERSION;
        param.device = cuCtx;
        param.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
        m_pEncodeAPI->nvEncOpenEncodeSessionEx(&param, &m_hEncoder);
    }
    return hr;
}

HRESULT Encoder::SetupEncoder(unsigned int dwWidth, unsigned int dwHeight, unsigned int dwBufferWidth, unsigned int dwBitRate)
{
    if (!m_hEncoder)
        return E_FAIL;
    HRESULT hr = S_OK;
    NVENCSTATUS status = NV_ENC_SUCCESS;
    m_dwWidth = dwWidth;
    m_dwHeight = dwHeight;
    m_dwBufferWidth = dwBufferWidth;
    // Validate basic configuration
    if (FAILED(hr = ValidateParams()))
    {
        fprintf(stderr, __FUNCTION__": Failed to find a valid encoder configuration.\n");
        return hr;
    }

    m_stInitEncParams.encodeGUID  = m_stEncodeGUID;
    m_stInitEncParams.presetGUID  = m_stPresetGUID;
    m_stInitEncParams.version = NV_ENC_INITIALIZE_PARAMS_VER;
    m_stInitEncParams.encodeWidth = dwWidth;
    m_stInitEncParams.encodeHeight= dwHeight;
    m_stInitEncParams.frameRateDen= 1;
    m_stInitEncParams.frameRateNum= 30;
    m_stInitEncParams.enablePTD   = 1;

    // Fetch encoder preset configuration
    memset(&m_stPresetConfig, 0, sizeof(m_stPresetConfig));
    m_stPresetConfig.version = NV_ENC_PRESET_CONFIG_VER;
    m_stPresetConfig.presetCfg.version = NV_ENC_CONFIG_VER;
    
    status = m_pEncodeAPI->nvEncGetEncodePresetConfig(m_hEncoder, m_stEncodeGUID, m_stPresetGUID, &m_stPresetConfig);
    if (status != NV_ENC_SUCCESS)
    {
        fprintf(stderr, __FUNCTION__": Failed to fetch preset encoder config.\n");
        return E_FAIL;
    }

    // Set up encoder configuration, local state vars
    memcpy(&m_stEncodeConfig, &m_stPresetConfig.presetCfg, sizeof(NV_ENC_CONFIG));
    m_stEncodeConfig.version = NV_ENC_CONFIG_VER;
    m_stEncodeConfig.rcParams.averageBitRate = dwBitRate;
    m_stEncodeConfig.rcParams.maxBitRate = 6*(dwBitRate/5); // 1.2 times
    m_stEncodeConfig.rcParams.vbvBufferSize = dwBitRate/(m_stInitEncParams.frameRateNum/m_stInitEncParams.frameRateDen);
    m_stEncodeConfig.rcParams.vbvInitialDelay= m_stEncodeConfig.rcParams.vbvInitialDelay;

    m_stInitEncParams.encodeConfig= &m_stEncodeConfig;

    // Initialize encoder
    status = m_pEncodeAPI->nvEncInitializeEncoder(m_hEncoder, &m_stInitEncParams);
    if (status != NV_ENC_SUCCESS)
    {
        fprintf(stderr, __FUNCTION__": Failed to initialize encoder.\n");
        return E_FAIL;
    }

    // Allocate IO Buffers
    if (FAILED(hr = AllocateIOBufs()))
    {
        fprintf(stderr, __FUNCTION__": Failed to allocate IO buffers.\n");
        return hr;
    }
    return S_OK;
}

HRESULT Encoder::Reconfigure(unsigned int dwWidth, unsigned int dwHeight, unsigned int dwBufferWidth, unsigned int dwBitRate)
{
    if (!m_hEncoder)
        return E_FAIL;

    HRESULT hr = S_OK;
    NVENCSTATUS status = NV_ENC_SUCCESS;
    hr = ReleaseIOBufs();
    if (hr != S_OK)
    {
        fprintf(stderr, __FUNCTION__": Failed to release old buffers.\n");
        return hr;
    }
    m_dwWidth = dwWidth;
    m_dwHeight= dwHeight;
    m_dwBufferWidth = dwBufferWidth;
    m_stInitEncParams.encodeWidth = m_dwWidth;
    m_stInitEncParams.encodeHeight= m_dwHeight;
    m_stInitEncParams.encodeConfig->rcParams.averageBitRate = dwBitRate;

    memset(&m_stReconfigureParam, 0, sizeof(m_stReconfigureParam));
    m_stReconfigureParam.version = NV_ENC_RECONFIGURE_PARAMS_VER;
    m_stReconfigureParam.resetEncoder = 1;
    m_stReconfigureParam.forceIDR     = 1;
    m_stReconfigureParam.reInitEncodeParams = m_stInitEncParams;
    status = m_pEncodeAPI->nvEncReconfigureEncoder(m_hEncoder, &m_stReconfigureParam);
    if (status != NV_ENC_SUCCESS)
    {
        fprintf(stderr, __FUNCTION__": Failed to reconfigure the encoder.\n");
        return E_FAIL;
    }
    hr = AllocateIOBufs();
    if (hr != S_OK)
    {
        fprintf(stderr, __FUNCTION__": Failed to allocate new buffers.\n");
        return hr;
    }
    return S_OK;
}

HRESULT Encoder::LaunchEncode(unsigned int i, CUdeviceptr devptr)
{
    if (!m_hEncoder)
        return E_FAIL;
    HRESULT hr = S_OK;
    NVENCSTATUS status = NV_ENC_SUCCESS;

    if (m_stInputSurface[i].hInputSurface == NULL)
    {
        if (m_stInputSurface[i].hRegisteredHandle == NULL)
        {
            NV_ENC_REGISTER_RESOURCE param = {0};
            // Register resource if not already registered
            param.version = NV_ENC_REGISTER_RESOURCE_VER;
            param.resourceToRegister = (void *)devptr;
            param.resourceType       = NV_ENC_INPUT_RESOURCE_TYPE_CUDADEVICEPTR;
            param.width              = m_dwWidth;
            param.height             = m_dwHeight;
            param.pitch              = m_dwBufferWidth;
            param.bufferFormat       = NV_ENC_BUFFER_FORMAT_NV12_PL;
            status = m_pEncodeAPI->nvEncRegisterResource(m_hEncoder,  &param);
            if (status != NV_ENC_SUCCESS)
            {
                fprintf(stderr, __FUNCTION__": Failed to register resource with encoder.\n");
                return E_FAIL;
            }
            m_stInputSurface[i].hRegisteredHandle = param.registeredResource;
        }

        NV_ENC_MAP_INPUT_RESOURCE mapParam = {0};
        mapParam.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
        mapParam.registeredResource = m_stInputSurface[i].hRegisteredHandle;
        status = m_pEncodeAPI->nvEncMapInputResource(m_hEncoder, &mapParam);
        if (status != NV_ENC_SUCCESS)
        {
            fprintf(stderr, __FUNCTION__": Failed to map resource with encoder.\n");
            return E_FAIL;
        }

        m_stInputSurface[i].hInputSurface   = mapParam.mappedResource;
        m_stInputSurface[i].bufferFmt       = mapParam.mappedBufferFmt;
        m_bUseMappedResource = true;
    }

    // Check states and launch encode
    memset(&m_stEncodePicParams, 0, sizeof(m_stEncodePicParams));
    m_stEncodePicParams.version         = NV_ENC_PIC_PARAMS_VER;
    m_stEncodePicParams.inputBuffer     = m_stInputSurface[i].hInputSurface;
    m_stEncodePicParams.bufferFmt       = m_stInputSurface[i].bufferFmt;
    m_stEncodePicParams.inputWidth      = m_stInputSurface[i].dwWidth;
    m_stEncodePicParams.inputHeight     = m_stInputSurface[i].dwHeight;
    m_stEncodePicParams.outputBitstream = m_stBitstreamBuffer[i].hBitstreamBuffer;
    m_stEncodePicParams.completionEvent = NULL;
    m_stEncodePicParams.pictureStruct   = NV_ENC_PIC_STRUCT_FRAME;
    m_stEncodePicParams.encodePicFlags  = 0;
    m_stEncodePicParams.inputTimeStamp  = 0;
    m_stEncodePicParams.inputDuration   = 0;
//    memcpy(&m_stEncodePicParams.rcParams, &m_stEncodeConfig.rcParams, sizeof(m_stEncodePicParams.rcParams));
    status = m_pEncodeAPI->nvEncEncodePicture(m_hEncoder, &m_stEncodePicParams);
    if (status != NV_ENC_SUCCESS)
    {
        fprintf(stderr, __FUNCTION__": Failed to encode input");
    }

    return S_OK;
}

HRESULT Encoder::GetBitstream(unsigned int i, FILE *fOut)
{
    if (!m_hEncoder)
        return E_FAIL;
    HRESULT hr = S_OK;
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;
    
    if (m_bUseMappedResource)
    {
        // Unmap input resource
        nvStatus = m_pEncodeAPI->nvEncUnmapInputResource(m_hEncoder, m_stInputSurface[i].hInputSurface);
        m_stInputSurface[i].hInputSurface = NULL;
    }
    if (m_dwFrameCount == 0)
    {
        m_dwFrameCount++;
        unsigned char buf[1024] = {'\0'};
        unsigned int size = 0;
        NV_ENC_SEQUENCE_PARAM_PAYLOAD param = {0};
        param.version = NV_ENC_SEQUENCE_PARAM_PAYLOAD_VER;
        param.inBufferSize = 1024;
        param.spsppsBuffer = (void *)buf;
        param.outSPSPPSPayloadSize = &size;
        m_pEncodeAPI->nvEncGetSequenceParams(m_hEncoder, &param);
        if (fOut)
        {
            fwrite(buf, sizeof(unsigned char), size, fOut);
        }
    }
    // Fetch encoded bitstream
    NV_ENC_LOCK_BITSTREAM lockBitstreamData = {0};
    lockBitstreamData.version = NV_ENC_LOCK_BITSTREAM_VER;
    lockBitstreamData.outputBitstream = m_stBitstreamBuffer[i].hBitstreamBuffer;
    lockBitstreamData.doNotWait = false;

    nvStatus = m_pEncodeAPI->nvEncLockBitstream(m_hEncoder, &lockBitstreamData);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        fprintf(stderr, __FUNCTION__": Failed to unlock bitstream.\n");
    }

    if (fOut)
        fwrite(lockBitstreamData.bitstreamBufferPtr, 1, lockBitstreamData.bitstreamSizeInBytes, fOut);

    nvStatus = m_pEncodeAPI->nvEncUnlockBitstream(m_hEncoder, m_stBitstreamBuffer[i].hBitstreamBuffer);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        fprintf(stderr, __FUNCTION__": Failed to unlock bitstream.\n");
    }

    return S_OK;
}

HRESULT Encoder::AllocateIOBufs()
{
    NVENCSTATUS status = NV_ENC_SUCCESS;
    if (!m_hEncoder)
        return E_FAIL;
    // Allocate and\or Register IO Buffers
    checkCudaErrors(cuMemAlloc(&m_cuDevptr, m_dwMaxBufferSize));
    for (unsigned int i = 0; i < MAX_BUF_QUEUE; i++)
    {
        m_stInputSurface[i].dwWidth = m_dwWidth;
        m_stInputSurface[i].dwHeight = m_dwHeight;
        m_stInputSurface[i].dwCuPitch= m_dwBufferWidth;

        m_stBitstreamBuffer[i].dwSize = 1024*1024;
        NV_ENC_CREATE_BITSTREAM_BUFFER stAllocBitstream;
        memset(&stAllocBitstream, 0, sizeof(stAllocBitstream));
        stAllocBitstream.version                   = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
        stAllocBitstream.size                      = 2*1024*1024;//m_stBitstreamBuffer[i].dwSize;
        stAllocBitstream.memoryHeap                = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;

        status = m_pEncodeAPI->nvEncCreateBitstreamBuffer(m_hEncoder, &stAllocBitstream);
        
        if (status != NV_ENC_SUCCESS)
        {
            fprintf(stderr, __FUNCTION__": Failed to allocate output buffers.\n");
            return E_OUTOFMEMORY;
        }
        m_stBitstreamBuffer[i].hBitstreamBuffer    = stAllocBitstream.bitstreamBuffer;
        m_stBitstreamBuffer[i].pBitstreamBufferPtr = stAllocBitstream.bitstreamBufferPtr;
    }
    return S_OK;
}

HRESULT Encoder::Flush()
{
    // Notify EOS
    return E_FAIL;
}

HRESULT Encoder::TearDown()
{
    NVENCSTATUS status = NV_ENC_SUCCESS;
    if (!m_hEncoder)
        return E_FAIL;
    // Session teardown
    // Release IO Buffers
    ReleaseIOBufs();
    // Destroy encode session
    status = m_pEncodeAPI->nvEncDestroyEncoder(m_hEncoder);
    // Reset local states
    return E_FAIL;
}

HRESULT Encoder::ReleaseIOBufs()
{
    NVENCSTATUS status = NV_ENC_SUCCESS;
    if (!m_hEncoder)
        return E_FAIL;
    // Destroy allocated buffers, unregister registered buffers
    for (unsigned int i = 0; i < MAX_BUF_QUEUE; i++)
    {
        if (m_stInputSurface[i].hRegisteredHandle)
            status = m_pEncodeAPI->nvEncUnregisterResource(m_hEncoder, m_stInputSurface[i].hRegisteredHandle);
        if (m_stBitstreamBuffer[i].hBitstreamBuffer)
            status = m_pEncodeAPI->nvEncDestroyBitstreamBuffer(m_hEncoder, m_stBitstreamBuffer[i].hBitstreamBuffer);
        memset(&m_stInputSurface[i], 0, sizeof(EncodeInputSurfaceInfo));
        memset(&m_stBitstreamBuffer[i], 0, sizeof(EncodeOutputBuffer));
    }

    return S_OK;
}

HRESULT Encoder::LoadEncAPI()
{
    // Load NvEncodeAPI DLL based on bit-ness

    NVENCSTATUS nvStatus;
    MYPROC nvEncodeAPICreateInstance; // function pointer to create instance in nvEncodeAPI

    if (Is64Bit())
    {
        m_hinstLib = LoadLibrary(TEXT(__NVEncodeLibName64));
    }
    else
    {
        m_hinstLib = LoadLibrary(TEXT(__NVEncodeLibName32));
    }

    if (m_hinstLib != NULL)
    {
        // Initialize NVENC API
        nvEncodeAPICreateInstance = (MYPROC) GetProcAddress(m_hinstLib, "NvEncodeAPICreateInstance");

        if (NULL != nvEncodeAPICreateInstance)
        {
            m_pEncodeAPI = new NV_ENCODE_API_FUNCTION_LIST;

            if (m_pEncodeAPI)
            {
                memset(m_pEncodeAPI, 0, sizeof(NV_ENCODE_API_FUNCTION_LIST));
                m_pEncodeAPI->version = NV_ENCODE_API_FUNCTION_LIST_VER;
                nvStatus = nvEncodeAPICreateInstance(m_pEncodeAPI);
                return S_OK;
                //bEncodeAPIFound = true;
            }
            else
            {
                //bEncodeAPIFound = false;
                fprintf(stderr, __FUNCTION__": Error  - out of memory");
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            fprintf(stderr, __FUNCTION__": failed to find NvEncodeAPICreateInstance");
        }
    }
    else
    {
        //bEncodeAPIFound = false;
        if (Is64Bit())
        {
            fprintf(stderr, __FUNCTION__": failed to load %s!", __NVEncodeLibName64);
        }
        else
        {
            fprintf(stderr, __FUNCTION__": failed to load %s!", __NVEncodeLibName32);
        }
    }
    return E_FAIL;
}

HRESULT Encoder::ValidateParams()
{
    HRESULT hr = E_FAIL;
    unsigned int tempCount = 0;
    NVENCSTATUS status = NV_ENC_SUCCESS;
    GUID *pEncGUIDs = NULL;
    GUID *pPresetGUIDs = NULL;
    bool bFound = false;

    if (!m_hEncoder)
        return E_FAIL;
    // Validate encoder initialization params based on capabilities exposed by the API
    // Codec GUID
    status = m_pEncodeAPI->nvEncGetEncodeGUIDCount(m_hEncoder, &m_dwEncodeGUIDCount);
    if (status != NV_ENC_SUCCESS)
    {
        fprintf(stderr, __FUNCTION__": Could not fetch encoder guid count.\n");
        return E_FAIL;
    }
    
    pEncGUIDs = (GUID *)malloc(m_dwEncodeGUIDCount*sizeof(GUID));
    status = m_pEncodeAPI->nvEncGetEncodeGUIDs(m_hEncoder, pEncGUIDs, m_dwEncodeGUIDCount, &tempCount);
    if (status != NV_ENC_SUCCESS)
    {
        fprintf(stderr, __FUNCTION__": Could not fetch encoder guid list.\n");
        return E_FAIL;
    }

    for (unsigned int i = 0, bFound = false; i < m_dwEncodeGUIDCount; i++)
    {
        if (pEncGUIDs[i] == m_stEncodeGUID)
        {
            // Preset GUID
            status = m_pEncodeAPI->nvEncGetEncodePresetCount(m_hEncoder, m_stEncodeGUID, &m_dwPresetGUIDCount);
            if (status != NV_ENC_SUCCESS)
            {
                fprintf(stderr, __FUNCTION__": Could not fetch preset guid count.\n");
                break;
            }
            
            pPresetGUIDs = (GUID *)malloc(m_dwPresetGUIDCount*sizeof(GUID));
            status = m_pEncodeAPI->nvEncGetEncodePresetGUIDs(m_hEncoder, m_stEncodeGUID, pPresetGUIDs, m_dwPresetGUIDCount, &tempCount);
            if (status != NV_ENC_SUCCESS)
            {
                fprintf(stderr, __FUNCTION__": Could not fetch preset guid list.\n");
                break;
            }

            for (unsigned int i = 0, bFound = false; i < m_dwPresetGUIDCount; i++)
            {
                if (pPresetGUIDs[i] == m_stPresetGUID)
                {
                    bFound = true;
                    return S_OK;
                }
            }
            break;
        }
    }

    //if (pEncGUIDs)
    //    free(pEncGUIDs);
    //if (pPresetGUIDs)
    //    free(pPresetGUIDs);

    return E_FAIL;
}