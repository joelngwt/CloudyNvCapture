/*!
 * \file
 *
 * This sample shows how to use NvIFROpenGL to encode the contents of a
 * framebuffer with the hardware H.264 encoder without modifying the
 * OpenGL application. This library intercepts application's glX* calls
 * and introduces NvIFROpenGL APIs in-between the application call
 * and actual glX* call in GL.
 *
 * To use this library run an application using command line e.g.
 *     LD_PRELOAD=/path/to/this/library glxgears
 *
 * OpenGL rendering into application Window will get redirected to
 * FBO internally created and each frame is captured and encoded in
 * glXSwapBuffers() call.
 * 
 * This library demonstrates the NvIFROpenGL capture and encode
 * for single threaded applications using one OpenGL context
 * and rendering into single Window.
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

#include <stdio.h>
#include "NvIFR/NvIFROpenGL.h"
#include "NvIFR_API.h"
#ifndef WIN32
  #include "GL/glx.h"
#endif
#include "GL/gl.h"
#include "GL/glext.h"
#include <assert.h>
#ifndef WIN32
  #include <pthread.h>
#endif
#include "CommandLine.h"
#include "Timer.h"
#include "Util.h"

/*****************************************************************************/

//! shim initialized.
static bool shim_initialized = false;
//! the NvIFR API function list.
static NvIFRAPI nvIFR;
//! Encoder FPS.
static unsigned int framesPerSecond = 30;
//! output file.
FILE *outFile;
//! frame counter
static unsigned int frameCounter = 0;
//! the frame at which the last screen update had been done
static unsigned int lastUpdateFrame = 0;
//! time the first frame had been drawn
static timerValue startTime = 0;
//! time at which the last screen update had been done
static float lastUpdateTime = 0;

//! Track Window to FBO mapping.
typedef struct _FBO_MAP {
    Window       win;
    unsigned int width;
    unsigned int height;
    GLXWindow    glxWin;
    GLXDrawable  draw;
    GLXDrawable  read;
    GLXContext   ctx;
    GLuint       texID[2];
    GLuint       fboID;
    pthread_t    threadID;
    //! the NvIFR session handle
    NV_IFROGL_SESSION_HANDLE sessionHandle;
    //! the NvIFR transfer object handle
    NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle;
    _FBO_MAP     *next;
} FBO_MAP;

//! Head of the FBP mapping list.
static FBO_MAP *fbo_mapping = NULL;

PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffers;
PFNGLFRAMEBUFFERTEXTUREEXTPROC glFramebufferTexture;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;

#define SHIM_LOG(...)    printf(__VA_ARGS__)
/*****************************************************************************/

/*
 * Helper functions.
 */

void shim_init(void)
{
    if (shim_initialized) {
        return;
    }
    shim_initialized = true;

    outFile = fopen("movie.h264", "wb");
    if (outFile == NULL) {
        SHIM_LOG("Failed to create output file.\n");
        exit(-1);
    }

    if (nvIFR.initialize() == false) {
        SHIM_LOG("Failed to create a NvIFROGL instance.\n");
        exit(-1);
    }

    *(void **)&glBindFramebuffer = (void *)glXGetProcAddressARB((const GLubyte*)("glBindFramebuffer"));
    *(void **)&glGenBuffers = (void *)glXGetProcAddressARB((const GLubyte*)("glGenBuffers"));
    *(void **)&glGenFramebuffers = (void *)glXGetProcAddressARB((const GLubyte*)("glGenFramebuffers"));
    *(void **)&glDeleteFramebuffers = (void *)glXGetProcAddressARB((const GLubyte*)("glDeleteFramebuffers"));
    *(void **)&glFramebufferTexture = (void *)glXGetProcAddressARB((const GLubyte*)("glFramebufferTexture"));
    *(void **)&glCheckFramebufferStatus = (void *)glXGetProcAddressARB((const GLubyte*)("glCheckFramebufferStatus"));
}

static bool UseFboPresent(GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
    FBO_MAP *fbo;
    if (fbo_mapping == NULL) {
        return false;
    }

    for (fbo = fbo_mapping; fbo != NULL; fbo = fbo->next) {
        if (fbo->draw == draw &&
            fbo->read == read &&
            fbo->ctx  == ctx) {
            // Validate texID and fboID?
            glBindFramebuffer(GL_FRAMEBUFFER, fbo->fboID);
            glViewport(0, 0, fbo->width, fbo->height);
            return true;
        }
    }
    return false;
}

static FBO_MAP *GetFboFromGLXDrawable(GLXDrawable draw)
{
    FBO_MAP *fbo = NULL;

    if (fbo_mapping == NULL) {
        return NULL;
    }

    for (fbo = fbo_mapping; fbo != NULL; fbo = fbo->next) {
        if (fbo->glxWin == draw) {
            break;
        }
    }
    return fbo;
}

