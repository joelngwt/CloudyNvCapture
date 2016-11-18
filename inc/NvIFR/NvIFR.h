/**
 * \file
 *
 * This file contains the interface constants, structure definitions and
 * function prototypes defining the NvIFR API.
 *
 * Copyright 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 *
 */


/**
 * \mainpage NVIDIA Inband Frame Readback (NVIFR).
 * NVIFR is a high performance, low latency API to capture and optionally compress
 * framebuffer. Both named (non-zero) framebuffers and the window-system provided
 * an individual DirectX RenderTarget. The output from NvIFR does not include any 
 * window decoration, composited overlay, cursor or taskbar; it provides solely the
 * pixels rendered into the RenderTarget, as soon as their rendering is
 * complete, ahead of any compositing that may be done by the windows manager.
 * In fact, NvIFR does not require that the RenderTarget even be
 * visible on the Windows desktop. It is ideally suited to application capture
 * and remoting, where the output of a single application, rather than the
 * entire desktop environment, is captured.
 *
 * NvIFR is intended to operate inband with a rendering application,
 * either as part of the application itself, or as part of a shim layer operating
 * immediately below the application. Like NvFBC, NvIFR operates
 * asynchronously to graphics rendering, using dedicated hardware compression
 * and copy engines in the GPU, and delivering pixel data to system memory with
 * minimal impact on rendering performance.
 */

#pragma once

typedef unsigned long NvU32;
typedef unsigned long long NvU64;
typedef unsigned char NvU8;

/**
 * \defgroup NVIFR NVIDIA Inband Frame Readback
 * \brief Defines the NVIDIA Inbanc Frame Reasback APIs
 */

/**
 * \defgroup NVIFR_ENUMS Enumerations defined for NVIFR
 * \ingroup NVIFR
 * \brief Defines Enumerations for use with NVIFR
 */

/**
 * \defgroup NVIFR_STRUCTS NVIFR API Parameter Structure definition
 * \ingroup NVIFR
 */

/**
 * \defgroup NVIFR_SURFACE_SHARING NVIFR D3D9 Surface Sharing Extension
 * \ingroup NVIFR
 * \brief Defines Surface sharing extensions for use with Dx9 devices only.
 */

/** \defgroup NVIFR_FUNCTIONS DLL Entry points for NVIFR
 * \ingroup NVIFR
 */

/**
 * \ingroup NVIFR
 * NVIFR Interface version
 */
#define NVIFR_VERSION 0x40

/**
 * \ingroup NVIFR
 * Maximum number of reference frames supported by the HW encoder.
 */
#define NVIFR_MAX_REF_FRAMES 0x10

/**
 * \ingroup NVIFR
 * Maximum number of output buffers that can be used with NVIFR
 */
#define MAX_N_BUFFERS 3

/**
 * \ingroup NVIFR
 * Macro to generate version number of parameter structs passed to NVIFR APIs.
 */
#define NVIFR_STRUCT_VERSION(typeName, ver) (NvU32)(sizeof(typeName) | ((ver)<<16) | (NVIFR_VERSION << 24))

/**
 * \ingroup NVIFR_ENUMS
 * Enumerates NVIFR status codes
 */
