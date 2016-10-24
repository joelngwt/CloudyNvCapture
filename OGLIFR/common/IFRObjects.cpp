/*!!
 * \file
 *
 * Implementation of IFRSessionObject anf IFRTransferObject.
 *
 * \copyright
 * Copyright 2013-2014 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO LICENSEE:
 *
 * This source code and/or documentation ("Licensed Deliverables") are
 * subject to the applicable NVIDIA license agreement that governs the
 * use of the Licensed Deliverables.
 */

#include "IFRObjects.h"
#include "Helper.h"

static NvIFRAPI nvIFR;
volatile static bool initialized = false;

/*!
 * Initialize IFR Apis.
 */
bool initilizeIFRApi(void)
{
    if (nvIFR.initialize())
    {
        initialized = true;
        printf("NvIFROpenGL API version number: %d.%d\n\n", ENCODEAPI_MAJOR(nvIFR), ENCODEAPI_MINOR(nvIFR));
    }
    else
    {
        initialized = false;
    }

    return initialized;
}

/*!
 * IFRSessionObject constructor.
 */
IFRSessionObject::IFRSessionObject(void) :
    m_handle(NULL)
{
}

/*!
 * IFRSessionObject destructor.
 */
IFRSessionObject::~IFRSessionObject(void)
{
    close();
}

/*!
 * Open IFR Session.
 */

bool IFRSessionObject::open(void)
{
    NV_IFROGL_SESSION_HANDLE sessionHandle;

    if (!initialized)
    {
        PRINT_ERROR("IFR Apis are not initialized\n");
        return false;
    }

    if (nvIFR.nvIFROGLCreateSession(&sessionHandle, NULL) !=
        NV_IFROGL_SUCCESS)
    {
        PRINT_ERROR("Failed to create NvIFR session");
        return false;
    }

    m_handle = sessionHandle;

    return true;
}

/*!
 * Close FIR Session.
 */
bool IFRSessionObject::close(void)
{
    NVIFROGLSTATUS status = NV_IFROGL_FAILURE;

    if (!initialize())
    {
        return true;
    }

    status = nvIFR.nvIFROGLDestroySession(m_handle);

    if (status != NV_IFROGL_SUCCESS)
    {
        PRINT_ERROR("Failed to destroy encoding session\n");
        return false;
    }

    m_handle = NULL;

    return true;
}

/*!
 * Returns true if session is valid.
 */
inline bool IFRSessionObject::initialize(void) const
{
    return (m_handle != NULL);
}

/*!
 * IFRTransferObject constructor.
 */
IFRTransferObject::IFRTransferObject(const IFRSessionObject *session) :
    m_session(session),
    m_encoderType(InvalidEncoder),
    m_handle(NULL)
{
}

/*!
 * IFRTransferObject destructor.
 */
IFRTransferObject::~IFRTransferObject(void)
{
    unsetEncodeConfig();
}

inline bool IFRTransferObject::initialize(void) const
{
    return (m_handle != NULL);
}

/*!
 * Set Encoding configuration with SysMem Encoder.
 */
bool IFRTransferObject::setEncodeConfig(SysConfig config)
{
    NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle = NULL;
    NVIFROGLSTATUS status = NV_IFROGL_FAILURE;

    if (m_session == NULL || !m_session->initialize())
    {
        PRINT_ERROR("IFRSession is closed\n");
        return false;
    }

    if (initialize()) {
        if (!unsetEncodeConfig()) {
            return false;
        }
    }

    status = nvIFR.nvIFROGLCreateTransferToSysObject(m_session->m_handle, &config,
                                                     &transferObjectHandle);

    if (status != NV_IFROGL_SUCCESS)
    {
        PRINT_ERROR("Failed to cretate transfer object\n");
        return false;
    }

    m_handle = transferObjectHandle;
    m_encoderType       = SysEncoder;
    m_encoderConfig.sys = config;

    memset(&(m_encoderParams), 0, sizeof(m_encoderParams));

    m_data = NULL;
    m_dataSize= 0;

    return true;
}

/*!
 * Set Encoding configuration with H264 Encoder.
 */
bool IFRTransferObject::setEncodeConfig(H264Config config)
{
    NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle = NULL;
    NVIFROGLSTATUS status = NV_IFROGL_FAILURE;

    if (m_session == NULL || !m_session->initialize())
    {
        PRINT_ERROR("IFRSession is closed\n");
        return false;
    }

    if (initialize()) {
        if (!unsetEncodeConfig()) {
            return false;
        }
    }

    status = nvIFR.nvIFROGLCreateTransferToHwEncObject(m_session->m_handle, &config,
                                                         &transferObjectHandle);

    if (status != NV_IFROGL_SUCCESS)
    {
        PRINT_ERROR("Failed to cretate transfer object\n");
        return false;
    }

    m_handle = transferObjectHandle;
    m_encoderType        = H264Encoder;
    m_encoderConfig.h264 = config;

    memset(&(m_encoderParams), 0, sizeof(m_encoderParams));

    m_data = NULL;
    m_dataSize= 0;

    return true;
}

