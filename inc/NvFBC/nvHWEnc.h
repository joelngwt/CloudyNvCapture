/**
* \file This file contains definitions for use with NVIDIA HW Encoder
*
 * Copyright 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 *
 */

#ifndef _NV_HW_ENC_
#define _NV_HW_ENC_

typedef unsigned long NvU32;
typedef unsigned long long NvU64;
typedef unsigned char NvU8;

/**
* \defgroup NV_HW_ENC NVIDIA HW Encoder parameters.
* \ingroup NVIFR_HWENC NVFBC_HWENC
* \brief Defines NVIDIA HW Encoder parameters.
*/
/**
* \defgroup NV_HW_ENC_ENUMS Enums
* \ingroup NV_HW_ENC NVIFR_HWENC NVFBC_HWENC
* \brief  NVIDIA HW Encoder enumerations
*/

/**
* \defgroup NV_HW_ENC_ENUMS Enums
* \ingroup NV_HW_ENC NVIFR_HWENC NVFBC_HWENC
* \brief  NVIDIA HW Encoder enumerations
*/

/**
* \ingroup NV_HW_ENC
* \brief Macro to generate struct versions.
*/
#define NVSTRUCT_VERSION(typeName, ver) (NvU32)(sizeof(typeName) | (ver)<<16) 

/**
* \ingroup NV_HW_ENC
* \brief Maximum number of reference frames to be used with NVIDIA HW Encoder.
*/
const unsigned int NV_HW_ENC_MAX_REF_FRAMES = 0x10;
/**
* \ingroup NV_HW_ENC
* \brief Maximum number of slices supported by NVIDIA HW Encoder.
*/
const unsigned int MAX_NUMBER_OF_SLICES = 16;

/**
* \ingroup NV_HW_ENC_ENUMS
* \brief Enumerates the video Codecs supported by the NVIFR HW Encoder.
*/
typedef enum _NV_HW_ENC_CODEC
{
    NV_HW_ENC_UNKNOWN = -1,   /**< Sentinel, do not use. */
    NV_HW_ENC_H264 = 4,    /**< Use this for configuring the session to perform H.264 encoding. */
    NV_HW_ENC_HEVC = 7,    /**< Use this for configuring the session to perform HEVC(H.265) encoding. */
} NV_HW_ENC_CODEC;

/**
* \ingroup NV_HW_ENC_ENUMS
* \brief Enumerates the rate control modes supported by the NVIDIA HW Encoder.
*/
typedef enum _NV_HW_ENC_PARAMS_RC_MODE
{
    NV_HW_ENC_PARAMS_RC_CONSTQP = 0x0,                      /**< Constant QP mode. */
    NV_HW_ENC_PARAMS_RC_VBR = 0x1,                          /**< Variable bitrate mode. */
    NV_HW_ENC_PARAMS_RC_CBR = 0x2,                          /**< Constant bitrate mode. */
    NV_HW_ENC_PARAMS_RC_2_PASS_QUALITY = 0x8,               /**< Multi pass encoding optimized for image quality and works best with single frame VBV buffer size. */
    NV_HW_ENC_PARAMS_RC_2_PASS_FRAMESIZE_CAP = 0x10,        /**< Multi pass encoding optimized for maintaining frame size and works best with single frame VBV buffer size. */
    NV_HW_ENC_PARAMS_RC_2_PASS_VBR = 0x20                   /**< Multi pass VBR. */
} NV_HW_ENC_PARAMS_RC_MODE;

/**
* \ingroup NV_HW_ENC_ENUMS
* \brief Enumerates the NVIDIA HW Encoder presets available for use.
*/
typedef enum _NV_HW_ENC_PRESET
{
    NV_HW_ENC_PRESET_LOW_LATENCY_HP = 0,                    /**< Use for Fastest encode, with suboptimal quality. */
    NV_HW_ENC_PRESET_LOW_LATENCY_HQ,                        /**< Use for better overall quality, compromising on encoding speed. */
    NV_HW_ENC_PRESET_LOW_LATENCY_DEFAULT,                   /**< Use for better quality than NV_HW_ENC_PRESET_LOW_LATENCY_HP and higher speed than NV_HW_ENC_PRESET_LOW_LATENCY_HQ. */
    NV_HW_ENC_PRESET_RESERVED2,                             /**< Do not use. */
    NV_HW_ENC_PRESET_LOSSLESS_HP,                           /**< Use for lossless encoding at higher performance. */
    NV_HW_ENC_PRESET_LAST,                                  /**< Sentinel value. Do not use. */
} NV_HW_ENC_PRESET;

