/*!
 * \file
 *
 * Header file for the EncodingThread class.
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

#ifndef ENCODING_THREAD_H
#define ENCODING_THREAD_H

#include "GL/glew.h"

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
  #include "GL/wglew.h"
  #include "GL/wglext.h"
#else
  #include "GL/glew.h"
  #include "GL/glext.h"
#endif

#include "NvIFR_API.h"

#include "Thread.h"
#include "Event.h"

//! Represents an encoding thread running on a GPU.
class EncodingThread
{
private:
    //! thread object for managing a thread.
    Thread m_thread;

    //! width of the encoded stream
    unsigned int m_width;
    //! height of the encoded stream
    unsigned int m_height;

    //! GPU handle for the GPU the thread runs on
    HGPUNV m_hGPU;
    //! Index of the device the thread runs on
    unsigned int m_deviceIndex;
    //! Encoding instance on this devices
    unsigned int m_instance;
    //! String decribing the device the thread runs on
    CHAR m_deviceString[128]; 
    //! Device context handle for the GPU the thread runs on
    HDC m_hDC;
    //! OpenGL render context handle for the GPU the thread runs on
    HGLRC m_hRC;

    //! the NvIFR API function list
    NvIFRAPI m_nvIFR;
    //! the NvIFR session handle
    NV_IFROGL_SESSION_HANDLE m_sessionHandle;
    //! the NvIFR transfer object handle
    NV_IFROGL_TRANSFEROBJECT_HANDLE m_transferObjectHandle;

    //! OpenGL object ID's
    GLuint m_fboID, m_texID[2], m_vboID, m_iboID;

    //! Encoding frame rate
    float m_frameRate;
    
    //! if signalled the thread had finished starting up
    Event m_startedUp;

    //! will be set by the main thread when the worker thread needs to be terminated
    volatile bool m_terminateThread;

	NV_IFROGL_HW_ENC_TYPE m_eCodec;

    bool initOpenGL();
    bool cleanupOpenGL();
    bool initNvIFR();
    bool cleanupNvIFR();

public:
    //! next thread in the list
    EncodingThread *m_next;

    EncodingThread(unsigned int width, unsigned int height, unsigned int deviceIndex, HGPUNV hGPU, unsigned int instance, NV_IFROGL_HW_ENC_TYPE eCodec);

    unsigned int getDeviceIndex() const
    {
        return m_deviceIndex;
    }

    unsigned int getDeviceInstance() const
    {
        return m_instance;
    }

    const char *getDeviceString() const
    {
        return m_deviceString;
    }

    const float getFrameRate() const
    {
        return m_frameRate;
    }

    bool threadProc();

    bool startup();
    bool terminate();
};

#endif // ENCODING_THREAD_H