typedef enum
{
    NVIFR_SUCCESS = 0,                                      /**< Indicates that the API call completed succesfully. */
    NVIFR_ERROR_GENERIC = -1,                               /**< Indicates that an undefined error condition was encountered. */
    NVIFR_ERROR_INVALID_PTR = -2,                           /**< Indicates that a pointer passed to NVIFR was NULL. */
    NVIFR_ERROR_INVALID_PARAM = -3,                         /**< Indicates that a parameter value passed to NVIFR is invalid. */
    NVIFR_ERROR_DX_DRIVER_FAILURE = -4,                     /**< Indicates that the NVIDIA driver encountered an error. */
    NVIFR_ERROR_HW_ENC_FAILURE = -5,                        /**< Indicates that the NVIDIA HW Encoder API returned an error. */
    NVIFR_ERROR_HW_ENC_NOT_INITIALIZED = -6,                /**< Indicates that the NVIDIA HW Encoder was not initialized. */
    NVIFR_ERROR_FILE_NOT_FOUND = -7,                        /**< Indicates that a required resource could not be located. */
    NVIFR_ERROR_OUT_OF_MEMORY = -8,                         /**< Indicates that a resource could not be allocated. */
    NVIFR_ERROR_INVALID_CALL = -9,                          /**< Indicates that an API was called in an incorrect sequence. */
    NVIFR_ERROR_UNSUPPORTED_INTERFACE = -10,                /**< Indicates that the NVIFR interface requested is not supported. */
    NVIFR_ERROR_NOT_SUPPORTED = -11,                        /**< Indicates that an unsupported operation was attempted. */
    NVIFR_ERROR_UNSUPPORTED_D3D_DEVICE = -12,               /**< Indicates that NVIFR does not support this version of DirectX. */
    NVIFR_ERROR_NVAPI_FAILURE = -13,                        /**< Indicates that NVAPI encountered an error. */
    NVIFR_ERROR_INCOMPATIBLE_VERSION = -14,                 /**< Indicates that an API was called with a parameter struct having an incompatible version. */
    NVIFR_ERROR_CPSS_SERVER_OFFLINE = -15,                  /**< Indicates that the NVIFR Cross-Process Surface Sharing server is offline. */
    NVIFR_ERROR_CONCURRENT_SESSION_LIMIT_EXCEEDED = -16,    /**< Indicates that maximum concurrent sessions limit for NVIFR has been reached, 
                                                                 Client should release an existing session to create a new one.*/
    NVIFR_ERROR_UNSUPPORTED_PLATFORM = -17,                 /**< Indicates that NVIFR sessions can not be created on this system configuration. */
} NVIFRRESULT;
#define NVIFR_ERROR_UNSUPPORTED_DEVICE NVIFR_ERROR_UNSUPPORTED_D3D_DEVICE /**< Indicates that NVIFR does not support this version of DirectX. */

/**
 * \ingroup NVIFR_ENUMS
 * Output buffer formats supported by NVIFRToSys interface
 */
typedef enum _NVIFR_BUFFER_FORMAT
{
    NVIFR_FORMAT_ARGB      = 0,      /**< ARGB (32bit storage per pixel). */
    NVIFR_FORMAT_RGB          ,      /**< RGB (24bit storage per pixel). */
    NVIFR_FORMAT_YUV_420      ,      /**< YYYYUV, UV Subsamples by factor of 2 in hieght and width. */
    NVIFR_FORMAT_RGB_PLANAR   ,      /**< RGB planar, RRRRRRRR GGGGGGGG BBBBBBBB. */
    NVIFR_FORMAT_RESERVED     ,      /**< Reserved format, should not be used. */
    NVIFR_FORMAT_YUV_444      ,      /**< YUV planar, YYYYYYYY UUUUUUUU VVVVVVVV. */
    NVIFR_FORMAT_LAST         ,      /**< Sentinel enum value. Not to be used. */
}NVIFR_BUFFER_FORMAT;

/**
 * \ingroup NVIFR_STRUCTS
 * Defines the parameters that should be passed to NvIFR_CreateEx API.
 */
typedef struct _NVIFR_CREATE_PARAMS
{
    NvU32   dwVersion;              /**< [in]  Struct version. Set to NVIFR_CREATE_PARAMS_VER.*/
    NvU32   dwInterfaceType;        /**< [in]  NvIFR Interface to be instantiated.*/
    void*   pDevice;                /**< [in]  DirectX device to be used for capturing RenderTarget data .*/
    void*   pPrivateData;           /**< [in]  Reserved. */
    NvU32   dwPrivateDataSize;      /**< [in]  Reserved. */
    NvU32   dwNvIFRVersion;         /**< [out] Highest NvIFR API version supported by underlying NvIFR driver. */
    void*   pNvIFR;                 /**< [out] Pointer to the instantiated NvIFR object. */
    NVIFRRESULT result;             /**< [out] Status code for the NvIFR object instantiation. */
    NvU32   dwReserved[27];         /**< [in]  Reserved. Set to 0. */
    void*   pReserved[29];          /**< [in]  Reserved. Set to NULL. */
} NVIFR_CREATE_PARAMS;
#define NVIFR_CREATE_PARAMS_VER NVIFR_STRUCT_VERSION(NVIFR_CREATE_PARAMS, 1); /**< Current Version of NVIFR_CREATE_PARAMS structure. */

/** 
 * \ingroup NVIFR_FUNCTIONS
 * \brief Type definition for NVIFR Entry point to create an NVIFR capture session object.
 * \param [inout] pParams Pointer to NVIFR_CREATE_PARAMS struct
 * \return An applicable NVIFRRESULT value.
 */
NVIFRRESULT __stdcall NvIFR_CreateEx(void *pParams);

