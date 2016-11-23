/*
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma once
#include "NvEncodeAPI/NvEncodeAPI.h"

// Encodering params structure, contains the data needed to initialize
// an encoder.

#define MAX_BUF_QUEUE 1

static char __NVEncodeLibName32[] = "nvEncodeAPI.dll";
static char __NVEncodeLibName64[] = "nvEncodeAPI64.dll";

typedef NVENCSTATUS(__stdcall *MYPROC)(NV_ENCODE_API_FUNCTION_LIST *);

typedef struct 
{
    unsigned int      dwWidth;
    unsigned int      dwHeight;
    unsigned int      dwLumaOffset;
    unsigned int      dwChromaOffset;
    void              *hInputSurface;
    unsigned int      lockedPitch;
    NV_ENC_BUFFER_FORMAT bufferFmt;
    void              *pExtAlloc;
    unsigned char     *pExtAllocHost;
    unsigned int      dwCuPitch;
    NV_ENC_INPUT_RESOURCE_TYPE type;
    void              *hRegisteredHandle;
}EncodeInputSurfaceInfo;

typedef struct 
{
    unsigned int     dwSize;
    unsigned int     dwBitstreamDataSize;
    void             *hBitstreamBuffer;
    HANDLE           hOutputEvent;
    bool             bWaitOnEvent;
    void             *pBitstreamBufferPtr;
    bool             bEOSFlag;
    bool             bReconfiguredflag;
}EncodeOutputBuffer;

// Encoder class, simple wrapper around the CUDA interface provided by NvEncode SDK
// See https://developer.nvidia.com/sites/default/files/akamai/cuda/files/CUDADownloads/NVENC_VideoEncoder_API_ProgGuide.pdf
// for more details.
class Encoder
{
    HMODULE                                              m_hinstLib;
    HANDLE                                               m_hEncoder;
    NV_ENCODE_API_FUNCTION_LIST                         *m_pEncodeAPI;
    // IDirect3DDevice9                                    *pD3d9Dev;
    CUcontext                                            m_cuContext;
    CUdeviceptr                                          m_cuDevptr;
    unsigned int                                         m_dwEncodeGUIDCount;
    GUID                                                 m_stEncodeGUID;
    unsigned int                                         m_dwCodecProfileGUIDCount;
    GUID                                                 m_stCodecProfileGUID;
    unsigned int                                         m_dwPresetGUIDCount;
    GUID                                                 m_stPresetGUID;
    unsigned int                                         m_encodersAvailable;
    unsigned int                                         m_dwInputFmtCount;
    NV_ENC_BUFFER_FORMAT                                *m_pAvailableSurfaceFmts;
    NV_ENC_BUFFER_FORMAT                                 m_dwInputFormat;
    NV_ENC_INITIALIZE_PARAMS                             m_stInitEncParams;
    NV_ENC_CONFIG                                        m_stEncodeConfig;
    NV_ENC_PRESET_CONFIG                                 m_stPresetConfig;
    NV_ENC_PIC_PARAMS                                    m_stEncodePicParams;
    NV_ENC_RECONFIGURE_PARAMS                            m_stReconfigureParam;
    //EncodeConfig                                         m_stEncoderInput[MAX_RECONFIGURATION];
    EncodeInputSurfaceInfo                               m_stInputSurface[MAX_BUF_QUEUE];
    EncodeOutputBuffer                                   m_stBitstreamBuffer[MAX_BUF_QUEUE];
    unsigned int                                         m_dwWidth;
    unsigned int                                         m_dwHeight;
    unsigned int                                         m_dwBufferWidth;
    bool                                                 m_bUseMappedResource;
    unsigned int                                         m_dwMaxBufferSize;
    unsigned int                                         m_dwFrameCount;
public:
    Encoder();
    ~Encoder();
public:
    HRESULT Init(CUcontext cuCtx, unsigned int dwMaxBufferSize);
    HRESULT SetupEncoder(unsigned int dwWidth, unsigned int dwHeight, unsigned int dwBufferWidth, unsigned int dwBitRate);
    HRESULT LaunchEncode(unsigned int i, CUdeviceptr devptr);
    HRESULT GetBitstream(unsigned int i, FILE *fout);
    HRESULT TearDown();
    HRESULT Reconfigure(unsigned int dwWidth, unsigned int dwHeight, unsigned int dwBufferWidth, unsigned int dwBitRate);
private:
    HRESULT AllocateIOBufs();
    HRESULT Flush();
    HRESULT ReleaseIOBufs();
    HRESULT LoadEncAPI();
    HRESULT ValidateParams();
    inline BOOL Is64Bit() { return (sizeof(void *)!=sizeof(DWORD)); }
};