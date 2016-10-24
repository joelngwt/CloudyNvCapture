/*!
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <initguid.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "Encoder.h"

#define SAFE_RELEASE(x) if(x){x->Release(); x=NULL;}

Encoder::Encoder()
    : m_hinstLib(NULL), m_hEncoder(NULL), m_pEncodeAPI(NULL), m_pAvailableSurfaceFmts(NULL)
    , m_bUseMappedResource(false)
    , m_dwWidth(0), m_dwHeight(0), m_dwFrameCount(0), m_dwMaxBufferSize(0)
    , m_dwEncodeGUIDCount(0)
    , m_stEncodeGUID(NV_ENC_CODEC_H264_GUID)
    , m_dwCodecProfileGUIDCount(0)
    , m_stCodecProfileGUID(NV_ENC_CODEC_PROFILE_AUTOSELECT_GUID)
    , m_dwPresetGUIDCount(0)
    , m_stPresetGUID(NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID)
    , m_dwInputFmtCount(0)
    , m_dwInputFormat(NV_ENC_BUFFER_FORMAT_ARGB)
    , m_bIsHEVC(false), m_bIsYUV444(false)
{
    // Init members

    memset(&m_stInitEncParams,    0, sizeof(m_stInitEncParams));
    memset(&m_stEncodeConfig,     0, sizeof(m_stEncodeConfig));
    memset(&m_stPresetConfig,     0, sizeof(m_stPresetConfig));
    memset(&m_stEncodePicParams,  0, sizeof(m_stEncodePicParams));
    memset(&m_stReconfigureParam, 0, sizeof(m_stReconfigureParam));
    for (unsigned int i = 0; i < MAX_BUF_QUEUE; i++)
    {
        memset(&m_stInputSurface[i], 0, sizeof(EncodeInputSurfaceInfo));
        memset(&m_stBitstreamBuffer[i], 0, sizeof(EncodeOutputBuffer));
    }
}

Encoder::~Encoder()
{
    TearDown();
    // Release EncodeAPI DLL
    if (m_pEncodeAPI)
    {
        delete m_pEncodeAPI;
        m_pEncodeAPI = NULL;
    }
    if (m_hinstLib)
    {
        FreeLibrary(m_hinstLib);
        m_hinstLib = NULL;
    }
    SAFE_RELEASE(m_pD3D9Dev);
    m_hEncoder = NULL;
    m_dwEncodeGUIDCount = 0;
    m_dwWidth = 0;
    m_dwHeight = 0;
    m_dwPresetGUIDCount = 0;
    m_dwEncodeGUIDCount = 0;
    m_dwInputFmtCount = 0;
    m_bUseMappedResource = false;
    m_dwMaxBufferSize = 0;
    m_dwMaxWidth = 0;
    m_dwMaxHeight = 0;
}

HRESULT Encoder::Init(IDirect3DDevice9* pD3D9Dev)
{
    HRESULT hr = S_OK;
    if (!pD3D9Dev)
    {
        return E_POINTER;
    }
    m_pD3D9Dev = pD3D9Dev;
    m_pD3D9Dev->AddRef();

    // Load EncodeAPI DLL, based on bit-ness
    hr = LoadEncAPI();
    if (SUCCEEDED(hr))
    {
        // Open Encode Session
        NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS param = { 0 };
        param.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
        param.apiVersion = NVENCAPI_VERSION;
        param.device = pD3D9Dev;
        param.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
        m_pEncodeAPI->nvEncOpenEncodeSessionEx(&param, &m_hEncoder);
    }
    return hr;
}

HRESULT Encoder::SetupEncoder(unsigned int dwWidth, unsigned int dwHeight, unsigned int dwBitRate, unsigned int maxWidth, unsigned int maxHeight,
    unsigned int codec, IDirect3DSurface9 **ppInputSurfs, bool bYUV444)
{
    if (!m_hEncoder)
        return E_FAIL;

    HRESULT hr = S_OK;
    NVENCSTATUS status = NV_ENC_SUCCESS;
    m_dwWidth = dwWidth;
    m_dwHeight = dwHeight;
    m_bIsHEVC = !!codec;
    m_bIsYUV444 = bYUV444;

    // Validate basic configuration
    if (FAILED(hr = ValidateParams()))
    {
        fprintf(stderr, __FUNCTION__": Failed to find a valid encoder configuration.\n");
        return hr;
    }

    if (m_bIsHEVC)
    {
        m_stEncodeGUID = NV_ENC_CODEC_HEVC_GUID;
        m_stCodecProfileGUID = NV_ENC_HEVC_PROFILE_MAIN_GUID;
        m_stPresetGUID = NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID;
    }
    else
    {
        m_stEncodeGUID = NV_ENC_CODEC_H264_GUID;
        m_stCodecProfileGUID = NV_ENC_H264_PROFILE_MAIN_GUID;
        m_stPresetGUID = NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID;
    }

    m_stInitEncParams.encodeGUID      = m_stEncodeGUID;
    m_stInitEncParams.presetGUID      = m_stPresetGUID;
    m_stInitEncParams.version         = NV_ENC_INITIALIZE_PARAMS_VER;
    m_stInitEncParams.encodeWidth     = dwWidth;
    m_stInitEncParams.encodeHeight    = dwHeight;
    m_stInitEncParams.maxEncodeWidth  = maxWidth;
    m_stInitEncParams.maxEncodeHeight = maxHeight;
    m_stInitEncParams.frameRateDen    = 1;
    m_stInitEncParams.frameRateNum    = 30;
    m_stInitEncParams.enablePTD       = 1;

    // Fetch encoder preset configuration
    memset(&m_stPresetConfig, 0, sizeof(m_stPresetConfig));
    m_stPresetConfig.version           = NV_ENC_PRESET_CONFIG_VER;
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
    m_stEncodeConfig.rcParams.averageBitRate  = dwBitRate;
    m_stEncodeConfig.rcParams.maxBitRate      = 6 * (dwBitRate / 5); // 1.2 times
    m_stEncodeConfig.rcParams.vbvBufferSize   = dwBitRate / (m_stInitEncParams.frameRateNum / m_stInitEncParams.frameRateDen);
    m_stEncodeConfig.rcParams.vbvInitialDelay = m_stEncodeConfig.rcParams.vbvInitialDelay;


    if (m_bIsHEVC)
    {
        m_stEncodeConfig.encodeCodecConfig.hevcConfig.chromaFormatIDC = m_bIsYUV444 ? 3 : 1;
        m_stEncodeConfig.encodeCodecConfig.hevcConfig.hevcVUIParameters.chromaSampleLocationFlag = 1;
        m_stEncodeConfig.encodeCodecConfig.hevcConfig.hevcVUIParameters.chromaSampleLocationBot  = 1;
        m_stEncodeConfig.encodeCodecConfig.hevcConfig.hevcVUIParameters.chromaSampleLocationTop  = 1;
        
        // ColourMatrix values 1=BT.709, 4=FCC, 5,6=BT.601; 7=SMPTE 240M; 9,10=BT.2020
        // If unspecified = BT.601 up to 768x576, = BT.709 for HD
        m_stEncodeConfig.encodeCodecConfig.hevcConfig.hevcVUIParameters.colourDescriptionPresentFlag = 1;
        m_stEncodeConfig.encodeCodecConfig.hevcConfig.hevcVUIParameters.colourMatrix                 = 1;  // (BT.709)
        m_stEncodeConfig.encodeCodecConfig.hevcConfig.hevcVUIParameters.transferCharacteristics      = 13; // (SRGB)
        m_stEncodeConfig.encodeCodecConfig.hevcConfig.hevcVUIParameters.colourPrimaries              = 1;  // (BT.709/sRGB)
    }
    else
    {
        m_stEncodeConfig.encodeCodecConfig.h264Config.chromaFormatIDC = m_bIsYUV444 ? 3 : 1;
        m_stEncodeConfig.encodeCodecConfig.h264Config.h264VUIParameters.chromaSampleLocationFlag = 1;
        m_stEncodeConfig.encodeCodecConfig.h264Config.h264VUIParameters.chromaSampleLocationBot  = 1;
        m_stEncodeConfig.encodeCodecConfig.h264Config.h264VUIParameters.chromaSampleLocationTop  = 1;

        // ColourMatrix values 1=BT.709, 4=FCC, 5,6=BT.601; 7=SMPTE 240M; 9,10=BT.2020
        // If unspecified = BT.601 up to 768x576, = BT.709 for HD
        m_stEncodeConfig.encodeCodecConfig.h264Config.h264VUIParameters.colourDescriptionPresentFlag = 1;
        m_stEncodeConfig.encodeCodecConfig.h264Config.h264VUIParameters.colourMatrix                 = 1;  // (BT.709)
        m_stEncodeConfig.encodeCodecConfig.h264Config.h264VUIParameters.transferCharacteristics      = 13; // (SRGB)
        m_stEncodeConfig.encodeCodecConfig.h264Config.h264VUIParameters.colourPrimaries              = 1;  // (BT.709/sRGB)
    }
    m_stInitEncParams.encodeConfig = &m_stEncodeConfig;

    // Initialize encoder
    status = m_pEncodeAPI->nvEncInitializeEncoder(m_hEncoder, &m_stInitEncParams);
    if (status != NV_ENC_SUCCESS)
    {
        fprintf(stderr, __FUNCTION__": Failed to initialize encoder.\n");
        return E_FAIL;
    }

    // Register Input buffers
    if (ppInputSurfs)
    {
        for (int i = 0; i < MAX_BUF_QUEUE; i++)
        {
            if (ppInputSurfs[i])
            {
                m_stInputSurface[i].pD3DSurf = ppInputSurfs[i];
                NV_ENC_REGISTER_RESOURCE param = { 0 };
                // Register resource if not already registered
                param.version = NV_ENC_REGISTER_RESOURCE_VER;
                param.resourceToRegister = (void *)m_stInputSurface[i].pD3DSurf;
                param.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
                param.width = m_dwWidth;
                param.height = m_dwHeight;
                status = m_pEncodeAPI->nvEncRegisterResource(m_hEncoder, &param);
                if (status != NV_ENC_SUCCESS)
                {
                    fprintf(stderr, __FUNCTION__": Failed to register resource with encoder.\n");
                    return E_FAIL;
                }
                m_stInputSurface[i].hRegisteredHandle = param.registeredResource;
            }
            else
            {
                fprintf(stderr, __FUNCTION__": No inpur resources provided. Quitting.\n");
                return E_POINTER;
            }
        }
    }
    else
    {
        fprintf(stderr, __FUNCTION__": No inpur resources provided. Quitting.\n");
        return E_POINTER;
    }

    // Allocate IO Buffers
    if (FAILED(hr = AllocateOPBufs()))
    {
        fprintf(stderr, __FUNCTION__": Failed to allocate IO buffers.\n");
        return hr;
    }
    return S_OK;
}

HRESULT Encoder::Reconfigure(unsigned int dwWidth, unsigned int dwHeight, unsigned int dwBitRate, BOOL bNeedNewResources)
{
    if (!m_hEncoder)
        return E_FAIL;

    HRESULT hr = S_OK;
    NVENCSTATUS status = NV_ENC_SUCCESS;

    if (bNeedNewResources)
    {
        hr = ReleaseIPBufs();
        //hr = ReleaseOPBufs();
        if (hr != S_OK)
        {
            fprintf(stderr, __FUNCTION__": Failed to release old buffers.\n");
            return hr;
        }
    }

    m_dwWidth = dwWidth;
    m_dwHeight = dwHeight;
    m_stInitEncParams.encodeWidth  = m_dwWidth;
    m_stInitEncParams.encodeHeight = m_dwHeight;
    m_stInitEncParams.encodeConfig->rcParams.averageBitRate = dwBitRate;

    memset(&m_stReconfigureParam, 0, sizeof(m_stReconfigureParam));
    m_stReconfigureParam.version      = NV_ENC_RECONFIGURE_PARAMS_VER;
    m_stReconfigureParam.resetEncoder = 1;
    m_stReconfigureParam.forceIDR     = 1;
    m_stReconfigureParam.reInitEncodeParams = m_stInitEncParams;
    status = m_pEncodeAPI->nvEncReconfigureEncoder(m_hEncoder, &m_stReconfigureParam);
    if (status != NV_ENC_SUCCESS)
    {
        fprintf(stderr, __FUNCTION__": Failed to reconfigure the encoder.\n");
        return E_FAIL;
    }
    return S_OK;
}

HRESULT Encoder::LaunchEncode(unsigned int i)
{
    if (!m_hEncoder)
        return E_FAIL;
    HRESULT hr = S_OK;
    NVENCSTATUS status = NV_ENC_SUCCESS;

    if (m_stInputSurface[i].hRegisteredHandle == NULL)
    {
        fprintf(stderr, __FUNCTION__": No input resources registered. Quitting.\n");
        return E_FAIL;
    }

    if (m_stInputSurface[i].hInputSurface == NULL)
    {
        NV_ENC_MAP_INPUT_RESOURCE mapParam = { 0 };
        mapParam.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
        mapParam.registeredResource = m_stInputSurface[i].hRegisteredHandle;
        status = m_pEncodeAPI->nvEncMapInputResource(m_hEncoder, &mapParam);
        if (status != NV_ENC_SUCCESS)
        {
            fprintf(stderr, __FUNCTION__": Failed to map resource with encoder.\n");
            return E_FAIL;
        }

		// Assumption here is that depending on the D3DFormat chosen, the NvEncMapInputResource will return the associated
		// NV12 (NV_ENC_BUFFER_FORMAT_NV12_PL) or YUV444 (NV_ENC_BUFFER_FORMAT_YUV444_PL)
        m_stInputSurface[i].hInputSurface = mapParam.mappedResource;
        m_stInputSurface[i].bufferFmt     = mapParam.mappedBufferFmt;
        m_bUseMappedResource = true;
    }

    // Check states and launch encode
    memset(&m_stEncodePicParams, 0, sizeof(m_stEncodePicParams));
    m_stEncodePicParams.version = NV_ENC_PIC_PARAMS_VER;
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
        fprintf(stderr, __FUNCTION__": Failed to encode input\n");
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
        unsigned char buf[1024] = { '\0' };
        unsigned int size = 0;
        NV_ENC_SEQUENCE_PARAM_PAYLOAD param = { 0 };
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
    m_dwFrameCount++;
    // Fetch encoded bitstream
    NV_ENC_LOCK_BITSTREAM lockBitstreamData = { 0 };
    lockBitstreamData.version = NV_ENC_LOCK_BITSTREAM_VER;
    lockBitstreamData.outputBitstream = m_stBitstreamBuffer[i].hBitstreamBuffer;
    lockBitstreamData.doNotWait = false;

    nvStatus = m_pEncodeAPI->nvEncLockBitstream(m_hEncoder, &lockBitstreamData);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        fprintf(stderr, __FUNCTION__": Failed to lock bitstream.\n");
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

HRESULT Encoder::AllocateOPBufs()
{
    NVENCSTATUS status = NV_ENC_SUCCESS;
    if (!m_hEncoder)
        return E_FAIL;

    for (unsigned int i = 0; i < MAX_BUF_QUEUE; i++)
    {
        m_stInputSurface[i].dwWidth = m_dwWidth;
        m_stInputSurface[i].dwHeight = m_dwHeight;

        m_stBitstreamBuffer[i].dwSize = 1024 * 1024;
        NV_ENC_CREATE_BITSTREAM_BUFFER stAllocBitstream;
        memset(&stAllocBitstream, 0, sizeof(stAllocBitstream));
        stAllocBitstream.version    = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
        stAllocBitstream.size       = 2 * 1024 * 1024;//m_stBitstreamBuffer[i].dwSize;
        stAllocBitstream.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;

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
    ReleaseIPBufs();
    ReleaseOPBufs();
    // Destroy encode session
    status = m_pEncodeAPI->nvEncDestroyEncoder(m_hEncoder);
    // Reset local states
    m_hEncoder = NULL;
    return E_FAIL;
}

HRESULT Encoder::ReleaseIPBufs()
{
    NVENCSTATUS status = NV_ENC_SUCCESS;
    if (!m_hEncoder)
        return E_FAIL;
    // Destroy allocated buffers, unregister registered buffers
    for (unsigned int i = 0; i < MAX_BUF_QUEUE; i++)
    {
        if (m_stInputSurface[i].hRegisteredHandle)
            status = m_pEncodeAPI->nvEncUnregisterResource(m_hEncoder, m_stInputSurface[i].hRegisteredHandle);
        memset(&m_stInputSurface[i], 0, sizeof(EncodeInputSurfaceInfo));
    }

    return S_OK;
}
HRESULT Encoder::ReleaseOPBufs()
{
    NVENCSTATUS status = NV_ENC_SUCCESS;
    if (!m_hEncoder)
        return E_FAIL;

    // Destroy allocated buffers, unregister registered buffers
    for (unsigned int i = 0; i < MAX_BUF_QUEUE; i++)
    {
        if (m_stBitstreamBuffer[i].hBitstreamBuffer)
            status = m_pEncodeAPI->nvEncDestroyBitstreamBuffer(m_hEncoder, m_stBitstreamBuffer[i].hBitstreamBuffer);
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
        nvEncodeAPICreateInstance = (MYPROC)GetProcAddress(m_hinstLib, "NvEncodeAPICreateInstance");

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

    return E_FAIL;
}