/**
* \ingroup NV_HW_ENC_ENUMS
* \brief Enumerates the NVIDIA HW Encoder supported modes for dividing encoded picture into slices.
*/
typedef enum _NV_HW_ENC_SLICING_MODE
{
    NV_HW_ENC_SLICING_MODE_DISABLED = 0,                     /**< Picture will be encoded as one slice. */
    NV_HW_ENC_SLICING_MODE_FIXED_NUM_MBS,                    /**< Picture will be divided into slices of n MBs, where n = dwSlicingModeParam. */
    NV_HW_ENC_SLICING_MODE_FIXED_NUM_BYTES,                  /**< Picture will be divided into slices of n Bytes, where n = dwSlicingModeParam. */
    NV_HW_ENC_SLICING_MODE_FIXED_NUM_MB_ROWS,                /**< Picture will be divided into slices of n rows of MBs, where n = dwSlicingModeParam. */
    NV_HW_ENC_SLICING_MODE_FIXED_NUM_SLICES,                 /**< Picture will be divided into n+1 slices, where n = dwSlicingModeParam. */
    NV_HW_ENC_SLICING_MODE_LAST,                             /**< Sentine value. Do not use. */
} NV_HW_ENC_SLICING_MODE;

/**
* \ingroup NV_HW_ENC_ENUMS
* \brief Enumerates the Deblocking modes supported by the NVIDIA HW Encoder.
*/
typedef enum _NV_HW_ENC_DEBLOCKING_FILTER_MODE
{
    NV_HW_ENC_DEBLOCKING_FILTER_MODE_DEFAULT = 0x0,         /**< Deblocking is enabled across the complete frame. */
    NV_HW_ENC_DEBLOCKING_FILTER_MODE_DISABLE = 0x01,        /**< Deblocking is disabled completely. */
    NV_HW_ENC_DEBLOCKING_FILTER_MODE_SLICE = 0x02,          /**< Deblocking is enabled but disabled across the slice boundary. */
} NV_HW_ENC_DEBLOCKING_FILTER_MODE;

/**
* \ingroup NV_HW_ENC_ENUMS
* \brief Enumerates stereo encoding formats. Currently unsupported.
*/
typedef enum _NV_HW_ENC_STEREO_FORMAT
{
    NV_HW_ENC_STEREO_NONE = 0,                              /**< Mono grab. Default use case. */
    NV_HW_ENC_STEREO_MVC,                                   /**< Full frames with MVC coding. */
    NV_HW_ENC_STEREO_TB,                                    /**< Top Bottom squeezed, with SEI inband message. */
    NV_HW_ENC_STEREO_LR,                                    /**< Left Right squeezed, with SEI inband message. */
    NV_HW_ENC_STEREO_SBS,                                   /**< Left Right squeezed, no SEI. */
    NV_HW_ENC_STEREO_LAST,                                  /**< Sentinel value. Do not use. */
} NV_HW_ENC_STEREO_FORMAT;