static bool RemoveFboFromList(FBO_MAP *fbo)
{
    FBO_MAP **track;

    assert(fbo_mapping);
    for (track = &fbo_mapping; *track != NULL; track = &((*track)->next)) {
        if (*track == fbo) {
            *track = (*track)->next;
            return true;
        }
    }
    return false;
}

static bool CreateFBO(FBO_MAP *fbo)
{
    // Set up the frame buffer.
    // Render into this fbo instead of Window.
    glGenTextures(2, fbo->texID);

    glBindTexture(GL_TEXTURE_RECTANGLE, fbo->texID[0]);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, fbo->width, fbo->height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glBindTexture(GL_TEXTURE_RECTANGLE, fbo->texID[1]);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT, fbo->width, 
                 fbo->height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);

    glGenFramebuffers(1, &fbo->fboID);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fboID);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fbo->texID[0], 0);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fbo->texID[1], 0);

    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SHIM_LOG("Framebuffer is incomplete.\n");
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

static FBO_MAP *CreateFboMap(Display * dpy, Window win, GLXWindow glxWin)
{
    Window root;
    int x, y;
    unsigned int width, height, border_width, depth;
    XGetGeometry(dpy, win, &root, &x, &y, &width, &height, &border_width, &depth);

    // Win <-> GLXWindow mapping to destroy fbo
    FBO_MAP *fbo = (FBO_MAP *)malloc(sizeof(FBO_MAP));
    if (fbo == NULL) {
        assert(0);
    }

    fbo->win    = win;
    fbo->width  = width;
    fbo->height = height;
    fbo->glxWin = glxWin;

    fbo->next   = fbo_mapping;
    fbo_mapping = fbo;

    return fbo;
}

static bool InitNvIFR(Display *dpy, GLXDrawable draw, GLXDrawable read,
                      GLXContext ctx)
{
    FBO_MAP *fboMap;

    shim_init();

    if (draw == None && read == None && ctx == NULL) {
        // Losing current.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    if (UseFboPresent(draw, read, ctx)) {
        return true;
    }

    fboMap = GetFboFromGLXDrawable(draw);
    if (fboMap == NULL) {
        // App might have passed drawable created by XCreateWindow.
        fboMap = CreateFboMap(dpy, draw, draw);
    }

    CreateFBO(fboMap);

    NV_IFROGL_H264_ENC_CONFIG config;
    // Create a NvIFR session. The session is associated with the current
    // OpenGL context.
    if (nvIFR.nvIFROGLCreateSession(&fboMap->sessionHandle, NULL) !=
        NV_IFROGL_SUCCESS) {
        SHIM_LOG("Failed to create a NvIFROGL session.\n");
        return false;
    }

    memset(&config, 0, sizeof(config));

    config.profile = 100;
    config.frameRateNum = framesPerSecond;
    config.frameRateDen = 1;
    config.width = fboMap->width;
    config.height = fboMap->height;
    config.avgBitRate = calculateBitrate(fboMap->width, fboMap->height);
    config.GOPLength = 75;
    config.rateControl = NV_IFROGL_H264_RATE_CONTROL_CBR;
    config.stereoFormat = NV_IFROGL_H264_STEREO_NONE;
    config.preset = NV_IFROGL_H264_PRESET_LOW_LATENCY_HP;

    // Create NvIFR h264 transfer object
    if (nvIFR.nvIFROGLCreateTransferToHwEncObject(fboMap->sessionHandle,
        &config, &fboMap->transferObjectHandle) != NV_IFROGL_SUCCESS) {
        SHIM_LOG("Failed to create a NvIFROGL transfer object.\n");
        return false;
    }

    fboMap->draw = draw;
    fboMap->read = read;
    fboMap->ctx  = ctx;
    fboMap->threadID = pthread_self();

    // Draw application frame into the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fboMap->fboID);
    glViewport(0, 0, fboMap->width, fboMap->height);

    return true;
}

/*****************************************************************************/

typedef GLXWindow (* PFNGLXCREATEWINDOWPROC) (Display *dpy, GLXFBConfig config,
                   Window win, const int *attrib_list);
GLXWindow glXCreateWindow(Display * dpy, GLXFBConfig config, Window win,
                          const int * attrib_list)
{
    GLXWindow ret = 0;
    __GLXextFuncPtr realFunction = NULL;
    realFunction = glXGetProcAddressARB((const GLubyte *) "glXCreateWindow");
    PFNGLXCREATEWINDOWPROC CreateWindow = (PFNGLXCREATEWINDOWPROC)realFunction;
    ret = (*CreateWindow)(dpy, config, win, attrib_list);

    CreateFboMap(dpy, win, ret);

    return ret;
}

