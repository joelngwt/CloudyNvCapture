/**
 * \file This file contains definitions for NVIFRHWEncoder Interface.
 *
 * Copyright 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 *
 */

#ifndef _NV_IFR_HW_ENC_
#define _NV_IFR_HW_ENC_

#include "NvIFR.h"
#include "nvHWEnc.h"
/**
 * \defgroup NVIFR_HWENC NVIFR Codec-agnostic HW Encoder Interface
 * \brief Defines NVIFR interface for direct compression using HW encoder. Outpts H.264 or HEVC bistream packets.
 */

/** 
 * \defgroup NVIFR_HWENC_STRUCTS Structs
 * \ingroup NVIFR_HWENC
 * \brief Definitions of Structures to be used with NVIFRHWEncoder Interface.
 */

/**
 * \defgroup NVIFR_HWENC_INTERFACE Object Interface
 * \ingroup NVIFR_HWENC
 * \brief Interface class definition for NVIFR HW Encode API
 */

/**
 * \ingroup NVIFR_HWENC
 * Macro to define INVIFRToHWEncoder Interface ID.
 */
#define NVIFR_TO_HWENCODER (0X2204) 

/**
 * \ingroup NVIFR_HWENC_STRUCTS
 * Defines the parameters for configuring the INVIFRToHWEncoder interface.
 * To be used with INVIFRToHWEncoder::NvIFRSetupHWEncoder API.
 */
typedef struct _NVIFR_HW_ENC_SETUP_PARAMS
{
    NvU32 dwVersion;                                                        /**< [in]: Struct version. Set to NVIFR_HW_ENC_SETUP_PARAMS_VER. */
    NV_HW_ENC_STEREO_FORMAT eStreamStereoFormat;                            /**< [in]: Set to one of the NV_HW_ENC_STEREO_FORMAT values. Default = NV_HW_ENC_STEREO_NONE. */
    NvU32 dwTargetWidth;                                                    /**< [in]: Specifies target with to capture */
    NvU32 dwTargetHeight;                                                   /**< [in]: Specifies target height to capture */
    NvU32 dwNBuffers;                                                       /**< [in]: Specifies number of bitstream buffers to be created. */
    NvU32 dwBSMaxSize;                                                      /**< [in]: Specifies maximum bitstream buffer size in bytes. */
    NV_HW_ENC_CONFIG_PARAMS configParams;                                   /**< [in]: Struct containing HW encoder initial configuration parameters. */
    unsigned char ** ppPageLockedBitStreamBuffers;                          /**< [out]: Container to hold the bitstream buffers allocated by NvIFR in system memory. */
    HANDLE * ppEncodeCompletionEvents;                                      /**< [out]: Container to hold the asynchronous encode completion events allocated by NvIFR. */
#pragma message("Custom YUV444 is deprecated and will be removed in future versions of GRID SDK")
#pragma deprecated(bIsCustomYUV444)
    NvU32 bIsCustomYUV444;                                                  /**< [out]: NvIFR will set this to 1 if the client has requested YUV444 capture,
                                                                                        and NvIFR needed to use custom YUV444 representation.
                                                                                        Only supported for H.264 encode on Kepler-architecture GPUs.  */
    NvU32 dwReserved[56];                                                   /**< [in]: Reserved. Set to 0. */
    void *pReserved[29];                                                    /**< [in]: Reserved. Set to NULL. */
} NVIFR_HW_ENC_SETUP_PARAMS;
#define NVIFR_HW_ENC_SETUP_PARAMS_VER NVIFR_STRUCT_VERSION(NVIFR_HW_ENC_SETUP_PARAMS, 1)

/**
 * \ingroup NVIFR_HWENC_STRUCTS
 * Defines the paramters for capturing and encoding the RenderTarget data using INVIFRToHWEncoder interface
 * To be used with INVIFRToHWEncoder::NvIFRTransferRenderTaretToHWEncoder API.
 */
typedef struct _NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS
{
    NvU32 dwVersion;                                                        /**< [in]: Struct version. Set to NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS_VER. */
    NvU32 dwBufferIndex;                                                    /**< [in]: Index of the capture buffer to be used. */
    NV_HW_ENC_PIC_PARAMS encodePicParams;                                   /**< [in]: Struct containing per-frame configuration parameters for the H264 HW Encoder. */
    NvU32 dwReserved[62];                                                   /**< [in]: Reserved. Set to 0. */
    void *pReserved[31];                                                    /**< [in]: Reserved. Set to NULL. */
} NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS;
#define NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS_VER NVIFR_STRUCT_VERSION(NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS, 1)

/**
 * \ingroup NVIFR_HWENC_STRUCTS
 * Defines the parameters describing the output bitstream.
 * To be used with INVIFRToHWEncoder::NvIFRGetStatsFromHWEncoder API.
 */