/**
* \ingroup NV_HW_ENC_STRUCTS
* \brief Defines the parameters to be used for configuring the NVIDIA HW Encoder.
*/
typedef struct _NV_HW_ENC_CONFIG_PARAMS
{
    NvU32                               dwVersion;                      /**< [in]: Set to NV_HW_ENC_CONFIG_PARAMS_VER. */
    NvU32                               dwProfile;                      /**< [in]: Codec profile that the HW encoder should use for video encoding.
                                                                                    66: Baseline (fastest encode / decode times, lowest image quality, lowest bitrate)
                                                                                    77: Main (balanced encode / decode times, image quality, and bitrate)
                                                                                    100: High (slowest encode / decode times, highest image quality, highest bitrate)
                                                                                    Overridden to 244 for H264 if client is using NV_HW_ENC_PRESET_LOSSLESS_HP. */
    NvU32                               dwFrameRateNum;                 /**< [in]: Frame rate numerator. Encoding frame rate = dwFrameRateNum / dwFrameRateDen.
                                                                                    This is not related to rate at which frames are grabbed. */
    NvU32                               dwFrameRateDen;                 /**< [in]: Frame rate denominator. Encoding frame rate = dwFrameRateNum / dwFrameRateDen.
                                                                                    This is not related to rate at which frames are grabbed. */
    NvU32                               dwAvgBitRate;                   /**< [in]: Target Average bitrate. HW Encoder will try to achieve this bitrate during video encoding.
                                                                                    This is the only bitrate setting useful for Constant Bit Rate RateControl mode.
                                                                                    This is ignored for NV_HW_ENC_PARAMS_RC_CONSTQP mode. */
    NvU32                               dwPeakBitRate;                  /**< [in]: Maximum bitrate that the HW encoder can hit while encoding video in VBR [variable bit rate mode].
                                                                                    This is ignored for NV_HW_ENC_PARAMS_RC_CONSTQP mode. */
    NvU32                               dwGOPLength;                    /**< [in]: Number of pictures in a GOP. every GOP beigins with an I frame, so this is the same as I-frame interval. */
    NvU32                               dwQP;                           /**< [in]: QP value to be used for rate control in ConstQP mode.
                                                                                    Overridden to 0 if client is using NV_HW_ENC_PRESET_LOSSLESS_HP. */
    NV_HW_ENC_PARAMS_RC_MODE            eRateControl;                   /**< [in]: Rate Control Mode to be used by the HW encoder.
                                                                                    Should be set to NV_HW_ENC_PARAMS_RC_CONSTQP for using NV_HW_ENC_PRESET_LOSSLESS_HP preset. */
    NV_HW_ENC_PRESET                    ePresetConfig;                  /**< [in]: Specifies the encoding preset used for fine tuning for encoding parameters */
    NvU32                               bOutBandSPSPPS : 1;             /**< [in]: Set this to 1, for enabling out of band generation of SPS, PPS headers. */
    NvU32                               bRecordTimeStamps : 1;          /**< [in]: Set this to 1, if client wants timestamp information to be preserved. Client should send timestamps as input. */
    NvU32                               bIntraFrameOnRequest : 1;       /**< [in]: Reserved. Set to 0. */
    NvU32                               bUseMaxRCQP : 1;                /**< [in]: Set this to 1, if client wants to specify maxRCQP[] */
    NvU32                               bEnableSubframeEncoding : 1;    /**< [in]: Unsupported. Set this to 0. */
    NvU32                               bRepeatSPSPPSHeader : 1;        /**< [in]: Fill SPS PPS header for every IDR frame. */
    NvU32                               bEnableIntraRefresh : 1;        /**< [in]: Set to 1, to enable gradual decoder refresh or intra-refresh.
                                                                                    If the GOP structure uses B frames this will be ignored. */
    NvU32                               bEnableConstrainedEncoding : 1; /**< [in]: Unsupported. Set this to 0. */
    NvU32                               bEnableMSE : 1;                 /**< [in]: Set this to 1 to enable returning MSE per channel in ::NV_HW_ENC_GRAB_FRAME_PARAMS
                                                                                    NOTE: Enabling this bit will have affect performance severly,
                                                                                    Set this bit only if Client wants to evaluate quality of encoder. */
    NvU32                               bEnableYUV444Encoding : 1;      /**< [in]: Set this to 1 to enable YUV444 encoding. */
    NvU32                               bEnableAdaptiveQuantization : 1;/**< [in]: Set this to 1 if client wants to enable adaptive quantization to encode. */
    NvU32                               bReservedBits : 21;             /**< [in]: Reserved. Set to 0. */
    NV_HW_ENC_SLICING_MODE              eSlicingMode;                   /**< [in]: Refer to enum NV_HW_ENC_SLICING_MODE for usage. */
    NvU32                               dwSlicingModeParam;             /**< [in]: Refer to enum NV_HW_ENC_SLICING_MODE for usage. */
    NvU32                               dwVBVBufferSize;                /**< [in]: VBV buffer size can be used to cap the frame size of encoded bitstream, reducing the need for buffering at decoder end.
                                                                                    For lowest latency, VBV buffersize = single frame size = channel bitrate/frame rate.
                                                                                    I-frame size may be larger than P or B frames.
                                                                                    Overridden by NvFBC in case of NV_HW_ENC_PARAMS_RC_2_PASS_QUALITY or NV_HW_ENC_PARAMS_RC_2_PASS_FRAMESIZE_CAP rate control modes. */
    NvU32                               dwVBVInitialDelay;              /**< [in]: No. of bits to buffer at the decoder end. For lowest latency, set to be equal to dwVBVBufferSize.
                                                                                    Overridden by NvFBC in case of NV_HW_ENC_PARAMS_RC_2_PASS_QUALITY or NV_HW_ENC_PARAMS_RC_2_PASS_FRAMESIZE_CAP rate control modes. */
    NvU32                               maxRCQP[3];                     /**< [in]: maxQP[0] = max QP for P frame, maxRCQP[1] = max QP for B frame, maxRCQP[2] = max QP for I frame respectively. */
    NvU32                               dwMaxNumRefFrames;              /**< [in]: This is used to specify the DPB size used for encoding. Setting this to 0 will allow encoder to use the default DPB size.
                                                                                    Low latency applications are recommended to use a large DPB size(recommended size is 16) so that it allows clients to invalidate
                                                                                    corrupt frames and use older frames for reference to improve error resiliency. */
    NV_HW_ENC_STEREO_FORMAT             eStereoFormat;                  /**< [in]: Stereo packing format required in the encoded bitstream. Only valid for MVC content. dwProfile == 128. */
    NV_HW_ENC_DEBLOCKING_FILTER_MODE    eDeblockingMode;                /**< [in]: Specifies the deblocking filter mode. Permissible value range: [0,2].
                                                                                    Refer to enum NV_HW_ENC_DEBLOCKING_FILTER_MODE for usage. */
    NV_HW_ENC_CODEC                     eCodec;                         /**< [in]: Specifies the codec to be used.  */
    NvU32                               dwReserved[42];                 /**< [in]: Reserved.Set to 0. */
    void*                               pReserved[32];                  /**< [in]: Reserved. Set to NULL. */
} NV_HW_ENC_CONFIG_PARAMS;
#define NV_HW_ENC_CONFIG_PARAMS_VER NVSTRUCT_VERSION(NV_HW_ENC_CONFIG_PARAMS, 1)

