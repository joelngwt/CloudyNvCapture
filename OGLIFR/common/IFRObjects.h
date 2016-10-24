/*!
 * \file
 *
 * Declaration of Classes IFRSessionObject and IFRTransferObject,
 * which helps to manage IFR sessions with different type of encoders.
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

#ifndef __IFR_OBJECTS_H__

#define __IFR_OBJECTS_H__

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <stdio.h>
#include <GL/gl.h>

#include "NvIFR/NvIFROpenGL.h"
#include "NvIFR_API.h"

/*!
 * SysConfig:
 *  IFR Object with SysConfig could just capture FBO rendering to System-Memory.
 *
 * H264Config:
 *  IFR Object with H264Config could capture and encode FBO rendering to H264 format.
 */
typedef NV_IFROGL_TO_SYS_CONFIG   SysConfig;
typedef NV_IFROGL_H264_ENC_CONFIG H264Config;

typedef struct SysEncodeParams_t
{
    NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAGS flags;
    unsigned int xOffset;
    unsigned int yOffset;
    unsigned int width;
    unsigned int height;
} SysEncodeParams;

typedef struct H264EncodeParams_t
{
    NV_IFROGL_H264_ENC_PARAMS *encodeParms;
    GLenum stereoAttachmentRight;
} H264EncodeParams;

class IFRSessionObject {
    friend class IFRTransferObject;

private:
    /*!
     * Handle to IFR session.
     */
    NV_IFROGL_SESSION_HANDLE m_handle;

public:
    IFRSessionObject(void);
    ~IFRSessionObject(void);

    /*!
     * Open IFR Session.
     */
    bool open(void);

    /*
     * Close IFR Session.
     */
    bool close(void);

    /*!
     * Returns 'TRUE' if IFRSessionObject is initialized.
     */
    inline bool initialize(void) const;
};

class IFRTransferObject {
private:
    /*!
     * IFRSessionObject to which this Transfer object belongs.
     */
    const IFRSessionObject *m_session;

    /*!
     * Enoder type used by Transfer Object.
     */
    enum {
        InvalidEncoder,
        SysEncoder,
        H264Encoder
    } m_encoderType;

    /*!
     * Encoder configuration.
     */
    union {
        SysConfig   sys;
        H264Config  h264;
    } m_encoderConfig;

    /*!
     * Provided Transfer Parameters.
     */
    GLuint m_framebuffer;
    GLenum m_attachment;
    union {
        SysEncodeParams  sys;
        H264EncodeParams h264;
    } m_encoderParams;

    /*!
     * IFR Transfer object handle.
     */
    NV_IFROGL_TRANSFEROBJECT_HANDLE m_handle;

    /*!
     * Data size and pointer valid for reading,
     * when transfer object is locked.
     */
    const void *m_data;
    uintptr_t m_dataSize;

public:
    IFRTransferObject(const IFRSessionObject *session);
    ~IFRTransferObject(void);


    /*!
     * Set Encoding configuration with SysMem Encoder.
     */
    bool setEncodeConfig(SysConfig config);

    /*!
     * Set Encoding configuration with H264 Encoder.
     */
    bool setEncodeConfig(H264Config config);

    /*!
     * Unset Encoder configuration.
     */
    bool unsetEncodeConfig(void);

    /*!
     * Returns 'TRUE' if IFRObject is initialized.
     */
    inline bool initialize(void) const;

    /*!
     * Set Encoding parameters, returns 'TRUE' after success otherwise 'FALSE'
     */
    bool transferEncodeParams(GLuint framebuffer, GLenum attachment,
                              SysEncodeParams params);
    bool transferEncodeParams(GLuint framebuffer, GLenum attachment,
                              H264EncodeParams params);

    /*!
     * Lock IFR object, so that you can read encoded data from it.
     */
    bool lock(void);
    /*!
     * Release IFR object, after this encoded data read from IFR object is not valid.
     */
    bool release(void);

    /*!
     * Write encoded data to file, You must need to lock IFR object before calling this function.
     */
    void writeToFile(FILE *fp);
};

/*!
 * Initialize IFR Apis, this function is not thread safe.
 */
bool initilizeIFRApi(void);

#endif