/** 
 * \ingroup NVIFR_FUNCTIONS
 * \brief Type definition for NVIFR Entry point to query the latest NVIFR version supported by the loaded NVIFR library.
 * \param [out] pVersion A pointer to an 32bit unsigned integer value.
 * \return An applicable NVIFRRESULT value.
 */
NVIFRRESULT __stdcall NvIFR_GetSDKVersion(NvU32* pVersion);

#if DIRECT3D_VERSION  == 0x0900

/**
 * \ingroup NVIFR_SURFACE_SHARING 
 * Handle to identify an NVIFR shared surface.
 */
typedef void * IFRSharedSurfaceHandle; 

/**
 * \ingroup NVIFR_FUNCTIONS
 * Creates a Shared surface to share data between D3D9 devices
 * \param [in] pDevice A IDirect3DDevice9 * device object
 * \param [in] dwWidth Width of the shared surface to be created.
 * \param [in] dwHeight Height of the shared surface to be created.
 * \param [out]Holds the handle to the Shared Surface that will be created
 * 
 * \return TRUE if the call succeeds, FALSE otherwise.
 */
BOOL __stdcall NvIFR_CreateSharedSurfaceEXT (IDirect3DDevice9 * pDevice, DWORD dwWidth, DWORD dwHeight, IFRSharedSurfaceHandle * phIFRSharedSurface);

/**
 * \ingroup NVIFR_FUNCTIONS
 * Destroys the Shared Surface
 * \param [in] pDevice The IDirect3DDevice9* device object that was used to create the Shared Surface that is to be destroyed.
 * \param [in] hIFRSharedSurface Handle to the Shared Surface that is to be destroyed.
 * \return TRUE if the call succeeds, FALSE otherwise.
 */
BOOL __stdcall NvIFR_DestroySharedSurfaceEXT (IDirect3DDevice9 * pDevice, IFRSharedSurfaceHandle hIFRSharedSurface);

/**
 * \ingroup NVIFR_FUNCTIONS
 * Copy data from a D3D9 surface to the Shared Surface
 * \param [in] pDevice The IDirect3DDevice9 object that was used to create the source D3D9 Surface.
 * \param [in] hIFRSharedSurface Handle to the destination Shared Surface.
 * \param [in] psurface Source D3D9 Surface.
 * \return TRUE if the call succeeds, FALSE otherwise.
 */
BOOL __stdcall NvIFR_CopyToSharedSurfaceEXT (IDirect3DDevice9 * pDevice, IFRSharedSurfaceHandle hIFRSharedSurface, IDirect3DSurface9 * pSurface);

/**
 * \ingroup NVIFR_FUNCTIONS
 * Copy data from Shared Surface to a D3D9 surface
 * \param [in] pDevice The IDirect3DDevice9 object that was used to create the destination D3D9 surface.
 * \param [in] hIFRSharedSurface Handle to the source Shared Surface
 * \param [in] pSurface Destination D3D9 surface
 * \return TRUE if the call succeeds, FALSE otherwise.
 */
BOOL __stdcall NvIFR_CopyFromSharedSurfaceEXT (IDirect3DDevice9 * pDevice, IFRSharedSurfaceHandle hIFRSharedSurface, IDirect3DSurface9 * pSurface);


/**
 * \cond API_PFN
 */
typedef BOOL (__stdcall * NvIFR_CreateSharedSurfaceEXTFunctionType) (IDirect3DDevice9 * pDevice, NvU32 dwWidth, NvU32 dwHeight, IFRSharedSurfaceHandle * phIFRSharedSurface);
typedef BOOL (__stdcall * NvIFR_DestroySharedSurfaceEXTFunctionType) (IDirect3DDevice9 * pDevice, IFRSharedSurfaceHandle hIFRSharedSurface);
typedef BOOL (__stdcall * NvIFR_CopyToSharedSurfaceEXTFunctionType) (IDirect3DDevice9 * pDevice, IFRSharedSurfaceHandle hIFRSharedSurface, IDirect3DSurface9 * pSurface);
typedef BOOL (__stdcall * NvIFR_CopyFromSharedSurfaceEXTFunctionType) (IDirect3DDevice9 * pDevice, IFRSharedSurfaceHandle hIFRSharedSurface, IDirect3DSurface9 * pSurface);
/**
 * \endcond _API_PFN
 */
#endif

/**
 * \cond API_PFN
 */
typedef NVIFRRESULT (__stdcall * NvIFR_CreateFunctionExType) (void *pParams);
typedef NVIFRRESULT (__stdcall * NvIFR_GetSDKVersionFunctionType) (NvU32 * pVersion);
/**
 * \endcond _API_PFN
 */