/**
* \ingroup NV_HW_ENC_STRUCTS
* \brief Defines per-frame encoder parameters to be specified to the NVIDIA HW Encoder.
*/
typedef struct _NV_HW_ENC_PIC_PARAMS
{
    NvU32                               dwVersion;                                              /**< [in]: Set to NV_HW_ENC_PIC_PARAMS_VER. */
    NvU32                               dwNewAvgBitRate;                                        /**< [in]: Specifies the new average bit rate to be used from this frame onwards. */
    NvU32                               dwNewPeakBitRate;                                       /**< [in]: Specifies the new peak bit rate to be used from this frame onwards. */
    NvU32                               dwNewVBVBufferSize;                                     /**< [in]: Specifies the new VBV buffer size to be used from this frame onwards. */
    NvU32                               dwNewVBVInitialDelay;                                   /**< [in]: Specifies the new VBV initial delay to be used from this frame onwards. */
    NvU64                               ulCaptureTimeStamp;                                     /**< [in]: Input timestamp to be associated with this input picture. */
    NvU64                               ulInvalidFrameTimeStamp[NV_HW_ENC_MAX_REF_FRAMES];      /**< [in]: This specifies an array of timestamps of the encoder references which the clients wants to invalidate. */
    NvU32                               dwRecoveryFrameCnt;                                     /**< [in]: Unused. Set to 0. */
    NvU32                               bInvalidateRefrenceFrames : 1;                          /**< [in]: Set this to 1 if client wants encoder references to be invalidated. Ignored if Intra-Refresh is enabled for the session. */
    NvU32                               bForceIDRFrame : 1;                                     /**< [in]: Replaces dwEncodeParamFlags. Set this to 1 if client wants the current frame to be encoded as an IDR frame. */
    NvU32                               bConstrainedFrame : 1;                                  /**< [in]: Reserved. Set to 0. */
    NvU32                               bStartIntraRefresh : 1;                                 /**< [in]: Set this to 1 if it wants to force Intra Refresh with intra refresh period dwIntraRefreshCnt. */
    NvU32                               bDynamicBitRate : 1;                                    /**< [in]: Set this to 1 if client wants to change bit rate from current frame. */
    NvU32                               bEnableAdaptiveQuantization : 1;                        /**< [in]: Set this to 1 if client wants to enable adaptive quantization to encode. */
    NvU32                               bReservedBits : 26;                                     /**< [in]: Reserved. Set to 0. */
    NvU32                               dwNumRefFramesToInvalidate;                             /**< [in]: This specifies number of encoder references which the clients wants to invalidate. */
    NvU32                               dwIntraRefreshCnt;                                      /**< [in]: Specifies the number of frames over which intra refresh will happen, if bStartIntraRefresh is set. */
    NvU32                               dwReserved[45];                                         /**< [in]: Reserved.Set to 0. */
    void*                               pReserved[32];                                          /**< [in]: Reserved. Set to NULL. */
} NV_HW_ENC_PIC_PARAMS;
#define NV_HW_ENC_PIC_PARAMS_VER NVSTRUCT_VERSION(NV_HW_ENC_PIC_PARAMS, 1)