typedef struct _NVIFR_HW_ENC_GET_BITSTREAM_PARAMS
{
    NvU32 dwVersion;                                                        /**< [in]: Struct version. Set to NVIFR_HW_ENC_GET_BITSTREAM_PARAMS_VER. */
    NvU32 dwBufferIndex;                                                    /**< [in]: Index of the capture buffer to be used. */
    NV_HW_ENC_GET_BIT_STREAM_PARAMS bitStreamParams;                        /**< [in/out]: Client provided struct populated by the H264 HW Encoder with encoding stats. */
    NvU32 dwReserved[62];                                                   /**< [in]: Reserved. Set to 0. */
    void *pReserved[31];                                                    /**< [in]: Reserved. Set to NULL. */
} NVIFR_HW_ENC_GET_BITSTREAM_PARAMS;
#define NVIFR_HW_ENC_GET_BITSTREAM_PARAMS_VER NVIFR_STRUCT_VERSION(NVIFR_HW_ENC_GET_BITSTREAM_PARAMS, 1)

/**
 * \ingroup NVIFR_HWENC_STRUCTS
 * Defines the parameters used to fetch SPS \ PPS header from the HW Encoder.
 * To be used with INVIFRToHWEncoder::NvIFRGetHeaderFromHWEncoder API.
 */
typedef struct _NVIFR_HW_ENC_GET_STREAM_HEADER_PARAMS
{
    NvU32 dwVersion;                                                        /**< [in]: Struct version. Set to NVIFR_HW_ENC_GET_STREAM_HEADER_PARAMS_VER. */
    NvU32 dwReserved1;                                                      /**< [in]: Reserved. Set to 0. */
    NvU32 dwSize;                                                           /**< [out]: Size of SPS PPS header data generated by the H264 HW Encoder */
    NvU8  *pBuffer;                                                         /**< [out]: Output buffer containing SPS PPS header data generated by the H264 HW Encoder */
    NvU32 dwReserved[62];                                                   /**< [in]: Reserved. Set to 0. */
    void *pReserved[331];                                                    /**< [in]: Reserved. Set to NULL. */
} NVIFR_HW_ENC_GET_STREAM_HEADER_PARAMS;
#define NVIFR_HW_ENC_GET_STREAM_HEADER_PARAMS_VER NVIFR_STRUCT_VERSION(NVIFR_HW_ENC_GET_STREAM_HEADER_PARAMS, 1)


/**
 * \ingroup NVIFR_HWENC_INTERFACE
 * Interface Defintion for NVIFR HW Encode Codec-Agnostic Interface.
 * One instance of the INVIFRToHWEncoder interface object corrsponds to one NVIFRToHWEncoder session.
 * Excatly one session per D3D device is supported.
 */
class INvIFRToHWEncoder_v1
{
public:
    /**
     * \brief NVIFR API for retrieving HW Encoder capabilities.
     * \param [inout] pGetCaps Pointer to NV_HW_ENC_GET_CAPS structure.
     * \return An applicable ::NVIFRRESULT value.
     */
    virtual NVIFRRESULT NvIFRGetHWEncCaps(NV_HW_ENC_GET_CAPS* pGetCaps) = 0;
    
    /**
     * \brief NVIFR API for configuring the Capture session and the HW Encoder
     * This allocates internal resources and initializes the NVIDIA HW Encoder session.
     * \param [inout] pSetUpParams Pointer to ::NVIFR_HW_ENC_SETUP_PARAMS structure.
     * \return An applicable ::NVIFRRESULT value.
     */
    virtual NVIFRRESULT NvIFRSetUpHWEncoder(NVIFR_HW_ENC_SETUP_PARAMS *pSetUpParams) = 0;

    /**
     * \brief NVIFR API for capturing RendertargetData and initiating the video encode process using HW Encoder.
     * \param [inout] pTransferParams Pointer to ::NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS structure.
     * \return An applicable ::NVIFRRESULT value.
     */
    virtual NVIFRRESULT NvIFRTransferRenderTargetToHWEncoder(NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS *pTransferParams) = 0;

    /**
     * \brief NVIFR API for fetching the output bitstream from HW Encoder.
     * \param [inout] pFrameStatsParams Pointer to ::NVIFR_HW_ENC_GET_BITSTREAM_PARAMS structure.
     * \return An applicable NVIFRRESULT value.
     */
    virtual NVIFRRESULT NvIFRGetStatsFromHWEncoder(NVIFR_HW_ENC_GET_BITSTREAM_PARAMS *pFrameStatsParams) = 0;
    
    /**
     * \brief NVIFR API for fetching SPS \ PPS information from HW Encoder.
     * \param [inout] pStreamHeaderParams Pointer to ::NVIFR_HW_ENC_GET_STREAM_HEADER_PARAMS structure.
     * \return An applicable ::NVIFRRESULT value.
     */
    virtual NVIFRRESULT NvIFRGetHeaderFromHWEncoder(NVIFR_HW_ENC_GET_STREAM_HEADER_PARAMS *pStreamHeaderParams) = 0;

    /**
     * \brief NVIFR API for destroying the NVIFR capture session.
     * \return An applicable ::NVIFRRESULT value.
     */
    virtual NVIFRRESULT NvIFRRelease() = 0;
};

typedef INvIFRToHWEncoder_v1 INvIFRToHWEncoder;

#endif