/*!
 * Unset Encoder configuration.
 */
bool IFRTransferObject::unsetEncodeConfig(void)
{
    NVIFROGLSTATUS status = NV_IFROGL_FAILURE;

    if (!initialize())
    {
        return true;
    }

    status = nvIFR.nvIFROGLDestroyTransferObject(m_handle);

    if (status != NV_IFROGL_SUCCESS)
    {
        PRINT_ERROR("Failed to destroy transferobject\n");
        return false;
    }

    m_handle = NULL;

    return true;
}

/*!
 * Transfer encoding parameters for system encoder.
 */
bool IFRTransferObject::transferEncodeParams(GLuint framebuffer, GLenum attachment,
                                             SysEncodeParams params)
{
    const IFRSessionObject *session = m_session;
    NVIFROGLSTATUS status = NV_IFROGL_FAILURE;

    if (session == NULL || !session->initialize())
    {
        PRINT_ERROR("IFRSession is closed\n");
        return false;
    }

    if (!initialize())
    {
        PRINT_ERROR("IFRObject is not initialized\n");
        return false;
    }

    if (m_encoderType != SysEncoder)
    {
        PRINT_ERROR("Invalid encoder parameters");
        return false;
    }

    status = nvIFR.nvIFROGLTransferFramebufferToSys(m_handle, framebuffer, attachment,
                                                    params.flags, params.xOffset, params.yOffset,
                                                    params.width, params.height);

    if (status != NV_IFROGL_SUCCESS)
    {
        return false;
    }

    m_framebuffer = framebuffer;
    m_attachment  = attachment;
    m_encoderParams.sys = params;

    return true;
}

/*!
 * Transfer encoding parameters for hardware h264 encoder.
 */
bool IFRTransferObject::transferEncodeParams(GLuint framebuffer, GLenum attachment,
                                             H264EncodeParams params)
{
    const IFRSessionObject *session = m_session;
    NVIFROGLSTATUS status = NV_IFROGL_FAILURE;

    if (session == NULL || !session->initialize())
    {
        PRINT_ERROR("IFRSession is closed\n");
        return false;
    }

    if (!initialize())
    {
        PRINT_ERROR("IFRObject is not initialized\n");
        return false;
    }

    if (m_encoderType != H264Encoder)
    {
        PRINT_ERROR("Invalid encoder parameters");
        return false;
    }

    status = nvIFR.nvIFROGLTransferFramebufferToHwEnc(m_handle, params.encodeParms,
                                                        framebuffer, attachment,
                                                        params.stereoAttachmentRight);

    if (status != NV_IFROGL_SUCCESS)
    {
        return false;
    }

    m_framebuffer = framebuffer;
    m_attachment  = attachment;
    m_encoderParams.h264 = params;

    return true;
}

/*!
 * Lock encoder, so that we can read encoded data.
 */
bool IFRTransferObject::lock(void)
{
    NVIFROGLSTATUS status = NV_IFROGL_FAILURE;

    if (!initialize())
    {
        PRINT_ERROR("IFRObject is not initialized\n");
        return false;
    }

    if (m_encoderType == InvalidEncoder)
    {
        return false;
    }

    status = nvIFR.nvIFROGLLockTransferData(m_handle, &(m_dataSize),
                                            &(m_data));
    return status == NV_IFROGL_SUCCESS;
}

/*!
 * Release encoder.
 */
bool IFRTransferObject::release(void)
{
    NVIFROGLSTATUS status = NV_IFROGL_FAILURE;

    if (!initialize())
    {
        PRINT_ERROR("IFRObject is not initialized\n");
        return false;
    }

    if (m_encoderType == InvalidEncoder)
    {
        return false;
    }

    status = nvIFR.nvIFROGLReleaseTransferData(m_handle);

    if (status == NV_IFROGL_SUCCESS)
    {
        m_data = NULL;
        m_dataSize = 0;
    }

    return status == NV_IFROGL_SUCCESS;
}


/*!
 * Write encoded data into file.
 */
void IFRTransferObject::writeToFile(FILE *fp)
{
    if (!initialize())
    {
        PRINT_ERROR("IFRObject is not initialized\n");
        return;
    }

    if (m_encoderType == InvalidEncoder)
    {
        return;
    }

    if (m_data)
    {
        fwrite(m_data, 1, m_dataSize, fp);
    }
}

