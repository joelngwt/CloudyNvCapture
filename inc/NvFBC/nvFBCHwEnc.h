/**
 * \file This file contains definitions for NVFBCHWEncoder.
 *
 * Copyright 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 *
 */


#ifndef _NVFBC_HW_ENC_
#define _NVFBC_HW_ENC_

#include "NvFBC.h"
#include "NvHWEnc.h"

/**
 * \ingroup NVFBC_TOSYS
 * \brief Macro to define the interface ID to be passed as NvFBCCreateParams::dwInterfaceType for creating an NVFBCHWEncoder capture session object.
 */
#define NVFBC_TO_HW_ENCODER (0x1405)

#pragma message("NvFBCHWEnc interface is deprecated, and will not be available from next release.")
#pragma message("Please use NvFBCToDx9Vid for capture and NVidia Video Codec SDK for HW Encode.")
#pragma deprecated(INvFBCHWEncoder)

/**
 * \ingroup NVFBC_HW_ENC_ENUMS
 * \brief Enumerates special commands for grab\capture supported by NVFBCHWEncoder interface.
 */
typedef enum _NVFBC_HW_ENC_FLAGS
{
    NVFBC_HW_ENC_NOFLAGS            = 0x0,                              /**< Default (no flags set). Grabbing will wait for a new frame or HW mouse move. */
    NVFBC_HW_ENC_NOWAIT             = 0x1,                              /**< Grabbing will not wait for a new frame nor a HW mouse move. */
    NVFBC_HW_ENC_RESERVED           = 0x2,                              /**< Reserved. Do not use. */
    NVFBC_HW_ENC_WAIT_WITH_TIMEOUT  = 0x10,                             /**< Grabbing will wait for a new frame or HW mouse move with a maximum wait time of NVFBC_HW_ENC_GRAB_FRAME_PARAMS::dwWaitTime millisecond*/   
} NVFBC_HW_ENC_FLAGS;

/**
 * \ingroup NVFBC_HW_ENC_ENUMS
 * \brief Enumerates Capture\Grab modes supported by NVFBCHWEncoder interface.
 */
typedef enum _NVFBC_HW_ENC_GRAB_MODE
{
    NVFBC_HW_ENC_SOURCEMODE_FULL = 0,                                   /**< Grab at full desktop resolution. */
    NVFBC_HW_ENC_SOURCEMODE_SCALE,                                      /**< Scale the grabbed image according to NVFBC_HW_ENC_GRAB_FRAME_PARAMS::dwTargetWidth
                                                                             and NVFBC_HW_ENC_GRAB_FRAME_PARAMS::dwTargetHeight. */
    NVFBC_HW_ENC_SOURCEMODE_CROP,                                       /**< Crop a subwindow without scaling, of NVFBC_HW_ENC_GRAB_FRAME_PARAMS::dwTargetWidth
                                                                             and NVFBC_HW_ENC_GRAB_FRAME_PARAMS::dwTargetHeight sizes, 
                                                                             starting at NVFBC_HW_ENC_GRAB_FRAME_PARAMS::dwStartX and NVFBC_HW_ENC_GRAB_FRAME_PARAMS::dwStartY. */
    NVFBC_HW_ENC_SOURCEMODE_LAST,                                       /**< Sentinel value. Do not use. */
} NVFBC_HW_ENC_GRAB_MODE;

/**
 * \ingroup NVFBC_HW_ENC_STRUCTS
 * \brief Defines parameters used to configure NVFBCHWEncoder capture session.
 */
typedef struct _NVFBC_HW_ENC_SETUP_PARAMS
{
    NvU32 dwVersion;                                                    /**< [in]: Struct version. Set this to NVFBC_HW_ENC_SETUP_PARAMS_VER. */
    NvU32 bWithHWCursor : 1;                                            /**< [in]: Set this to 1 for capturing mouse cursor with each grab. */
#pragma message("Custom YUV444 is deprecated and will be removed in future versions of GRID SDK")
#pragma deprecated(bIsCustomYUV444)
    NvU32 bIsCustomYUV444:1;                                                            /**< [out]: NvFBC will set this to 1 if the client has requested YUV444 capture,
                                                                                                     and NvFBC needed to use custom YUV444 representation.
                                                                                                     only supported for H.264 encode on Kepler-architecture GPUs.  */
    
    NvU32 bEnableSeparateCursorCapture : 1;                             /**< [in]: The client should set this to 1 if it wants to enable mouse capture separately from Grab().*/
    NvU32 bReservedBits : 29;                                           /**< [in]: Reserved. Set to 0. */
    NV_HW_ENC_CONFIG_PARAMS EncodeConfig;                               /**< [in]: HW Encoder initial configuration parameters.       */
    NvU32 dwBSMaxSize;                                                  /**< [in]: Specifies maximum bitstream buffer size in bytes. */
    NvU32 dwReserved1;                                                  /**< [in]: Reserved. Set to 0. */
    void *hCursorCaptureEvent;                                          /**< [out]: Event handle to be signalled when there is an update to the HW cursor state. */
    NvU32 dwReserved[60];                                               /**< [in]: Reserved. Set to 0. */
    void *pReserved[30];                                                /**< [in]: Reserved. Set to NULL. */
} NVFBC_HW_ENC_SETUP_PARAMS;
#define NVFBC_HW_ENC_SETUP_PARAMS_VER NVFBC_STRUCT_VERSION(NVFBC_HW_ENC_SETUP_PARAMS, 1)

