/*!
 * \file
 *
 * Implementation EncodingThread class.
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

#include <assert.h>
#include <stdio.h>
#include "encodingThread.h"

#include "Util.h"
#include "Timer.h"

/*!
 * The vertex data for a colored cube. Position data is interleaved with
 * color data.
 */
static const GLfloat vertexData[] =
{
    // left-bottom-front
    -1.f, -1.f, -1.f,
     0.f,  0.f,  1.f,

    // right-bottom-front
     1.f, -1.f, -1.f,
     0.f,  0.f,  0.f,

    // right-top-front
     1.f,  1.f, -1.f,
     0.f,  1.f,  1.f,

    // left-top-front
    -1.f,  1.f, -1.f,
     0.f,  1.f,  0.f,

    // left-bottom-back
    -1.f, -1.f,  1.f,
     1.f,  0.f,  1.f,

    // right-bottom-back
     1.f, -1.f,  1.f,
     1.f,  0.f,  0.f,

    // right-top-back
     1.f,  1.f,  1.f,
     1.f,  1.f,  1.f,

    // left-top-back
    -1.f,  1.f,  1.f,
     1.f,  1.f,  0.f,
};

/*!
 * The index data for a cube drawn using GL_QUADS.
 */
static const GLushort indexData[] =
{
     0, 1, 2, 3,    // front
     1, 5, 6, 2,    // right
     3, 2, 6, 7,    // top
     4, 0, 3, 7,    // left
     4, 5, 1, 0,    // bottom
     5, 4, 7, 6,    // back
};

/*!
 * Thread helper function. Calls the thread proc of the encoding thread class passed
 * in by userData.
 *
 * \param userData [in]
 *   Pointer to an instance of the encoding thread class
 *
 * \return 0 if passed
 */
static unsigned int ThreadHelperProc(void *userData)
{
    EncodingThread *encodingThread = (EncodingThread*)userData;

    return (encodingThread->threadProc() == false);
}

/*!
 * Constructor.
 */
EncodingThread::EncodingThread(unsigned int width, unsigned int height, unsigned int deviceIndex, HGPUNV hGPU, unsigned int instance, NV_IFROGL_HW_ENC_TYPE eCodec) :
    m_width(width),
    m_height(height),
    m_hGPU(hGPU),
    m_deviceIndex(deviceIndex),
    m_instance(instance),
    m_hDC(0),
    m_hRC(0),
    m_sessionHandle(0),
    m_transferObjectHandle(0),
    m_fboID(0),
    m_vboID(0),
    m_iboID(0),
    m_frameRate(0.f),
    m_terminateThread(false),
	m_eCodec(eCodec)
{
    m_deviceString[0] = '\0';
    m_texID[0] = 0;
    m_texID[1] = 0;
}

/*!
 * Start the encoding thread.
 *
 * \return false if failed.
 */
bool EncodingThread::startup()
{
    if (!m_thread.create(ThreadHelperProc, (void*)this))
        return false;

    m_startedUp.wait();

    return true;
}

/*!
 * Terminate the encoding thread.
 *
 * \return false if failed.
 */
bool EncodingThread::terminate()
{
    // terminate the thread
    m_terminateThread = true;

    if (!m_thread.waitForExit())
    {
        fprintf(stderr, "Failed to wait for the encoder thread to exit.\n");
        return false;
    }

    return true;
}

/*!
 * Setup OpenGL.
 *
 * \return false if failed.
 */