typedef void ( * PFNGLXDESTROYWINDOWPROC) (Display *dpy, GLXWindow window);
void glXDestroyWindow(Display* dpy,GLXWindow window)
{
    FBO_MAP *fbo;
    __GLXextFuncPtr realFunction = NULL;

    realFunction = glXGetProcAddressARB((const GLubyte *) "glXDestroyWindow");
    PFNGLXDESTROYWINDOWPROC DestroyWindow = (PFNGLXDESTROYWINDOWPROC)realFunction;
    (*DestroyWindow)(dpy, window);

    fbo = GetFboFromGLXDrawable(window);

    if (nvIFR.nvIFROGLDestroyTransferObject(fbo->transferObjectHandle) !=
        NV_IFROGL_SUCCESS) {
        SHIM_LOG("Failed to destroy the NvIFROGL transfer object.\n");
        return;
    }

    if (nvIFR.nvIFROGLDestroySession(fbo->sessionHandle) != NV_IFROGL_SUCCESS) {
        SHIM_LOG("Failed to destroy the NvIFROGL session.\n");
        return;
    }

    glDeleteTextures(2, fbo->texID);
    glDeleteFramebuffers(1, &fbo->fboID);
    RemoveFboFromList(fbo);
    free(fbo);
}

typedef Bool (* PFNGLXMAKECONTEXTCURRENTPROC) (Display *dpy, GLXDrawable draw,
              GLXDrawable read, GLXContext ctx);
Bool glXMakeContextCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read,
                           GLXContext ctx)
{
    Bool ret = 0;
    __GLXextFuncPtr realFunction = NULL;
    PFNGLXMAKECONTEXTCURRENTPROC MakeContextCurrent;

    realFunction = glXGetProcAddressARB((const GLubyte *) "glXMakeContextCurrent");
    MakeContextCurrent = (PFNGLXMAKECONTEXTCURRENTPROC)realFunction;
    ret = (*MakeContextCurrent)(dpy, draw, read, ctx);

    assert(ret == true);
    assert(draw == read);

    InitNvIFR(dpy, draw, read, ctx);

    return ret;
}

typedef Bool (* PFNGLXMAKECURRENTPROC) (Display *dpy, GLXDrawable draw,
              GLXContext glxc);
Bool glXMakeCurrent(Display * dpy, GLXDrawable draw, GLXContext glxc)
{
    Bool ret = 0;
    __GLXextFuncPtr realFunction = NULL;
    PFNGLXMAKECURRENTPROC MakeCurrent;

    realFunction = glXGetProcAddressARB((const GLubyte *) "glXMakeCurrent");
    MakeCurrent = (PFNGLXMAKECURRENTPROC)realFunction;
    ret = (*MakeCurrent)(dpy, draw, glxc);

    assert(ret == true);

    InitNvIFR(dpy, draw, draw, glxc);

    return ret;
}

typedef Bool (* PFNGLXSWAPBUFFERSPROC) (Display *dpy, GLXDrawable drawable);
void glXSwapBuffers(Display * dpy, GLXDrawable drawable)
{
    uintptr_t dataSize;
    const void *data;
    FBO_MAP *fbo = GetFboFromGLXDrawable(drawable);
    float elapsedTime;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // transfer the FBO
    if (nvIFR.nvIFROGLTransferFramebufferToHwEnc(fbo->transferObjectHandle,
        NULL, fbo->fboID, GL_COLOR_ATTACHMENT0, GL_NONE) != NV_IFROGL_SUCCESS) {
        SHIM_LOG("Failed to transfer data from the framebuffer.\n");
        exit(-1);
    }

    // lock the transferred data
    if (nvIFR.nvIFROGLLockTransferData(fbo->transferObjectHandle, &dataSize,
        &data) != NV_IFROGL_SUCCESS) {
        SHIM_LOG("Failed to lock the transferred data.\n");
        exit(-1);
    }

    // write encoded frame to the h264 file.
    fwrite(data, 1, dataSize, outFile);

    // release the data buffer
    if (nvIFR.nvIFROGLReleaseTransferData(fbo->transferObjectHandle) !=
        NV_IFROGL_SUCCESS) {
        SHIM_LOG("Failed to release the transferred data.\n");
        exit(-1);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fboID);

    __GLXextFuncPtr realFunction = NULL;
    realFunction = glXGetProcAddressARB((const GLubyte *) "glXSwapBuffers");
    PFNGLXSWAPBUFFERSPROC SwapBuffers = (PFNGLXSWAPBUFFERSPROC)realFunction;
    (*SwapBuffers)(dpy, drawable);

    if (startTime == 0) {
        startTime = getTimeInuS();
    }

    frameCounter++;

    elapsedTime = (float)(getTimeInuS() - startTime) / 1000000;
    // print the frame rate every second
    if (elapsedTime - lastUpdateTime > 1.f) {
        float frameRate;

        frameRate = (float)((frameCounter - lastUpdateFrame))/ (elapsedTime - lastUpdateTime);
        lastUpdateFrame = frameCounter;
        lastUpdateTime = elapsedTime;

        SHIM_LOG("Encoding %dx%d at %4.0f fps\n", fbo->width, fbo->height, frameRate);
    }
}