/**
 * \ingroup NVFBC_HW_ENC_STRUCTS
 * \brief Defines parameters for a Grab\Capture call in the NVFBCHWEncoder capture session.
 * Also holds information regarding the grabbed data.
 */
typedef struct _NVFBC_HW_ENC_GRAB_FRAME_PARAMS
{
    NvU32 dwVersion;                                                    /**< [in]: Struct version. Set to NVFBC_HW_ENC_GRAB_FRAME_PARAMS_VER.*/
    NvU32 dwFlags;                                                      /**< [in]: Set this to one of NVFBC_HW_ENC_FLAGS values. Default = NVFBC_HW_ENC_NOFLAGS. */
    NVFBC_HW_ENC_GRAB_MODE eGMode;                                      /**< [in]: Set this to one of NVFBC_HW_ENC_GRAB_MODE values. Default = NVFBC_HW_ENC_SOURCEMODE_FULL. */
    NvU32 dwTargetWidth;                                                /**< [in]: Set this to indicate the target width in case of eGMode = NVFBC_HW_ENC_SOURCEMODE_SCALE and in case of eGMode = NVFBC_HW_ENC_SOURCEMODE_CROP. */
    NvU32 dwTargetHeight;                                               /**< [in]: Set this to indicate the target height in case of eGMode = NVFBC_HW_ENC_SOURCEMODE_SCALE and in case of eGMode = NVFBC_HW_ENC_SOURCEMODE_CROP. */
    NvU32 dwStartX;                                                     /**< [in]: Set this to indicate the starting x-coordinate of cropping rect in case of in case of eGMode = NVFBC_HW_ENC_SOURCEMODE_CROP. Cropping rect = {dwStartX, dwStartY, dwStartX+dwTargetWidth, dwStartY+dwTargetHeight} */
    NvU32 dwStartY;                                                     /**< [in]: Set this to indicate the starting y-coordinate of cropping rect in case of in case of eGMode = NVFBC_HW_ENC_SOURCEMODE_CROP. Cropping rect = {dwStartX, dwStartY, dwStartX+dwTargetWidth, dwStartY+dwTargetHeight} */
    NvU32 dwWaitTime;                                                   /**< [in]: Time limit in millisecond to wait for a new frame or a HW mouse move. Use with NvFBC_HW_ENC_WAIT_WITH_TIMEOUT */
    NvFBCFrameGrabInfo NvFBCFrameGrabInfo;                              /**< [in/out]: NvFBCFrameGrabInfo struct, used to communicate miscellaneous information. */
    NvU8* pBitStreamBuffer;                                             /**< [out]: Pointer to a buffer, provided by the client. NvFBC will write encoded bitstream data in the buffer.*/
    NV_HW_ENC_PIC_PARAMS  EncodeParams;                                 /**< [in]: Structure containing per-frame configuration paramters for the HW encoder. */
    NV_HW_ENC_GET_BIT_STREAM_PARAMS GetBitStreamParams;                 /**< [out]: Structure containing data about the captured frame. The client allocates this struct. */
    NvU32 dwFrameInfoVer;                                               /**< [in]: Struct version for NV_HW_ENC_GET_BIT_STREAM_PARAMS. Set to NV_HW_ENC_GET_BIT_STREAM_PARAMS_VER.*/
    NvU32 dwMSE[3];                                                     /**< [out]: Mean square error used to evaluate Quality. Set flag in  NV_HW_ENC_CONFIG_PARAMS to enable getting MSE values per channel(Y, U, V) */
    NvU32 dwReserved[52];                                               /**< [in]: Reserved. Set to 0. */
    void *pReserved[28];                                                /**< [in]: Reserved. Set to NULL. */
} NVFBC_HW_ENC_GRAB_FRAME_PARAMS;
#define NVFBC_HW_ENC_GRAB_FRAME_PARAMS_VER NVFBC_STRUCT_VERSION(NVFBC_HW_ENC_GRAB_FRAME_PARAMS, 1)

