/**
 * \file This file contains definitions for NVIFRToSys
 *
 * Copyright 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 *
 */

#ifndef _NVIFR_TO_SYS_H_
#define _NVIFR_TO_SYS_H_

#include "NvIFR.h"
/**
 * \defgroup NVIFR_TOSYS NVIFR Interface for readback to System Memory 
 * \brief Defines NVIFR interface for image readback in pagelocked system memory.
 */

/**
 * \defgroup NVIFR_TOSYS_ENUMS Enums
 * \ingroup NVIFR_TOSYS
 * \brief Enum definitions for use with NVIFRToSys API.
 */

/**
 * \defgroup NVIFR_TOSYS_STRUCTS Structs
 * \ingroup NVIFR_TOSYS
 * \brief Struct definitions for use with NVIFRToSys API.
 */

/**
 * \defgroup NVIFR_TOSYS_INTERFACE Object Interface
 * \ingroup NVIFR_TOSYS
 * \brief Interface class definition for NVFBCToSys Capture API
 */

/**
 * \ingroup NVIFR_TOSYS 
 * Interface identifier to be used with NvIFR_CreateEx to create an INvIFRH264HWEncoder object.
 */
#define NVIFR_TOSYS (0x2102)    

/**
 * \ingroup NVIFR_TOSYS_ENUMS
 * \brief Enumerates NVIFR Stereo output formats.
 * Currently unsupported.
 */
typedef enum
{
    NVIFR_SYS_STEREO_NONE           =      0,               /**< Capture in Mono-view format (no stereo). */
    NVIFR_SYS_STEREO_FULL_LR                ,               /**< Full frames captured (one for each view), returned independantly. */
    NVIFR_SYS_STEREO_LAST                   ,               /**< Sentinel value. Not to be used */
}NVIFR_SYS_STEREO_FORMAT;

/**
 * \ingroup NVIFR_TOSYS_STRUCTS
 * \brief Defines NVIFRToSys session configuration parameters.
 * To be used with INvIFRToSys::NvIFRSetupTargetBufferToSys
 */
typedef struct
{
    NvU32                   dwVersion;                      /**< [in]: Struct version. Set to NVIFR_TOSYS_SETUP_PARAMS_VER.*/
    NVIFR_BUFFER_FORMAT     eFormat;                        /**< [in]: Output Buffer format.*/
    NVIFR_SYS_STEREO_FORMAT eSysStereoFormat;               /**< [in]: Stereo capture format.*/
    NvU32                   dwNBuffers;                     /**< [in]: Number of output buffers to allocate.*/
    NvU8                  **ppPageLockedSysmemBuffers;      /**< [out]: Container for holding output buffer handles. NvIFR allocates the memory. */
    HANDLE                 *ppTransferCompletionEvents;     /**< [out]: Container for holding work completion events associated with the output buffers. NvIFR allocates the memory.*/
    NvU32                   dwTargetWidth;                  /**< [in]: Output Width.*/
    NvU32                   dwTargetHeight;                 /**< [in]: Output Height.*/
    NvU32                   dwReserved[58];                 /**< [in]: Reserved. Set to 0.*/
    NvU32                   pReserved[30];                  /**< [in]: Reserved. Set to NULL.*/
} NVIFR_TOSYS_SETUP_PARAMS;
#define NVIFR_TOSYS_SETUP_PARAMS_VER NVIFR_STRUCT_VERSION(NVIFR_TOSYS_SETUP_PARAMS, 1)

/**
 * \ingroup NVIFR_TOSYS_INTERFACE
 * \brief Interface Defintion for NVIFR System Memory Readbck Interface.
 * One instance of the INVIFRToSys interface object corrsponds to one NVIFRToSys session.
 * Excatly one session per D3D device is supported.
 */
class INvIFRToSys_v1
{
public:
    /**
     * \brief NVIFR API for configuring the INvIFRToSys capture session.
     * Sets up the output buffers in pagelocked system memory, and also some internal resources.
     * \param [inout] pParams Pointer to ::NVIFR_TOSYS_SETUP_PARAMS structure.
     * \return An applicable ::NVIFRRESULT value.
     */
    virtual NVIFRRESULT NvIFRSetUpTargetBufferToSys(NVIFR_TOSYS_SETUP_PARAMS *pParams)=0;

    /**
     * \brief NVIFR API to capture RenderTarget data and copy it to a systen memory buffer.
     * The output buffer is one of the buffers allocated using INvIFRToSys::NvIFRSetupTargetBufferToSys
     * and is identified using the dwBufferIndex parameter.
     * \param [in] dwBufferIndex Index of the output buffer to be used for copying the RenderTarget data.
     * \return An applicable ::NVIFRRESULT value.
     */
    virtual NVIFRRESULT NvIFRTransferRenderTargetToSys(NvU32 dwBufferIndex)=0;

    /**
     * \brief NVIFR API to destroy the NVIFR capture session.
     * \return  An applicable ::NVIFRRESULT value.
     */
    virtual NVIFRRESULT NvIFRRelease() = 0;
};

typedef INvIFRToSys_v1 NvIFRToSys;

#endif // _NVIFR_TO_SYS_H_