/**
* \ingroup NV_HW_ENC_STRUCTS
* \brief Defines the parameters needed to fetch the encoded bistream from the NVIDIA HW Encoder.
*/
typedef struct _NV_HW_ENC_GET_BIT_STREAM_PARAMS
{
    NvU32                               dwVersion;                                              /**< [in]: Set to NV_HW_ENC_GET_BIT_STREAM_PARAMS_VER. */
    NvU32                               dwByteSize;                                             /**< [out]: Size of the generated bitstream, in bytes. */
    NvU32                               bIsIFrame : 1;                                          /**< [out]: Set by the HW Encoder if the generated bitstream corresponds to an I frame. */
    NvU32                               bReserved : 31;                                         /**< [in]: Reserved. Set to 0. */
    NvU64                               ulTimeStamp;                                            /**< [out]: Timestamp associated with the encoded frame. */
    NvU32                               dwSliceOffsets[MAX_NUMBER_OF_SLICES];                   /**< [out]: Slice offsets. */
    NvU32                               dwReserved[60];                                         /**< [in]: Reserved. Set to 0. */
    void*                               pReserved[32];                                          /**< [in]: Reserved. Set to 0. */
} NV_HW_ENC_GET_BIT_STREAM_PARAMS;
#define NV_HW_ENC_GET_BIT_STREAM_PARAMS_VER NVSTRUCT_VERSION(NV_HW_ENC_GET_BIT_STREAM_PARAMS, 1)

/**
* \ingroup NV_HW_ENC_STRUCTS
* \brief Defines the parameters used to check NVIDIA HW Encoder capabilities.
*/
typedef struct  _NV_HW_ENC_GET_CAPS
{
    NvU32                               dwVersion;                                              /**< [in]: Set to NV_HW_ENC_CAPS_VER. */
    NV_HW_ENC_CODEC                     eCodec;                                                 /**< [in]: Codec: Refer to NV_HW_ENC_CODEC for usage. */
    NvU32                               bYUV444Supported : 1;                                   /**< [out]: YUV444 support. */
    NvU32                               bLosslessEncodingSupported : 1;                         /**< [out]: Lossless support. */
    NvU32                               bDynamicBitRateSupported : 1;                           /**< [out]: Dynamic bitrange support. */
    NvU32                               bIntraRefreshSupported : 1;                             /**< [out]: Intra refresh support. */
    NvU32                               bRefPicInvalidateSupported : 1;                         /**< [out]: Ref pic invalidation support. */
    NvU32                               bConstrainedEncodingSupported : 1;                      /**< [out]: Constrained encodeing support. */
    NvU32                               bStereoMVCSupported : 1;                                /**< [out]: Stereo multi view coding support. */
    NvU32                               bReservedBits : 25;                                     /**< [out]: Reserved bits. Set to 0. */
    NvU32                               reserved[32];                                           /**< [out]: Reserved. Set to 0. */
}NV_HW_ENC_GET_CAPS;
#define NV_HW_ENC_CAPS_VER NVSTRUCT_VERSION(NV_HW_ENC_GET_CAPS, 1)

#endif