/**
 * \ingroup NVFBC_HW_ENC_STRUCTS
 * \brief Defines parameters for fetching Sequence Parameter Header and Picture Parameter Header from NVFBCHWEncoder.
 */
typedef struct _NVFBC_HW_ENC_GET_STREAM_HEADER_PARAMS
{
    NvU32 dwVersion;                                                    /**< [in]: Struct version. Set to NVFBC_HW_ENC_GET_STREAM_HEADER_PARAMS_VER. */
    NvU32 dwReserved1;                                                  /**< [in]: Reserved. Set to 0. */
    NvU32 dwSize;                                                       /**< [out]: Contains size of the SPS\PPs header data written by the HW encoder. */
    NvU8  *pBuffer;                                                     /**< [out]: Pointer to a client allocated buffer. NvFBC HW encoder writes SPS\PPS data to this buffer. */
    NvU32 dwReserved[62];                                               /**< [in]: Reserved. Set to 0. */
    void *pReserved[30];                                                /**< [in]: Reserved. Set to NULL. */
} NVFBC_HW_ENC_GET_STREAM_HEADER_PARAMS;
#define NVFBC_HW_ENC_GET_STREAM_HEADER_PARAMS_VER NVFBC_STRUCT_VERSION(NVFBC_HW_ENC_GET_STREAM_HEADER_PARAMS, 1)

/**
 * \ingroup NVFBC_HW_ENC_INTERFACE
 * \brief Interface class definition for NvFBCHWEncoder capture API
 *  Replaces NVFBCToH264HWEncoder API, enables scope to extend encoding support to codecs other than H.264
 */
class INvFBCHWEncoder_v2
{
    /* First Release */
public:
    /**
    * \brief Queries capabilities of the NVIDIA HW Encoder.
    * \param [inout] pGetCaps Pointer to a struct of type ::NV_HW_ENC_GET_CAPS
    * \return An applicable ::NVFBCRESULT value.
    */
    virtual NVFBCRESULT NVFBCAPI NvFBCGetHWEncCaps(NV_HW_ENC_GET_CAPS* pGetCaps) = 0;

    /**
     * \brief Set up NVFBCHWEncoder capture and video encoder according to the provided parameters.
     *  Allocates internal resources and configures the NVIDIA HW Encoder.
     * \param [in] pSetUpParam Pointer to a struct of type ::NVFBC_HW_ENC_SETUP_PARAMS
     * \return An applicable ::NVFBCRESULT value.
     */
    virtual NVFBCRESULT NVFBCAPI NvFBCHWEncSetUp(NVFBC_HW_ENC_SETUP_PARAMS *pSetUpParam) = 0;

     /**
     * \brief Captures the desktop and generates a video bitstream packet.
     *  If the API returns a failure, the client should check the return codes and ::NvFBCFrameGrabInfo output fields to determine if the session needs to be re-created.
     * \param [in] pGrabFrameParam Pointer to a struct of type ::NVFBC_HW_ENC_GRAB_FRAME_PARAMS
     * \return An applicable ::NVFBCRESULT value.
     */
    virtual NVFBCRESULT NVFBCAPI NvFBCHWEncGrabFrame(NVFBC_HW_ENC_GRAB_FRAME_PARAMS *pGrabFrameParam) = 0;

    /**
    * \brief Fetches Sequence Parameter and Picture Parameter set header.
    * \param [inout] pGetStreamHeaderParams Pointer to a struct of type ::NVFBC_HW_ENC_GET_STREAM_HEADER_PARAMS
    */
    virtual NVFBCRESULT NVFBCAPI NvFBCHWEncGetStreamHeader(NVFBC_HW_ENC_GET_STREAM_HEADER_PARAMS *pGetStreamHeaderParams) = 0;

    /**
     * \brief Destroys the NVFBCHWEncoder capture session.
     * \return An applicable ::NVFBCRESULT value.
     */
    virtual NVFBCRESULT NVFBCAPI NvFBCHWEncRelease() = 0;

    /**
    * \brief Captures HW cursor data whenever shape of mouse is changed
    * \param [inout] pParam Pointer to a struct of type ::NVFBC_TOSYS_GRAB_MOUSE_PARAMS.
    * \return An applicable ::NVFBCRESULT value.
    */
    virtual NVFBCRESULT NVFBCAPI NvFBCHWEncCursorCapture(NVFBC_CURSOR_CAPTURE_PARAMS *pParam) = 0;
};

typedef INvFBCHWEncoder_v2 INvFBCHWEncoder;

/**
 * @} NVFBC_HW_ENC_INTERFACE
 */

#endif // _NVFBC_HW_ENC_H_