bool EncodingThread::initOpenGL()
{
    GPU_DEVICE gpuDevice;
    HGPUNV gpuMask[2];
    PIXELFORMATDESCRIPTOR pfd;
    int pixelFormat;

    gpuMask[0] = m_hGPU;
    gpuMask[1] = NULL;
    m_hDC = wglCreateAffinityDCNV(gpuMask);
    if (!m_hDC)
    {
        printf("wglCreateAffinityDCNV failed\n"); 
        return false;
    }

    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_SUPPORT_OPENGL;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;

    pixelFormat = ChoosePixelFormat(m_hDC, &pfd);
    if (pixelFormat == 0)
    {
        printf("ChoosePixelFormat failed\n"); 
        return false;
    }
    if (!SetPixelFormat(m_hDC, pixelFormat, &pfd))
    {
        printf("SetPixelFormat failed\n"); 
        return false;
    }

    m_hRC = wglCreateContext(m_hDC);
    if (m_hRC == 0)
    {
        fprintf(stderr, "Failed to create context.\n");
        return false;
    }

    if (!wglMakeCurrent(m_hDC, m_hRC))
    {
        fprintf(stderr, "wglMakeCurrent failed");
        return false;
    }

    // get the device name
    if (wglEnumGpuDevicesNV(m_hGPU, 0, &gpuDevice))
    {
        strncpy(m_deviceString, gpuDevice.DeviceString, sizeof(m_deviceString));
    }

    // Set up the frame buffer
    glGenTextures(2, m_texID);

    glBindTexture(GL_TEXTURE_RECTANGLE, m_texID[0]);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glBindTexture(GL_TEXTURE_RECTANGLE, m_texID[1]);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);

    glGenFramebuffers(1, &m_fboID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texID[0], 0);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_texID[1], 0);

    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Framebuffer is incomplete.\n");
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Set up the geometry data
    glGenBuffers(1, &m_vboID);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), &vertexData, GL_STATIC_DRAW);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 6 * sizeof(GLfloat), 0);
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(3, GL_FLOAT, 6 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

    glGenBuffers(1, &m_iboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iboID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexData), &indexData, GL_STATIC_DRAW);

    return true;
}

/*!
 * Cleanup OpenGL resources.
 *
 * \return false if failed.
 */
bool EncodingThread::cleanupOpenGL()
{
    glDeleteBuffers(1, &m_iboID);
    glDeleteBuffers(1, &m_vboID);
    glDeleteTextures(2, m_texID);
    glDeleteFramebuffers(1, &m_fboID);

    if (m_hRC)
    {
        // the main context is no longer needed
        if (!wglDeleteContext(m_hRC))
        {
            fprintf(stderr, "wglDeleteContext failed");
            return false;
        }
        m_hRC = 0;
    }

    if (m_hDC)
    {
        wglDeleteDCNV(m_hDC);
        m_hDC = 0;
    }

    return true;
}

/*!
 * NvIFR debug call back function.
 *
 * \param severity [in]
 *   severity
 * \param message [in]
 *   debug message
 * \param userParam [in]
 *   user provided parameter
 */
void NVIFROGLAPI nvIFRDebugCallBack(NV_IFROGL_DEBUG_SEVERITY severity, const char *message, void *userParam)
{
    const char *severityString[] =
    {
        "notification",
        "low",
        "medium",
        "high"
    };

    assert(severity < sizeof(severityString) / sizeof(severityString[0]));

    fprintf(stderr, "NvIFR debug message (severity %s): %s.\n", severityString[severity], message);
    fflush(stderr);
}

/*!
 * Setup NvIFR.
 *
 * \return false if failed.
 */
bool EncodingThread::initNvIFR()
{
    NV_IFROGL_H264_ENC_CONFIG config;

    if (m_nvIFR.initialize() == false)
    {
        fprintf(stderr, "Failed to create a NvIFROGL instance.\n");
        return false;
    }

    if (m_nvIFR.nvIFROGLDebugMessageCallback(&nvIFRDebugCallBack, NULL) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to set the NvIFR debug callback function.\n");
        return false;
    }

    // A session is required. The session is associated with the current OpenGL context.
    if (m_nvIFR.nvIFROGLCreateSession(&m_sessionHandle, NULL) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to create a NvIFROGL session.\n");
        return false;
    }

    memset(&config, 0, sizeof(config));

    config.profile          = m_eCodec == NV_IFROGL_HW_ENC_H264 ? 100 : 1;
    config.frameRateNum     = 30;
    config.frameRateDen     = 1;
    config.width            = m_width;
    config.height           = m_height;
    config.avgBitRate       = calculateBitrate(m_width, m_height);
    config.peakBitRate      = config.avgBitRate;
    config.GOPLength        = 0xFFFFFFFF;
    config.rateControl      = NV_IFROGL_HW_ENC_RATE_CONTROL_2_PASS_QUALITY;
    config.preset           = NV_IFROGL_HW_ENC_PRESET_LOW_LATENCY_HP;
    config.stereoFormat     = NV_IFROGL_HW_ENC_STEREO_NONE;
    config.VBVBufferSize    = config.avgBitRate / config.frameRateNum;
    config.VBVInitialDelay  = config.VBVBufferSize;
	config.codecType        = m_eCodec;

    if (m_nvIFR.nvIFROGLCreateTransferToHwEncObject(m_sessionHandle, &config, &m_transferObjectHandle) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to create a NvIFROGL transfer object.\n");
        return false;
    }

    return true;
}

/*!
 * Cleanup NvIFR resources.
 *
 * \return false if failed.
 */
bool EncodingThread::cleanupNvIFR()
{
    if (m_transferObjectHandle)
    {
        if (m_nvIFR.nvIFROGLDestroyTransferObject(m_transferObjectHandle) != NV_IFROGL_SUCCESS)
        {
            fprintf(stderr, "Failed to destroy the NvIFROGL transfer object.\n");
            return false;
        }
        m_transferObjectHandle = 0;
    }

    if (m_sessionHandle)
    {
        if (m_nvIFR.nvIFROGLDestroySession(m_sessionHandle) != NV_IFROGL_SUCCESS)
        {
            fprintf(stderr, "Failed to destroy the NvIFROGL session.\n");
            return false;
        }
        m_sessionHandle = 0;
    }

    return true;
}

/*!
 * Thread procedure.
 *
 * \return false if failed
 */
bool EncodingThread::threadProc()
{
    uintptr_t dataSize;
    unsigned int transferCounter = 0;
    unsigned int lastUpdateTransfer = 0;
    timerValue startTime;
    float elapsedTime;
    float lastUpdateTime = 0.f;
    const void *data;
    char outFileName[256];
    FILE *fp;

	if (m_eCodec == NV_IFROGL_HW_ENC_H264) {
		sprintf(outFileName, "glIFR_Multi_out-0x%x.h264", GetCurrentThreadId());
	} else {
		sprintf(outFileName, "glIFR_Multi_out-0x%x.h265", GetCurrentThreadId());
	}
    fp = fopen(outFileName, "wb");
    if (!initOpenGL())
        return false;

    if (!initNvIFR())
        return false;

    glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
    glViewport(0, 0, m_width, m_height);

    glClearColor(0.2f, 0.4f, 0.7f, 1.f);

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.,1., -1.,1., 1.,5.);

    m_startedUp.signal();

    startTime = getTimeInuS();

    while (!m_terminateThread)
    {
        // Draw a colored cube to the FBO
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0.f, 0.f, -3.f);
        // rotate it, else it's boring
        glRotatef(14.f * ((float)transferCounter / (float)30.f) + 5.f, 0.f, 0.f, 1.f);
        glRotatef(20.f * ((float)transferCounter / (float)30.f) + 30.f, 0.f, 1.f, 0.f);
        glRotatef(10.f * ((float)transferCounter / (float)30.f) + 10.f, 1.f, 0.f, 0.f);
        glDrawElements(GL_QUADS, sizeof(indexData)/sizeof(indexData[0]), GL_UNSIGNED_SHORT, 0);

        // transfer the FBO
        if (m_nvIFR.nvIFROGLTransferFramebufferToHwEnc(m_transferObjectHandle, NULL, m_fboID, GL_COLOR_ATTACHMENT0, GL_NONE) != NV_IFROGL_SUCCESS)
        {
            fprintf(stderr, "Failed to transfer data from the framebuffer.\n");
            break;
        }

        // map the transferred data
        if (m_nvIFR.nvIFROGLLockTransferData(m_transferObjectHandle, &dataSize, &data) != NV_IFROGL_SUCCESS)
        {
            fprintf(stderr, "Failed to lock the transferred data.\n");
            break;
        }

        // write to the file
        fwrite(data, 1, dataSize, fp);

        // unmap the data buffer
        if (m_nvIFR.nvIFROGLReleaseTransferData(m_transferObjectHandle) != NV_IFROGL_SUCCESS)
        {
            fprintf(stderr, "Failed to release the transferred data.\n");
            break;
        }

        transferCounter++;
        elapsedTime = (float)(getTimeInuS() - startTime) / 1000000.f;

        // update the frame rate every second
        if (elapsedTime - lastUpdateTime > 1.f)
        {
            m_frameRate = (float)(transferCounter - lastUpdateTransfer) / (elapsedTime - lastUpdateTime);
            lastUpdateTransfer = transferCounter;
            lastUpdateTime = elapsedTime;
        }
    }
    fclose(fp);

    if (!cleanupNvIFR())
        return false;

    if (!cleanupOpenGL())
        return false;

    return true;
}
